#pragma once

#include <Preferences.h>
#include "admin_svc.h"
#include "version.h"

// تاریخچهٔ واقعی نصب روی همین سخت‌افزار؛ NVS در آپلود عادی app0 حفظ می‌شود.
// ۱۵۰ رکورد آخر حلقوی نگه داشته می‌شود و seq شمارندهٔ کل نصب‌هاست.
static constexpr uint16_t FW_INSTALL_HISTORY_CAPACITY = 150;
static constexpr uint32_t FW_INSTALL_RECORD_MAGIC = 0x46574831UL;  // FWH1

struct FwInstallHistoryRecord {
  uint32_t magic;
  uint32_t seq;
  int64_t build_epoch;
  int64_t install_epoch;
  char version[16];
};

static void fw_install_history_slot_key(uint16_t slot, char key[8]) {
  snprintf(key, 8, "r%03u", (unsigned)slot);
}

static int64_t fw_install_history_build_epoch() {
  static const char mons[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mon[4] = {0};
  int day = 0, year = 0, hh = 0, mi = 0, ss = 0;
  if (sscanf(__DATE__, "%3s %d %d", mon, &day, &year) != 3 ||
      sscanf(__TIME__, "%d:%d:%d", &hh, &mi, &ss) != 3)
    return 0;
  const char *p = strstr(mons, mon);
  if (!p || ((p - mons) % 3) != 0) return 0;
  const int month = (int)((p - mons) / 3) + 1;
  return civil_to_epoch(year, month, day, hh, mi, ss);
}

static int64_t fw_install_history_device_epoch() {
  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  if (ds3231_read_time(&y, &m, &d, &hh, &mi, &ss)) {
    soft_clock_set_civil(y, m, d, hh, mi, ss);
    return civil_to_epoch(y, m, d, hh, mi, ss);
  }
  return soft_now_epoch();
}

static bool fw_install_history_record_valid(const FwInstallHistoryRecord &r) {
  return r.magic == FW_INSTALL_RECORD_MAGIC && r.seq > 0 &&
         r.version[0] != '\0' && r.version[sizeof(r.version) - 1] == '\0';
}

static bool fw_install_history_read_slot(Preferences &p, uint16_t slot,
                                         FwInstallHistoryRecord *out) {
  if (!out || slot >= FW_INSTALL_HISTORY_CAPACITY) return false;
  char key[8];
  fw_install_history_slot_key(slot, key);
  memset(out, 0, sizeof(*out));
  if (p.getBytesLength(key) != sizeof(*out) ||
      p.getBytes(key, out, sizeof(*out)) != sizeof(*out))
    return false;
  return fw_install_history_record_valid(*out);
}

// در هر بوت صدا زده می‌شود؛ ریست‌های تکراری همان نسخه رکورد تازه نمی‌سازند.
static bool fw_install_history_register_current() {
  Preferences p;
  if (!p.begin("fw_hist", false)) return false;

  uint16_t count = p.getUShort("count", 0);
  uint16_t next = p.getUShort("next", 0);
  uint32_t total = p.getUInt("total", 0);
  if (count > FW_INSTALL_HISTORY_CAPACITY) count = FW_INSTALL_HISTORY_CAPACITY;
  if (next >= FW_INSTALL_HISTORY_CAPACITY) next = 0;

  const String last = p.getString("last", "");
  if (count > 0 && last == PROJECT_VERSION_STRING) {
    // اگر RTC در بوت اول آماده نبود، زمان نصب همان رکورد را در فرصت بعد کامل کن.
    const uint16_t last_slot =
      (uint16_t)((next + FW_INSTALL_HISTORY_CAPACITY - 1) % FW_INSTALL_HISTORY_CAPACITY);
    FwInstallHistoryRecord r{};
    if (fw_install_history_read_slot(p, last_slot, &r) &&
        strcmp(r.version, PROJECT_VERSION_STRING) == 0 && r.install_epoch <= 0) {
      const int64_t now = fw_install_history_device_epoch();
      if (now > 0) {
        r.install_epoch = now;
        char key[8];
        fw_install_history_slot_key(last_slot, key);
        p.putBytes(key, &r, sizeof(r));
      }
    }
    p.end();
    return true;
  }

  FwInstallHistoryRecord r{};
  r.magic = FW_INSTALL_RECORD_MAGIC;
  r.seq = total + 1U;
  r.build_epoch = fw_install_history_build_epoch();
  r.install_epoch = fw_install_history_device_epoch();
  snprintf(r.version, sizeof(r.version), "%s", PROJECT_VERSION_STRING);
  r.version[sizeof(r.version) - 1] = '\0';

  char key[8];
  fw_install_history_slot_key(next, key);
  if (p.putBytes(key, &r, sizeof(r)) != sizeof(r)) {
    p.end();
    return false;
  }

  next = (uint16_t)((next + 1U) % FW_INSTALL_HISTORY_CAPACITY);
  if (count < FW_INSTALL_HISTORY_CAPACITY) count++;
  total = r.seq;
  p.putUShort("count", count);
  p.putUShort("next", next);
  p.putUInt("total", total);
  p.putString("last", PROJECT_VERSION_STRING);
  p.end();
  return true;
}

static void fw_install_history_stats(uint32_t *total_out, uint16_t *count_out) {
  uint32_t total = 0;
  uint16_t count = 0;
  Preferences p;
  if (p.begin("fw_hist", true)) {
    total = p.getUInt("total", 0);
    count = p.getUShort("count", 0);
    p.end();
  }
  if (count > FW_INSTALL_HISTORY_CAPACITY) count = FW_INSTALL_HISTORY_CAPACITY;
  if (total_out) *total_out = total;
  if (count_out) *count_out = count;
}

// offset صفر = جدیدترین رکورد.
static bool fw_install_history_read_newest(uint16_t offset, FwInstallHistoryRecord *out) {
  if (!out) return false;
  Preferences p;
  if (!p.begin("fw_hist", true)) return false;
  uint16_t count = p.getUShort("count", 0);
  uint16_t next = p.getUShort("next", 0);
  if (count > FW_INSTALL_HISTORY_CAPACITY) count = FW_INSTALL_HISTORY_CAPACITY;
  if (next >= FW_INSTALL_HISTORY_CAPACITY || offset >= count) {
    p.end();
    return false;
  }
  const uint16_t slot = (uint16_t)(
    (next + FW_INSTALL_HISTORY_CAPACITY - 1U - offset) % FW_INSTALL_HISTORY_CAPACITY);
  const bool ok = fw_install_history_read_slot(p, slot, out);
  p.end();
  return ok;
}
