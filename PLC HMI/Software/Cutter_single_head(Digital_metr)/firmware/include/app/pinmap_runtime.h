#ifndef HAFFLER_CUTTER_PINMAP_RUNTIME_H
#define HAFFLER_CUTTER_PINMAP_RUNTIME_H

// نگاشت منطقی مستقل ده ورودی I0..I7 و T1/T2.
// زوج‌های I0/I1 و I2/I3 از نظر سخت‌افزاری ADC مشترک دارند.
// I4/I5 روی ADC GPIO4: نقش پیش‌فرض = سنسور برگشت تیغه / خطای موتور.
// T1/T2 ورودی دیجیتال مستقل GPIO21/GPIO37 هستند و مانند Iها قابل نگاشت‌اند.
// هر منبع فیزیکی فقط یک مالک دارد؛ انتخاب منبع اشغال‌شده دو نقش را جابه‌جا می‌کند.

static const uint8_t PINMAP_IN_COUNT = 10;
static const uint8_t PINMAP_IN_NC = 10;
static volatile uint8_t pinmap_in_source[PINMAP_IN_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static volatile bool pinmap_in_physical_active[PINMAP_IN_COUNT] =
  {false, false, false, false, false, false, false, false, false, false};
static volatile bool pinmap_motion_limits_ready_cached = true;
static volatile uint32_t pinmap_in_generation = 1;
static portMUX_TYPE pinmap_in_mux = portMUX_INITIALIZER_UNLOCKED;

static uint64_t pinmap_in_pack(const uint8_t map[PINMAP_IN_COUNT]) {
  uint64_t packed = 0;
  for (int role = 0; role < PINMAP_IN_COUNT; ++role)
    packed |= ((uint64_t)(map[role] & 0x0fU)) << (role * 4);
  return packed;
}

static bool pinmap_in_validate(const uint8_t map[PINMAP_IN_COUNT]) {
  bool used[PINMAP_IN_COUNT] =
    {false, false, false, false, false, false, false, false, false, false};
  for (int role = 0; role < PINMAP_IN_COUNT; ++role) {
    if (map[role] > PINMAP_IN_NC) return false;
    if (map[role] == PINMAP_IN_NC) continue;
    if (used[map[role]]) return false;
    used[map[role]] = true;
  }
  return true;
}

static void pinmap_in_defaults(uint8_t map[PINMAP_IN_COUNT]) {
  for (int i = 0; i < PINMAP_IN_COUNT; ++i) map[i] = (uint8_t)i;
}

static void pinmap_in_load() {
  uint8_t map[PINMAP_IN_COUNT];
  pinmap_in_defaults(map);
  pref.begin("params", true);
  const bool has_v2 = pref.isKey("pm_in_v2");
  const bool has_v1 = pref.isKey("pm_in_v1");
  const uint64_t packed_v2 = pref.getULong64("pm_in_v2", pinmap_in_pack(map));
  const uint64_t packed_v1 = pref.getULong64("pm_in_v1", 0);
  pref.end();
  if (has_v2) {
    for (int role = 0; role < PINMAP_IN_COUNT; ++role)
      map[role] = (uint8_t)((packed_v2 >> (role * 4)) & 0x0fU);
  } else if (has_v1) {
    // مهاجرت نسخهٔ هشت‌ورودی: مقدار 8 قدیمی «قطع» بود، نه T1.
    for (int role = 0; role < 8; ++role) {
      const uint8_t legacy = (uint8_t)((packed_v1 >> (role * 4)) & 0x0fU);
      map[role] = legacy == 8 ? PINMAP_IN_NC : legacy;
    }
  }
  if (!pinmap_in_validate(map)) pinmap_in_defaults(map);
  portENTER_CRITICAL(&pinmap_in_mux);
  for (int role = 0; role < PINMAP_IN_COUNT; ++role) pinmap_in_source[role] = map[role];
  pinmap_motion_limits_ready_cached =
    (map[0] < PINMAP_IN_COUNT && map[1] < PINMAP_IN_COUNT);
  ++pinmap_in_generation;
  portEXIT_CRITICAL(&pinmap_in_mux);
}

static bool pinmap_in_save_snapshot(const uint8_t map[PINMAP_IN_COUNT]) {
  if (!pinmap_in_validate(map)) return false;
  pref.begin("params", false);
  const size_t written = pref.putULong64("pm_in_v2", pinmap_in_pack(map));
  pref.end();
  return written == sizeof(uint64_t);
}

static uint8_t pinmap_in_get_source(int role) {
  if (role < 0 || role >= PINMAP_IN_COUNT) return PINMAP_IN_NC;
  portENTER_CRITICAL(&pinmap_in_mux);
  const uint8_t src = pinmap_in_source[role];
  portEXIT_CRITICAL(&pinmap_in_mux);
  return src;
}

static bool pinmap_in_role_connected(int role) {
  return pinmap_in_get_source(role) < PINMAP_IN_COUNT;
}

static inline bool IRAM_ATTR pinmap_in_motion_limits_ready() {
  return pinmap_motion_limits_ready_cached;
}

static bool pinmap_in_role_active(int role) {
  if (role < 0 || role >= PINMAP_IN_COUNT) return false;
  uint8_t src;
  bool active = false;
  portENTER_CRITICAL(&pinmap_in_mux);
  src = pinmap_in_source[role];
  if (src < PINMAP_IN_COUNT) active = pinmap_in_physical_active[src];
  portEXIT_CRITICAL(&pinmap_in_mux);

  // Fail-safe: قطع نرم‌افزاریِ نقش استپ، دستگاه را متوقف نگه می‌دارد؛
  // قطع درب محافظ false می‌ماند تا سیکل هرگز شروع نشود.
  if (src == PINMAP_IN_NC && role == 3) return true;
  return active;
}

// [UI] Read one physical input source for the F3 live-status display.
static bool pinmap_in_source_active(uint8_t source) {
  if (source >= PINMAP_IN_COUNT) return false;
  bool active;
  portENTER_CRITICAL(&pinmap_in_mux);
  active = pinmap_in_physical_active[source];
  portEXIT_CRITICAL(&pinmap_in_mux);
  return active;
}

static void pinmap_in_publish_pair(int first, bool first_active, int second, bool second_active) {
  if (first < 0 || first >= PINMAP_IN_COUNT ||
      second < 0 || second >= PINMAP_IN_COUNT) return;
  portENTER_CRITICAL(&pinmap_in_mux);
  pinmap_in_physical_active[first] = first_active;
  pinmap_in_physical_active[second] = second_active;
  portEXIT_CRITICAL(&pinmap_in_mux);
}

static void pinmap_in_publish_one(int terminal, bool active) {
  if (terminal < 0 || terminal >= PINMAP_IN_COUNT) return;
  portENTER_CRITICAL(&pinmap_in_mux);
  pinmap_in_physical_active[terminal] = active;
  portEXIT_CRITICAL(&pinmap_in_mux);
}

static uint32_t pinmap_in_get_generation() {
  portENTER_CRITICAL(&pinmap_in_mux);
  const uint32_t generation = pinmap_in_generation;
  portEXIT_CRITICAL(&pinmap_in_mux);
  return generation;
}

// نگاشت مستقل تمام خروجی‌های Q0..Q9. نقش‌های 0/1/2 به‌ترتیب پالس/جهت/فعال‌سازی استپرند.
// هر ترمینال فقط یک مالک دارد؛ انتخاب ترمینال اشغال‌شده فقط همان دو نقش را جابه‌جا می‌کند.
static const uint8_t PINMAP_OUT_NC = 10;
static volatile uint8_t pinmap_out_source[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static const int pinmap_out_gpio[10] = {16, 42, 39, 48, 40, 15, 35, 36, 38, 41};
static volatile bool pinmap_stepper_rebind_pending = false;
static volatile bool pinmap_stepper_ready_cached = true;
static portMUX_TYPE pinmap_out_mux = portMUX_INITIALIZER_UNLOCKED;

static int *pinmap_out_role_var(int role) {
  switch (role) {
    case 0: return &IOS.pul_pin;
    case 1: return &IOS.dir_pin;
    case 2: return &IOS.enb_pin;
    case 3: return &IOS.u_clip_pin;
    case 4: return &IOS.f_clip_pin;
    case 5: return &IOS.spray_pin;
    case 6: return &IOS.saw_pin;
    case 7: return &IOS.lamp_pin;
    case 8: return &IOS.sawl_ift_pin;
    case 9: return &IOS.q9_pin;
    default: return NULL;
  }
}

static int pinmap_out_gpio_for_source(uint8_t source) {
  if (source > 9) return -1;
  return pinmap_out_gpio[source];
}

static bool pinmap_out_validate(const uint8_t map[10]) {
  bool used[10] = {false, false, false, false, false, false, false, false, false, false};
  for (int i = 0; i < 10; ++i) {
    if (map[i] == PINMAP_OUT_NC) continue;
    if (map[i] > 9 || used[map[i]]) return false;
    used[map[i]] = true;
  }
  return true;
}

static uint64_t pinmap_out_pack(const uint8_t map[10]) {
  uint64_t packed = 0;
  for (int i = 0; i < 10; ++i) packed |= ((uint64_t)(map[i] & 0x0fU)) << (i * 4);
  return packed;
}

static bool pinmap_out_stepper_connected_map(const uint8_t map[10]) {
  return map[0] < 10 && map[1] < 10 && map[2] < 10;
}

static int pinmap_out_owner_in_map(const uint8_t map[10], int source) {
  for (int role = 0; role < 10; ++role) if (map[role] == source) return role;
  return -1;
}

static void pinmap_out_set_safe_levels(const uint8_t map[10]) {
  for (int source = 0; source < 10; ++source) {
    const int gpio = pinmap_out_gpio_for_source((uint8_t)source);
    const int role = pinmap_out_owner_in_map(map, source);
    // پالس در حالت امن LOW است؛ جهت، EN و تمام عملگرهای active-low در حالت امن HIGH.
    // استاندارد زیمنس: مقداردهی به ثبات خروجی پیش از تغییر مود جهت حذف شوک اولیه بوبین
    digitalWrite(gpio, role == 0 ? LOW : HIGH);
    pinMode(gpio, OUTPUT);
  }
}

static void pinmap_out_commit_runtime(const uint8_t map[10], bool request_stepper_rebind) {
  portENTER_CRITICAL(&pinmap_out_mux);
  for (int i = 0; i < 10; ++i) pinmap_out_source[i] = map[i];
  if (request_stepper_rebind) pinmap_stepper_ready_cached = false;
  portEXIT_CRITICAL(&pinmap_out_mux);
  for (int role = 0; role <= 9; ++role) {
    int *v = pinmap_out_role_var(role);
    if (v) *v = pinmap_out_gpio_for_source(map[role]);
  }
  // Task3 فقط بعد از تکمیل همهٔ IOS.* مجاز است پایه‌های سریع استپر را دوباره bind کند.
  if (request_stepper_rebind) {
    portENTER_CRITICAL(&pinmap_out_mux);
    pinmap_stepper_rebind_pending = true;
    portEXIT_CRITICAL(&pinmap_out_mux);
  }
}

static void pinmap_out_load() {
  uint8_t map[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  pref.begin("params", true);
  const bool has_v2 = pref.isKey("pm_out_v2");
  const bool has_v1 = pref.isKey("pm_out_v1");
  const uint64_t packed_v2 = pref.getULong64("pm_out_v2", pinmap_out_pack(map));
  const uint32_t packed_v1 = pref.getUInt("pm_out_v1", 0);
  pref.end();
  if (has_v2) {
    for (int i = 0; i < 10; ++i) map[i] = (uint8_t)((packed_v2 >> (i * 4)) & 0x0fU);
  } else if (has_v1) {
    // مهاجرت بدون تغییر Q0..Q2 از نسخه‌ای که فقط Q3..Q9 را ذخیره می‌کرد.
    for (int role = 3; role <= 9; ++role)
      map[role] = (uint8_t)((packed_v1 >> ((role - 3) * 4)) & 0x0fU);
  }
  if (!pinmap_out_validate(map)) {
    for (int i = 0; i < 10; ++i) map[i] = (uint8_t)i;
  }
  pinmap_out_set_safe_levels(map);
  pinmap_out_commit_runtime(map, false);
  pinmap_stepper_ready_cached = pinmap_out_stepper_connected_map(map);
}

static uint8_t pinmap_out_get_source(int role) {
  if (role < 0 || role > 9) return PINMAP_OUT_NC;
  portENTER_CRITICAL(&pinmap_out_mux);
  const uint8_t src = pinmap_out_source[role];
  portEXIT_CRITICAL(&pinmap_out_mux);
  return src;
}

static inline bool IRAM_ATTR pinmap_out_stepper_ready() {
  return pinmap_stepper_ready_cached;
}

static bool pinmap_out_take_stepper_rebind_request() {
  portENTER_CRITICAL(&pinmap_out_mux);
  const bool pending = pinmap_stepper_rebind_pending;
  pinmap_stepper_rebind_pending = false;
  portEXIT_CRITICAL(&pinmap_out_mux);
  return pending;
}

static void pinmap_out_finish_stepper_rebind(bool ready) {
  portENTER_CRITICAL(&pinmap_out_mux);
  pinmap_stepper_ready_cached = ready;
  portEXIT_CRITICAL(&pinmap_out_mux);
}

static bool pinmap_out_process_ready() {
  // درایو استپر، گیره بالا، گیره محافظ، اره و بالابر برای سیکل الزامی‌اند.
  // اسپری و لامپ می‌توانند عمداً N.C. باشند؛ Q9 رزرو است.
  if (!pinmap_out_stepper_ready()) return false;
  static const uint8_t required_roles[] = {0, 1, 2, 3, 4, 6, 8};
  for (unsigned i = 0; i < sizeof(required_roles); ++i)
    if (pinmap_out_get_source(required_roles[i]) == PINMAP_OUT_NC) return false;
  return true;
}

// تغییرات صفحهٔ تنظیم پایه فقط در RAM نگه داشته می‌شود. NVS و GPIO فقط با
// کلید ذخیرهٔ بالای صفحه commit می‌شوند.
static uint8_t pinmap_in_pending[PINMAP_IN_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static uint8_t pinmap_out_pending[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static bool pinmap_pending_ready = false;
static uint32_t pinmap_pending_revision = 0;

static void pinmap_pending_touch() {
  pinmap_pending_revision = pinmap_pending_revision + 1U;
}

static void pinmap_pending_begin() {
  bool changed = !pinmap_pending_ready;
  portENTER_CRITICAL(&pinmap_in_mux);
  for (int i = 0; i < PINMAP_IN_COUNT; ++i) {
    if (pinmap_in_pending[i] != pinmap_in_source[i]) changed = true;
    pinmap_in_pending[i] = pinmap_in_source[i];
  }
  portEXIT_CRITICAL(&pinmap_in_mux);
  portENTER_CRITICAL(&pinmap_out_mux);
  for (int i = 0; i < 10; ++i) {
    if (pinmap_out_pending[i] != pinmap_out_source[i]) changed = true;
    pinmap_out_pending[i] = pinmap_out_source[i];
  }
  portEXIT_CRITICAL(&pinmap_out_mux);
  pinmap_pending_ready = true;
  if (changed) pinmap_pending_touch();
}

static void pinmap_pending_ensure() {
  if (!pinmap_pending_ready) pinmap_pending_begin();
}

static void pinmap_pending_discard() {
  pinmap_pending_begin();
}

static uint8_t pinmap_in_get_pending_source(int role) {
  if (role < 0 || role >= PINMAP_IN_COUNT) return PINMAP_IN_NC;
  pinmap_pending_ensure();
  return pinmap_in_pending[role];
}

static uint8_t pinmap_out_get_pending_source(int role) {
  if (role < 0 || role > 9) return PINMAP_OUT_NC;
  pinmap_pending_ensure();
  return pinmap_out_pending[role];
}

static bool pinmap_pending_has_changes() {
  pinmap_pending_ensure();
  bool changed = false;
  portENTER_CRITICAL(&pinmap_in_mux);
  for (int i = 0; i < PINMAP_IN_COUNT; ++i)
    if (pinmap_in_pending[i] != pinmap_in_source[i]) { changed = true; break; }
  portEXIT_CRITICAL(&pinmap_in_mux);
  if (changed) return true;
  portENTER_CRITICAL(&pinmap_out_mux);
  for (int i = 0; i < 10; ++i)
    if (pinmap_out_pending[i] != pinmap_out_source[i]) { changed = true; break; }
  portEXIT_CRITICAL(&pinmap_out_mux);
  return changed;
}

static bool pinmap_in_stage(int role, uint8_t new_source) {
  if (role < 0 || role >= PINMAP_IN_COUNT || new_source > PINMAP_IN_NC) return false;
  pinmap_pending_ensure();
  uint8_t before[PINMAP_IN_COUNT];
  for (int i = 0; i < PINMAP_IN_COUNT; ++i) before[i] = pinmap_in_pending[i];
  const uint8_t old_source = pinmap_in_pending[role];
  if (old_source == new_source) return true;
  if (new_source < PINMAP_IN_COUNT) {
    int owner = -1;
    for (int i = 0; i < PINMAP_IN_COUNT; ++i)
      if (i != role && pinmap_in_pending[i] == new_source) { owner = i; break; }
    if (owner >= 0) pinmap_in_pending[owner] = old_source;
  }
  pinmap_in_pending[role] = new_source;
  if (!pinmap_in_validate(pinmap_in_pending)) {
    for (int i = 0; i < PINMAP_IN_COUNT; ++i) pinmap_in_pending[i] = before[i];
    return false;
  }
  pinmap_pending_touch();
  return true;
}

static bool pinmap_out_stage(int role, uint8_t new_source) {
  if (role < 0 || role > 9 || !(new_source <= 9 || new_source == PINMAP_OUT_NC))
    return false;
  pinmap_pending_ensure();
  uint8_t before[10];
  for (int i = 0; i < 10; ++i) before[i] = pinmap_out_pending[i];
  const uint8_t old_source = pinmap_out_pending[role];
  if (old_source == new_source) return true;
  if (new_source <= 9) {
    int owner = -1;
    for (int i = 0; i < 10; ++i)
      if (i != role && pinmap_out_pending[i] == new_source) { owner = i; break; }
    if (owner >= 0) pinmap_out_pending[owner] = old_source;
  }
  pinmap_out_pending[role] = new_source;
  if (!pinmap_out_validate(pinmap_out_pending)) {
    for (int i = 0; i < 10; ++i) pinmap_out_pending[i] = before[i];
    return false;
  }
  pinmap_pending_touch();
  return true;
}

static void pinmap_in_stage_defaults() {
  pinmap_pending_ensure();
  for (int i = 0; i < PINMAP_IN_COUNT; ++i) pinmap_in_pending[i] = (uint8_t)i;
  pinmap_pending_touch();
}

static void pinmap_out_stage_defaults() {
  pinmap_pending_ensure();
  for (int i = 0; i < 10; ++i) pinmap_out_pending[i] = (uint8_t)i;
  pinmap_pending_touch();
}

static bool pinmap_out_save_snapshot(const uint8_t map[10]) {
  if (!pinmap_out_validate(map)) return false;
  pref.begin("params", false);
  const size_t written = pref.putULong64("pm_out_v2", pinmap_out_pack(map));
  pref.end();
  return written == sizeof(uint64_t);
}

static bool pinmap_pending_commit() {
  pinmap_pending_ensure();
  if (!pinmap_in_validate(pinmap_in_pending) || !pinmap_out_validate(pinmap_out_pending))
    return false;

  uint8_t current_in[PINMAP_IN_COUNT];
  uint8_t current_out[10];
  portENTER_CRITICAL(&pinmap_in_mux);
  for (int i = 0; i < PINMAP_IN_COUNT; ++i) current_in[i] = pinmap_in_source[i];
  portEXIT_CRITICAL(&pinmap_in_mux);
  portENTER_CRITICAL(&pinmap_out_mux);
  for (int i = 0; i < 10; ++i) current_out[i] = pinmap_out_source[i];
  portEXIT_CRITICAL(&pinmap_out_mux);

  bool in_changed = false;
  bool out_changed = false;
  for (int i = 0; i < PINMAP_IN_COUNT; ++i)
    if (current_in[i] != pinmap_in_pending[i]) { in_changed = true; break; }
  for (int i = 0; i < 10; ++i)
    if (current_out[i] != pinmap_out_pending[i]) { out_changed = true; break; }
  if (!in_changed && !out_changed) return true;

  const bool old_limits_ready = pinmap_motion_limits_ready_cached;
  const bool old_stepper_ready = pinmap_out_stepper_ready();
  if (in_changed) {
    portENTER_CRITICAL(&pinmap_in_mux);
    pinmap_motion_limits_ready_cached = false;
    portEXIT_CRITICAL(&pinmap_in_mux);
  }
  if (out_changed) {
    pinmap_out_finish_stepper_rebind(false);
    pinmap_out_set_safe_levels(current_out);
  }

  bool in_saved = !in_changed || pinmap_in_save_snapshot(pinmap_in_pending);
  bool out_saved = in_saved && (!out_changed || pinmap_out_save_snapshot(pinmap_out_pending));
  if (!in_saved || !out_saved) {
    // اگر نوشتن دوم شکست خورد، رکورد اول نیز به مقدار قبلی برگردد.
    if (in_changed && in_saved) pinmap_in_save_snapshot(current_in);
    if (out_changed && out_saved) pinmap_out_save_snapshot(current_out);
    if (in_changed) {
      portENTER_CRITICAL(&pinmap_in_mux);
      pinmap_motion_limits_ready_cached = old_limits_ready;
      portEXIT_CRITICAL(&pinmap_in_mux);
    }
    if (out_changed) pinmap_out_finish_stepper_rebind(old_stepper_ready);
    return false;
  }

  if (in_changed) {
    portENTER_CRITICAL(&pinmap_in_mux);
    for (int i = 0; i < PINMAP_IN_COUNT; ++i) pinmap_in_source[i] = pinmap_in_pending[i];
    pinmap_motion_limits_ready_cached =
      (pinmap_in_pending[0] < PINMAP_IN_COUNT &&
       pinmap_in_pending[1] < PINMAP_IN_COUNT);
    ++pinmap_in_generation;
    portEXIT_CRITICAL(&pinmap_in_mux);
  }
  if (out_changed) {
    const bool stepper_changed =
      current_out[0] != pinmap_out_pending[0] ||
      current_out[1] != pinmap_out_pending[1] ||
      current_out[2] != pinmap_out_pending[2];
    pinmap_out_set_safe_levels(pinmap_out_pending);
    pinmap_out_commit_runtime(pinmap_out_pending, stepper_changed);
    if (!stepper_changed)
      pinmap_out_finish_stepper_rebind(pinmap_out_stepper_connected_map(pinmap_out_pending));
  }
  pinmap_pending_begin();
  return true;
}

enum {
  PINMAP_NOTICE_NONE = 0,
  PINMAP_NOTICE_I0,
  PINMAP_NOTICE_I1,
  PINMAP_NOTICE_Q0,
  PINMAP_NOTICE_Q1,
  PINMAP_NOTICE_Q2,
  PINMAP_NOTICE_I2,
  PINMAP_NOTICE_Q3,
  PINMAP_NOTICE_Q4,
  PINMAP_NOTICE_Q6,
  PINMAP_NOTICE_Q8,
  PINMAP_NOTICE_STEPPER_WAIT
};

static volatile uint8_t pinmap_notice_pending = PINMAP_NOTICE_NONE;
static volatile uint32_t pinmap_notice_last_ms = 0;
static portMUX_TYPE pinmap_notice_mux = portMUX_INITIALIZER_UNLOCKED;

static void pinmap_notice_request(uint8_t reason) {
  if (reason == PINMAP_NOTICE_NONE) return;
  const uint32_t now = millis();
  portENTER_CRITICAL(&pinmap_notice_mux);
  if (pinmap_notice_pending == PINMAP_NOTICE_NONE &&
      (pinmap_notice_last_ms == 0 || (uint32_t)(now - pinmap_notice_last_ms) >= 2500U)) {
    pinmap_notice_pending = reason;
    pinmap_notice_last_ms = now;
  }
  portEXIT_CRITICAL(&pinmap_notice_mux);
}

static uint8_t pinmap_notice_take() {
  portENTER_CRITICAL(&pinmap_notice_mux);
  const uint8_t reason = pinmap_notice_pending;
  pinmap_notice_pending = PINMAP_NOTICE_NONE;
  portEXIT_CRITICAL(&pinmap_notice_mux);
  return reason;
}

static uint8_t pinmap_motion_missing_reason() {
  if (pinmap_out_get_source(0) == PINMAP_OUT_NC) return PINMAP_NOTICE_Q0;
  if (pinmap_out_get_source(1) == PINMAP_OUT_NC) return PINMAP_NOTICE_Q1;
  if (pinmap_out_get_source(2) == PINMAP_OUT_NC) return PINMAP_NOTICE_Q2;
  if (!pinmap_out_stepper_ready()) return PINMAP_NOTICE_STEPPER_WAIT;
  return PINMAP_NOTICE_NONE;
}

static uint8_t pinmap_cut_missing_reason() {
  if (pinmap_in_get_source(2) == PINMAP_IN_NC) return PINMAP_NOTICE_I2;
  const uint8_t motion_reason = pinmap_motion_missing_reason();
  if (motion_reason != PINMAP_NOTICE_NONE) return motion_reason;
  if (pinmap_out_get_source(3) == PINMAP_OUT_NC) return PINMAP_NOTICE_Q3;
  if (pinmap_out_get_source(4) == PINMAP_OUT_NC) return PINMAP_NOTICE_Q4;
  if (pinmap_out_get_source(6) == PINMAP_OUT_NC) return PINMAP_NOTICE_Q6;
  if (pinmap_out_get_source(8) == PINMAP_OUT_NC) return PINMAP_NOTICE_Q8;
  return PINMAP_NOTICE_NONE;
}

static void pinmap_notice_request_motion_block() {
  pinmap_notice_request(pinmap_motion_missing_reason());
}

static void pinmap_notice_request_cut_block() {
  pinmap_notice_request(pinmap_cut_missing_reason());
}

static inline void pinmap_out_write(int pin, uint8_t level) {
  if (pin >= 0) digitalWrite(pin, level);
}

static inline int pinmap_out_read(int pin) {
  return pin >= 0 ? digitalRead(pin) : HIGH;
}

static inline void pinmap_out_mode(int pin, uint8_t mode) {
  if (pin >= 0) pinMode(pin, mode);
}

#endif
