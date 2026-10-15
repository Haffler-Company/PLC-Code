
#include <Preferences.h>
#include <math.h>
class SerializedPreferences : public Preferences {
 public:
  bool begin(const char *name, bool readOnly = false, const char *partition_label = NULL) {
    locked_ = preferencesSemaphore &&
              xSemaphoreTakeRecursive(preferencesSemaphore, portMAX_DELAY) == pdTRUE;
    return Preferences::begin(name, readOnly, partition_label);
  }

  void end() {
    Preferences::end();
    if (locked_) {
      locked_ = false;
      xSemaphoreGiveRecursive(preferencesSemaphore);
    }
  }

 private:
  bool locked_ = false;
};

SerializedPreferences pref;

// NVS روی فلش است: نوشتنِ مقدارِ تغییری نکرده فقط عمر فلش و زمان UI را مصرف می‌کند.
// همهٔ ذخیره‌های تنظیمات از این دروازه عبور می‌کنند؛ کلید غایب یک‌بار نوشته می‌شود.
static inline void pref_put_int_if_changed(Preferences &p, const char *key, int value) {
  if (!p.isKey(key) || p.getInt(key, value) != value) p.putInt(key, value);
}
static inline void pref_put_uint_if_changed(Preferences &p, const char *key, uint32_t value) {
  if (!p.isKey(key) || p.getUInt(key, value) != value) p.putUInt(key, value);
}
static inline void pref_put_float_if_changed(Preferences &p, const char *key, float value) {
  if (!p.isKey(key) || p.getFloat(key, value) != value) p.putFloat(key, value);
}
static inline void pref_put_bool_if_changed(Preferences &p, const char *key, bool value) {
  if (!p.isKey(key) || p.getBool(key, value) != value) p.putBool(key, value);
}
static inline void pref_put_uchar_if_changed(Preferences &p, const char *key, uint8_t value) {
  if (!p.isKey(key) || p.getUChar(key, value) != value) p.putUChar(key, value);
}
static inline void pref_put_string_if_changed(Preferences &p, const char *key, const String &value) {
  if (!p.isKey(key) || p.getString(key, value) != value) p.putString(key, value);
}

// F1 local web connection.  A mode is committed only after its radio path is
// usable; an unfinished chooser or failed STA password therefore survives no reboot.
enum WebWifiMode : uint8_t { WEB_WIFI_DIRECT = 0, WEB_WIFI_STA = 1 };
static inline bool web_wifi_saved_mode(WebWifiMode *mode) {
  if (!mode) return false;
  pref.begin("params", true);
  const bool present = pref.isKey("web_wifi_mode");
  const uint8_t raw = pref.getUChar("web_wifi_mode", WEB_WIFI_DIRECT);
  pref.end();
  if (!present || raw > WEB_WIFI_STA) return false;
  *mode = (WebWifiMode)raw;
  return true;
}
static inline void web_wifi_save_mode(WebWifiMode mode) {
  pref.begin("params", false);
  pref_put_uchar_if_changed(pref, "web_wifi_mode", (uint8_t)mode);
  pref.end();
}

// ۰=یک دقیقه، ۱=۱۰ دقیقه، ۲=۳۰ دقیقه، ۳..۶=فواصل بالاتر، ۷=خاموش. پیش‌فرض کارخانه ۳۰ دقیقه (حالت ۲) است.
uint8_t screensaver_timeout_mode = 2;
static inline void save_screensaver_timeout_mode() {
  pref.begin("params", false);
  pref_put_uchar_if_changed(pref, "ss_mode", screensaver_timeout_mode);
  pref.end();
}

// نوار تشخیص اختیاری است: فقط با انتخاب آگاهانهٔ اپراتور روی همهٔ صفحه‌ها دیده می‌شود.
bool diagnostics_overlay_enabled = false;
static inline void save_diagnostics_overlay_enabled() {
  pref.begin("params", false);
  pref_put_bool_if_changed(pref, "diag_ov", diagnostics_overlay_enabled);
  pref.end();
}

static inline float valid_axis_pitch_or_default(float pitch) {
  return (isfinite(pitch) && pitch >= 0.1f && pitch <= 200.0f) ? pitch : 20.0f;
}
static inline int valid_drv_ppr_or_default(int ppr) {
  return (ppr >= 200 && ppr <= 100000) ? ppr : 6400;
}
void save_speed() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "glb_spd", STP.global_speed);
  pref.end();
}
void save_init_speed() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "init_spd", STP.global_init_speed);
  pref.end();
}

void save_setlang() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "glb_slan", GUI.system_language);
  pref.end();
}
void save_init_pos() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "init_pos", STP.initial_Position_p);
  pref.end();
}
void save_final_pos() {
  pref.begin("params", false);
  pref_put_string_if_changed(pref, "final_pos", STP.final_position_mm);
  pref_put_int_if_changed(pref, "final_posp", STP.final_position_p);
  pref.end();
}
void save_axis_pitch() {
  STP.Axis_Pitch = valid_axis_pitch_or_default(STP.Axis_Pitch);
  pref.begin("params", false);
  pref_put_float_if_changed(pref, "ax_pch", STP.Axis_Pitch);
  pref.end();
}
void save_um_ppulse() {
  pref.begin("params", false);
  pref_put_float_if_changed(pref, "um_ppulse_f", STP.um_per_pulse);
  pref.end();
}
void save_L1_GUI() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "L1_GUI", GUI.L1_GUI);
  pref.end();
}
void save_L2_GUI() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "L2_GUI", GUI.L2_GUI);
  pref.end();
}
uint8_t sys_volume = 80;
uint8_t alarm_volume = 100;

void save_volumes() {
  pref.begin("params", false);
  pref_put_uchar_if_changed(pref, "sys_vol", sys_volume);
  pref_put_uchar_if_changed(pref, "alrm_vol", alarm_volume);
  pref.end();
}

void save_buzzer_state() {
  pref.begin("params", false);
  pref_put_bool_if_changed(pref, "buz_stat", GUI.buzzer_enable);
  pref.end();
}
void save_visible_raws() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "vis_raws", GUI.visible_raws);
  pref.end();
}
void save_permanently_preferences_sp_val(int index, float value) {
  if (index < 0 || index >= LIST_MAX) return;
  pref.begin("params", false);
  char key[16];
  snprintf(key, sizeof(key), "sp_val_%d", index);  // create key like "sp_val_0"
  pref_put_float_if_changed(pref, key, value);
  pref.end();
}
void save_permanently_preferences_count_val(int index, int value) {
  if (index < 0 || index >= LIST_MAX) return;
  pref.begin("params", false);
  char key[20];
  snprintf(key, sizeof(key), "count_val_%d", index);  // create key like "count_val_0"
  pref_put_int_if_changed(pref, key, value);
  pref.end();
}
void save_permanently_preferences_count_base(int index, int value) {
  if (index < 0 || index >= LIST_MAX) return;
  pref.begin("params", false);
  char key[20];
  snprintf(key, sizeof(key), "cnt_base_%d", index);
  pref_put_int_if_changed(pref, key, value);
  pref.end();
}
void save_permanently_preferences_angle_val(int index, uint8_t value) {
  if (index < 0 || index >= LIST_MAX) return;
  pref.begin("params", false);
  char key[20];
  snprintf(key, sizeof(key), "ang_%d", index);
  pref_put_uchar_if_changed(pref, key, value);
  pref.end();
}
void save_last_pos_p() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "lst_pos", STP.currentPosition_p);
  pref.end();
}
void save_cut_timer() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "cut_timer", IOS.cut_prcs_timer);
  pref.end();
}
void save_base_dir() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "base_dir", STP.system_base_dir);
  pref.end();
}
void save_main_f_mem(int idx) {
  if (idx < 0 || idx > 4) return;
  pref.begin("params", false);
  char k[10], kv[12];
  snprintf(k, sizeof(k), "fmem%d", idx + 1);
  snprintf(kv, sizeof(kv), "fmemv%d", idx + 1);
  pref_put_float_if_changed(pref, k, main_f_mem_mm[idx]);
  pref_put_bool_if_changed(pref, kv, main_f_mem_valid[idx]);
  pref.end();
}
void save_stopstate() {
  pref.begin("params", false);
  pref_put_bool_if_changed(pref, "stop_nc", stopstate);
  pref.end();
}
// سیاست ذخیرهٔ فرمان‌های اپ وب: 0=هنوز پرسیده نشده، 1=ذخیرهٔ خودکار، 2=تأیید روی نمایشگر
void save_web_save_policy() {
  pref.begin("params", false);
  pref_put_uchar_if_changed(pref, "wsvp", web_save_policy);
  pref.end();
}
struct {
  int global_speed;
  int global_init_speed;
  int system_language;
  int cut_prcs_timer;
  int system_base_dir;
  bool buzzer_enable;
  bool virtual_kb_enable;
  bool joystick_enable;
  bool blade_err_enable;
  float blade_err_mm;
  int visible_raws;
} PEND;

void load_pending_from_current() {
  PEND.global_speed     = STP.global_speed;
  PEND.global_init_speed = STP.global_init_speed;
  PEND.system_language  = GUI.system_language;
  PEND.cut_prcs_timer   = IOS.cut_prcs_timer;
  PEND.system_base_dir  = STP.system_base_dir;
  PEND.buzzer_enable    = GUI.buzzer_enable;
  PEND.virtual_kb_enable = GUI.virtual_kb_enable;
  PEND.joystick_enable  = GUI.joystick_enable;
  PEND.blade_err_enable = blade_err_enable;
  PEND.blade_err_mm     = blade_err_mm;
  PEND.visible_raws     = GUI.visible_raws;
}

void apply_pending() {
  STP.global_speed      = PEND.global_speed;
  STP.global_init_speed = PEND.global_init_speed;
  jog_speed_interval    = constrain(STP.global_speed, 8, 100);  // جاگ = همان سرعت اتوماتیک
  GUI.system_language   = PEND.system_language;
  IOS.cut_prcs_timer    = PEND.cut_prcs_timer;
  STP.system_base_dir   = PEND.system_base_dir;
  GUI.buzzer_enable     = PEND.buzzer_enable;
  GUI.virtual_kb_enable = PEND.virtual_kb_enable;
  GUI.joystick_enable   = PEND.joystick_enable;
  blade_err_enable      = PEND.blade_err_enable;
  blade_err_mm          = PEND.blade_err_mm;
  GUI.visible_raws      = PEND.visible_raws;
}

void save_all() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "glb_spd", STP.global_speed);
  pref_put_int_if_changed(pref, "init_spd", STP.global_init_speed);
  pref_put_int_if_changed(pref, "glb_slan", GUI.system_language);
  pref_put_int_if_changed(pref, "drv_ppr", STP.drv_ppr);
  pref_put_int_if_changed(pref, "init_pos", STP.initial_Position_p);
  pref_put_string_if_changed(pref, "final_pos", STP.final_position_mm);
  pref_put_int_if_changed(pref, "final_posp", STP.final_position_p);
  pref_put_float_if_changed(pref, "ax_pch", STP.Axis_Pitch);
  pref_put_float_if_changed(pref, "um_ppulse_f", STP.um_per_pulse);
  pref_put_int_if_changed(pref, "L1_GUI", GUI.L1_GUI);
  pref_put_int_if_changed(pref, "L2_GUI", GUI.L2_GUI);
  pref_put_bool_if_changed(pref, "buz_stat", GUI.buzzer_enable);
  pref_put_bool_if_changed(pref, "vkb_en", GUI.virtual_kb_enable);
  pref_put_uchar_if_changed(pref, "vkb_mode", GUI.vkb_mode);
  pref_put_bool_if_changed(pref, "diag_ov", diagnostics_overlay_enabled);
  pref_put_bool_if_changed(pref, "js_en", GUI.joystick_enable);
  pref_put_bool_if_changed(pref, "bld_err_en", blade_err_enable);
  pref_put_float_if_changed(pref, "bld_err", blade_err_mm);
  pref_put_int_if_changed(pref, "vis_raws", GUI.visible_raws);
  pref_put_int_if_changed(pref, "cut_timer", IOS.cut_prcs_timer);
  pref_put_int_if_changed(pref, "base_dir", STP.system_base_dir);
  pref_put_bool_if_changed(pref, "stop_nc", stopstate);
  pref_put_int_if_changed(pref, "stock_len", stock_len_mm);
  pref_put_float_if_changed(pref, "kerf", kerf_mm);
  pref_put_int_if_changed(pref, "trim", trim_mm);
  pref_put_int_if_changed(pref, "jog_spd", jog_speed_interval);
  pref_put_int_if_changed(pref, "jog_init", jog_init_speed);
  pref_put_int_if_changed(pref, "auto_ramp", auto_ramp_ms);
  pref_put_int_if_changed(pref, "jog_ramp", jog_ramp_ms);
  pref_put_int_if_changed(pref, "f3_t_puls", f3_stepper_test_pulses);
  pref_put_int_if_changed(pref, "f3_t_per", f3_stepper_test_period_ms);
  pref_put_bool_if_changed(pref, "prn_en", printer_enabled);
  pref_put_uchar_if_changed(pref, "prn_model", printer_model);
  pref_put_float_if_changed(pref, "cpu_talarm", cpu_temp_alarm_c);
  pref.end();
}
bool g_oobe_done = false;
String g_company_name = "HAFFLER";

void save_oobe_done(bool done) {
  g_oobe_done = done;
  pref.begin("params", false);
  pref_put_bool_if_changed(pref, "oobe_done", done);
  pref.end();
  Preferences p;
  p.begin("chone", false);
  p.putBool("AdAsk", done);
  p.end();
}

void save_company_name(const String &name) {
  g_company_name = name;
  if (g_company_name.length() == 0) g_company_name = "HAFFLER";
  if (g_company_name.length() > 22) g_company_name = g_company_name.substring(0, 22);
  pref.begin("params", false);
  pref_put_string_if_changed(pref, "comp_name", g_company_name);
  pref.end();
  Preferences p;
  p.begin("chone", false);
  p.putString("CoName", g_company_name);
  p.putBool("CoAsk", true);
  p.end();
}

void save_stock_kerf() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "stock_len", stock_len_mm);
  pref_put_float_if_changed(pref, "kerf", kerf_mm);
  pref_put_int_if_changed(pref, "trim", trim_mm);
  pref.end();
}
void save_jog_speed() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "jog_spd", jog_speed_interval);
  pref_put_int_if_changed(pref, "jog_init", jog_init_speed);
  pref_put_int_if_changed(pref, "jog_ramp", jog_ramp_ms);
  pref.end();
}
void save_ramp_times() {
  pref.begin("params", false);
  pref_put_int_if_changed(pref, "auto_ramp", auto_ramp_ms);
  pref_put_int_if_changed(pref, "jog_ramp", jog_ramp_ms);
  pref.end();
}
void save_list_sp_now(int index) {
  if (index < 0 || index >= LIST_MAX) return;
  GUI.pending_sp_save_bits[index >> 3] =
    (uint8_t)(GUI.pending_sp_save_bits[index >> 3] & ~(uint8_t)(1u << (index & 7)));
  save_permanently_preferences_sp_val(index, AUT.list_sp_Val_p[index]);
}
void save_list_count_now(int index) {
  if (index < 0 || index >= LIST_MAX) return;
  GUI.pending_count_save_bits[index >> 3] =
    (uint8_t)(GUI.pending_count_save_bits[index >> 3] & ~(uint8_t)(1u << (index & 7)));
  save_permanently_preferences_count_val(index, AUT.list_count_Val[index]);
}
void save_list_row_now(int index) {
  if (index < 0 || index >= LIST_MAX) return;
  GUI.pending_zero_bits[index >> 3] =
    (uint8_t)(GUI.pending_zero_bits[index >> 3] & ~(uint8_t)(1u << (index & 7)));
  save_list_sp_now(index);
  save_list_count_now(index);
}
void flush_pending_list_saves_now() {
  if (GUI.flag_save_visible_raws) {
    GUI.flag_save_visible_raws = false;
    save_visible_raws();
  }
  pref.begin("params", false);
  char key[24];
  for (int i = 0; i < LIST_MAX; i++) {
    uint8_t bit = (uint8_t)(1u << (i & 7));
    if (GUI.pending_zero_bits[i >> 3] & bit) {
      GUI.pending_zero_bits[i >> 3] = (uint8_t)(GUI.pending_zero_bits[i >> 3] & ~bit);
      AUT.list_sp_Val_p[i] = 0;
      AUT.list_count_Val[i] = 0;
      AUT.list_count_base_Val[i] = 0;
      snprintf(key, sizeof(key), "sp_val_%d", i);
      pref_put_float_if_changed(pref, key, 0.0f);
      snprintf(key, sizeof(key), "count_val_%d", i);
      pref_put_int_if_changed(pref, key, 0);
      snprintf(key, sizeof(key), "cnt_base_%d", i);
      pref_put_int_if_changed(pref, key, 0);
    }
    if (GUI.pending_sp_save_bits[i >> 3] & bit) {
      GUI.pending_sp_save_bits[i >> 3] = (uint8_t)(GUI.pending_sp_save_bits[i >> 3] & ~bit);
      snprintf(key, sizeof(key), "sp_val_%d", i);
      pref_put_float_if_changed(pref, key, AUT.list_sp_Val_p[i]);
    }
    if (GUI.pending_count_save_bits[i >> 3] & bit) {
      GUI.pending_count_save_bits[i >> 3] = (uint8_t)(GUI.pending_count_save_bits[i >> 3] & ~bit);
      snprintf(key, sizeof(key), "count_val_%d", i);
      pref_put_int_if_changed(pref, key, AUT.list_count_Val[i]);
    }
  }
  pref.end();
}
void save_total_cuts() {
  pref.begin("params", false);
  pref_put_uint_if_changed(pref, "tot_cuts", total_cuts);
  pref.end();
}
void save_user_cuts() {
  pref.begin("params", false);
  pref_put_uint_if_changed(pref, "user_cuts", user_cuts);
  pref.end();
}
void save_warranty_state() {
  pref.begin("params", false);
  pref_put_bool_if_changed(pref, "warr_on", warranty_active);
  pref_put_string_if_changed(pref, "warr_dt", warranty_date);
  pref.end();
}
void save_cut_counters() {
  // کل عمری = جمع سه مد؛ tot_cuts جدا نوشته نمی‌شود تا یک put کمتر باشد
  total_cuts = cuts_life_manual + cuts_life_list + cuts_life_opt;
  pref.begin("params", false);
  pref_put_uint_if_changed(pref, "user_cuts", user_cuts);
  pref_put_uint_if_changed(pref, "cl_man", cuts_life_manual);
  pref_put_uint_if_changed(pref, "cl_lst", cuts_life_list);
  pref_put_uint_if_changed(pref, "cl_opt", cuts_life_opt);
  pref_put_uint_if_changed(pref, "cd_man", cuts_day_manual);
  pref_put_uint_if_changed(pref, "cd_lst", cuts_day_list);
  pref_put_uint_if_changed(pref, "cd_opt", cuts_day_opt);
  pref_put_uint_if_changed(pref, "cd_key", cuts_day_key);
  pref_put_uint_if_changed(pref, "bld_cuts", blade_cuts);
  pref_put_uint_if_changed(pref, "bld_lim", blade_limit);
  pref_put_bool_if_changed(pref, "bld_rem", blade_reminder_enabled);
  pref_put_string_if_changed(pref, "bld_dt", blade_service_date);
  pref.end();
}
void blade_service_reset(const char *date_str) {
  blade_cuts = 0;
  if (date_str && strlen(date_str) > 0) {
    strncpy(blade_service_date, date_str, sizeof(blade_service_date) - 1);
    blade_service_date[sizeof(blade_service_date) - 1] = '\0';
  }
  save_cut_counters();
}
void blade_set_limit(uint32_t lim) {
  if (lim < 100) lim = 100;
  if (lim > 100000) lim = 100000;
  blade_limit = lim;
  save_cut_counters();
}
void blade_set_reminder(bool en) {
  blade_reminder_enabled = en;
  save_cut_counters();
}
void save_cpu_temp_alarm() {
  pref.begin("params", false);
  pref_put_float_if_changed(pref, "cpu_talarm", cpu_temp_alarm_c);
  pref.end();
}
void save_admin_pin() {
  pref.begin("params", false);
  pref_put_string_if_changed(pref, "admin_pin", admin_pin);
  pref.end();
}
void save_admin_svc_state() {
  if (soft_clock_valid && soft_epoch_base > 0) {
    soft_epoch_base = soft_epoch_base + (int64_t)((millis() - soft_millis_base) / 1000UL);
    soft_millis_base = millis();
  }
  pref.begin("params", false);
  pref.putString("admin_pin", admin_pin);
  pref_put_int_if_changed(pref, "day_rem", day_lock_remain);
  for (uint8_t i = 0; i < 5; ++i) { char k[8]; snprintf(k, sizeof(k), "dl_t%u", i + 1); pref_put_int_if_changed(pref, k, day_lock_stage_days[i]); }
  pref_put_uchar_if_changed(pref, "dl_act", day_lock_active_stage);
  pref_put_bool_if_changed(pref, "dl_g10", day_lock_grace10_used);
  pref_put_bool_if_changed(pref, "dl_t1c", day_lock_t1_from_chain);
  pref.putBool("dev_lock", (bool)device_locked);
  pref.putLong64("soft_ep", soft_epoch_base);
  pref.putUInt("soft_ms", soft_millis_base);
  pref.putLong64("day_mid", day_lock_last_midnight);
  pref.putBool("clk_ok", soft_clock_valid);
  pref.putUInt("user_cuts", user_cuts);
  pref.end();
}

static void pref_copy_int(Preferences &src, Preferences &dst, const char *key) {
  if (src.isKey(key)) dst.putInt(key, src.getInt(key, 0));
}
static void pref_copy_uint(Preferences &src, Preferences &dst, const char *key) {
  if (src.isKey(key)) dst.putUInt(key, src.getUInt(key, 0));
}
static void pref_copy_ulong64(Preferences &src, Preferences &dst, const char *key) {
  if (src.isKey(key)) dst.putULong64(key, src.getULong64(key, 0));
}
static void pref_copy_float(Preferences &src, Preferences &dst, const char *key) {
  if (src.isKey(key)) dst.putFloat(key, src.getFloat(key, 0));
}
static void pref_copy_bool(Preferences &src, Preferences &dst, const char *key) {
  if (src.isKey(key)) dst.putBool(key, src.getBool(key, false));
}
static void pref_copy_uchar(Preferences &src, Preferences &dst, const char *key) {
  if (src.isKey(key)) dst.putUChar(key, src.getUChar(key, 0));
}
static void pref_copy_str(Preferences &src, Preferences &dst, const char *key) {
  if (src.isKey(key)) dst.putString(key, src.getString(key, ""));
}

// ذخیرهٔ تنظیمات عملیاتی به‌عنوان «تنظیمات کارخانه» (بدون لیست/شمارنده‌ها/قفل)
void save_factory_snapshot() {
  // اول RAM → params تا snapshot تازه باشد
  save_all();
  save_stock_kerf();
  save_jog_speed();
  save_cpu_temp_alarm();
  save_admin_pin();
  Preferences fac;
  pref.begin("params", true);
  fac.begin("factory", false);
  fac.clear();
  fac.putBool("f_ok", true);
  pref_copy_int(pref, fac, "glb_spd");
  pref_copy_int(pref, fac, "init_spd");
  pref_copy_int(pref, fac, "glb_slan");
  pref_copy_int(pref, fac, "init_pos");
  pref_copy_str(pref, fac, "final_pos");
  pref_copy_int(pref, fac, "final_posp");
  pref_copy_float(pref, fac, "ax_pch");
  pref_copy_float(pref, fac, "um_ppulse_f");
  pref_copy_int(pref, fac, "L1_GUI");
  pref_copy_int(pref, fac, "L2_GUI");
  pref_copy_bool(pref, fac, "buz_stat");
  pref_copy_bool(pref, fac, "vkb_en");
  pref_copy_uchar(pref, fac, "vkb_mode");
  pref_copy_bool(pref, fac, "diag_ov");
  pref_copy_bool(pref, fac, "js_en");
  pref_copy_int(pref, fac, "vis_raws");
  pref_copy_int(pref, fac, "cut_timer");
  pref_copy_int(pref, fac, "base_dir");
  pref_copy_bool(pref, fac, "stop_nc");
  pref_copy_int(pref, fac, "stock_len");
  pref_copy_float(pref, fac, "kerf");
  pref_copy_bool(pref, fac, "bld_err_en");
  pref_copy_float(pref, fac, "bld_err");
  pref_copy_int(pref, fac, "trim");
  pref_copy_int(pref, fac, "jog_spd");
  pref_copy_int(pref, fac, "jog_init");
  pref_copy_int(pref, fac, "auto_ramp");
  pref_copy_int(pref, fac, "jog_ramp");
  pref_copy_int(pref, fac, "f3_t_puls");
  pref_copy_int(pref, fac, "f3_t_per");
  pref_copy_bool(pref, fac, "prn_en");
  pref_copy_uchar(pref, fac, "prn_model");
  pref_copy_float(pref, fac, "cpu_talarm");
  pref_copy_int(pref, fac, "drv_ppr");
  pref_copy_float(pref, fac, "um_ppulse_f");
  pref_copy_str(pref, fac, "admin_pin");
  pref_copy_ulong64(pref, fac, "pm_in_v1");
  pref_copy_uint(pref, fac, "pm_out_v1");
  pref_copy_ulong64(pref, fac, "pm_in_v2");
  pref_copy_ulong64(pref, fac, "pm_out_v2");
  fac.end();
  pref.end();
}

bool factory_snapshot_exists() {
  Preferences fac;
  fac.begin("factory", true);
  bool ok = fac.getBool("f_ok", false);
  fac.end();
  return ok;
}

static void restore_factory_into_params() {
  Preferences fac;
  fac.begin("factory", true);
  pref.begin("params", false);
  pref.clear();
  pref_copy_int(fac, pref, "glb_spd");
  pref_copy_int(fac, pref, "init_spd");
  pref_copy_int(fac, pref, "glb_slan");
  pref_copy_int(fac, pref, "init_pos");
  pref_copy_str(fac, pref, "final_pos");
  pref_copy_int(fac, pref, "final_posp");
  pref_copy_float(fac, pref, "ax_pch");
  pref_copy_float(fac, pref, "um_ppulse_f");
  pref_copy_int(fac, pref, "L1_GUI");
  pref_copy_int(fac, pref, "L2_GUI");
  pref_copy_bool(fac, pref, "buz_stat");
  pref_copy_bool(fac, pref, "vkb_en");
  pref_copy_uchar(fac, pref, "vkb_mode");
  pref_copy_bool(fac, pref, "diag_ov");
  pref_copy_bool(fac, pref, "js_en");
  pref_copy_int(fac, pref, "vis_raws");
  pref_copy_int(fac, pref, "cut_timer");
  pref_copy_int(fac, pref, "base_dir");
  pref_copy_bool(fac, pref, "stop_nc");
  pref_copy_int(fac, pref, "stock_len");
  pref_copy_float(fac, pref, "kerf");
  pref_copy_bool(fac, pref, "bld_err_en");
  pref_copy_float(fac, pref, "bld_err");
  pref_copy_int(fac, pref, "trim");
  pref_copy_int(fac, pref, "jog_spd");
  pref_copy_int(fac, pref, "jog_init");
  pref_copy_int(fac, pref, "auto_ramp");
  pref_copy_int(fac, pref, "jog_ramp");
  pref_copy_int(fac, pref, "f3_t_puls");
  pref_copy_int(fac, pref, "f3_t_per");
  pref_copy_bool(fac, pref, "prn_en");
  pref_copy_uchar(fac, pref, "prn_model");
  pref_copy_float(fac, pref, "cpu_talarm");
  pref_copy_int(fac, pref, "drv_ppr");
  pref_copy_float(fac, pref, "um_ppulse_f");
  pref_copy_str(fac, pref, "admin_pin");
  pref_copy_ulong64(fac, pref, "pm_in_v1");
  pref_copy_uint(fac, pref, "pm_out_v1");
  pref_copy_ulong64(fac, pref, "pm_in_v2");
  pref_copy_ulong64(fac, pref, "pm_out_v2");
  // قفل روزشمار و ساعت نرم را ریست نکن / شمارنده عمر حفظ
  pref.putInt("day_rem", 0);
  for (uint8_t i = 1; i <= 5; ++i) { char k[8]; snprintf(k, sizeof(k), "dl_t%u", i); pref.putInt(k, 0); }
  pref.putUChar("dl_act", 0);
  pref.putBool("dev_lock", false);
  pref.putUInt("user_cuts", 0);
  pref.end();
  fac.end();
}

static void apply_hardcoded_factory_defaults() {
  pref.begin("params", false);
  pref.clear();
  pref.putInt("glb_spd", 50);
  pref.putInt("init_spd", 320);
  pref.putInt("glb_slan", 1);
  pref.putInt("drv_ppr", 6400);
  pref.putInt("init_pos", 60 * (int)(6400 / 20));
  pref.putString("final_pos", "6000");
  pref.putInt("final_posp", 6000 * (6400 / 20));
  pref.putFloat("ax_pch", 20);
  pref.putFloat("um_ppulse_f", 3.125f);
  pref.putInt("L1_GUI", 100);
  pref.putInt("L2_GUI", 60);
  pref.putBool("buz_stat", true);
  pref.putBool("vkb_en", true);
  pref.putUChar("vkb_mode", 0);  // AUTO: USB وصل → soft OFF / قطع → soft ON
  pref.putBool("diag_ov", false);
  pref.putBool("js_en", true);
  pref.putUChar("ss_mode", 2);   // اسکرین‌سیور کارخانه: ۳۰ دقیقه
  pref.putInt("vis_raws", 0);
  pref.putInt("cut_timer", 5000);
  pref.putInt("base_dir", 1);
  pref.putInt("f3_t_puls", 50);
  pref.putInt("f3_t_per", 25);
  pref.putBool("prn_en", false);
  pref.putUChar("prn_model", 0);
  pref.putFloat("cpu_talarm", 75.0f);
  pref.putBool("stop_nc", true);
  pref.putInt("stock_len", 6000);
  pref.putFloat("kerf", 3.0f);
  pref.putBool("bld_err_en", false);
  pref.putFloat("bld_err", 2.0f);
  pref.putInt("trim", 0);
  pref.putInt("jog_spd", 50);
  pref.putInt("jog_init", 320);
  pref.putInt("auto_ramp", 1000);
  pref.putInt("jog_ramp", 1000);
  pref.putString("admin_pin", "123456");
  pref.putBool("oobe_done", true);
  pref.putString("comp_name", "HAFFLER");
  pref.putInt("day_rem", 0);
  for (uint8_t i = 1; i <= 5; ++i) { char k[8]; snprintf(k, sizeof(k), "dl_t%u", i); pref.putInt(k, 0); }
  pref.putUChar("dl_act", 0);
  pref.putBool("dev_lock", false);
  pref.putUInt("user_cuts", 0);
  pref.end();
}

// ریست فکتوری: بازگشت به snapshot کارخانه (یا پیش‌فرض کد) — tot_cuts و گارانتی حفظ می‌شود
void factory_reset() {
  uint32_t keep_cuts = 0;
  bool keep_warr = false;
  char keep_warr_dt[24] = "";
  pref.begin("params", true);
  keep_cuts = pref.getUInt("tot_cuts", 0);
  keep_warr = pref.getBool("warr_on", false);
  {
    String wd = pref.getString("warr_dt", "");
    strncpy(keep_warr_dt, wd.c_str(), sizeof(keep_warr_dt) - 1);
  }
  pref.end();
  if (factory_snapshot_exists()) restore_factory_into_params();
  else apply_hardcoded_factory_defaults();
  pref.begin("params", false);
  pref.putUInt("tot_cuts", keep_cuts);
  pref.putBool("warr_on", keep_warr);
  pref.putString("warr_dt", keep_warr_dt);
  // اسکرین‌سیور کارخانه: ۳۰ دقیقه
  pref.putUChar("ss_mode", 2);
  for (uint8_t i = 1; i <= 5; ++i) { char k[8]; snprintf(k, sizeof(k), "dl_t%u", i); pref.putInt(k, 0); }
  pref.putUChar("dl_act", 0);
  pref.end();
  delay(200);
  ESP.restart();
}
void handle_stopstate_save_request() {
  if (!stopstate_save_request) return;
  stopstate_save_request = false;
  save_stopstate();
}
void recall_params() {
  pref.begin("params", false);
  const bool fresh_flash = !pref.isKey("glb_spd") || !pref.isKey("oobe_done");
  if (fresh_flash) {
    pref.end();
    apply_hardcoded_factory_defaults();
    if (!factory_snapshot_exists()) {
      save_factory_snapshot();
    }
    pref.begin("params", false);
  }
  STP.global_speed = constrain(pref.getInt("glb_spd", 50), 8, 100);
  STP.global_init_speed = constrain(pref.getInt("init_spd", 320), 40, 400);
  GUI.system_language = pref.getInt("glb_slan", 1);
  if (GUI.system_language != 1 && GUI.system_language != 2) GUI.system_language = 1;
  STP.drv_ppr = valid_drv_ppr_or_default(pref.getInt("drv_ppr", 6400));
  STP.Axis_Pitch = valid_axis_pitch_or_default(pref.getFloat("ax_pch", 20.0f));
  STP.um_per_pulse = pref.getFloat("um_ppulse_f", 0.0f);
  // Legacy releases stored a rounded integer (3 for an exact 3.125 µm).
  // Prefer the valid pitch/pulse geometry during migration to avoid a 4.17%
  // scale error; the legacy key is intentionally not promoted to exact data.
  if (!isfinite(STP.um_per_pulse) || STP.um_per_pulse <= 0.0001f)
    STP.um_per_pulse = (STP.Axis_Pitch * 1000.0f) / (float)STP.drv_ppr;
  const float init_ppmm = axisPulsesPerMm();
  STP.currentPosition_p = pref.getInt("lst_pos", (int)lroundf(60.0f * init_ppmm));
  STP.initial_Position_p = pref.getInt("init_pos", (int)lroundf(60.0f * init_ppmm));
  if (STP.initial_Position_p < 0) STP.initial_Position_p = 0;
  STP.final_position_mm = pref.getString("final_pos", "6000");
  if (STP.final_position_mm.length() == 0 || STP.final_position_mm.toFloat() < 100.0f || STP.final_position_mm.toFloat() > 20000.0f) {
    STP.final_position_mm = "6000";
  }
  STP.final_position_p = pref.getInt("final_posp", (int)lroundf(STP.final_position_mm.toFloat() * init_ppmm));
  if (STP.final_position_p < (int)lroundf(100.0f * init_ppmm)) {
    STP.final_position_p = (int)lroundf(STP.final_position_mm.toFloat() * init_ppmm);
  }
  GUI.L1_GUI = pref.getInt("L1_GUI", 100);
  GUI.L2_GUI = pref.getInt("L2_GUI", 60);
  GUI.buzzer_enable = pref.getBool("buz_stat", true);
  PEND.buzzer_enable = GUI.buzzer_enable;
  sys_volume = pref.getUChar("sys_vol", 80);
  if (sys_volume > 100) sys_volume = 80;
  alarm_volume = pref.getUChar("alrm_vol", 100);
  if (alarm_volume > 100) alarm_volume = 100;
  web_save_policy = (uint8_t)constrain(pref.getUChar("wsvp", 0), 0, 2);
  if (pref.isKey("vkb_mode")) GUI.vkb_mode = (uint8_t)constrain(pref.getUChar("vkb_mode", 0), 0, 2);
  else GUI.vkb_mode = pref.getBool("vkb_en", true) ? 0 : 2;
  GUI.virtual_kb_enable = (GUI.vkb_mode == 1) || (GUI.vkb_mode == 0);
  screensaver_timeout_mode = (uint8_t)constrain(pref.getUChar("ss_mode", 2), 0, 7);
  diagnostics_overlay_enabled = pref.getBool("diag_ov", false);
  GUI.joystick_enable = pref.getBool("js_en", true);
  PEND.joystick_enable = GUI.joystick_enable;
  printer_enabled = pref.getBool("prn_en", false);
  printer_model = (uint8_t)constrain(pref.getUChar("prn_model", 0), 0, 1);
  blade_err_enable = pref.getBool("bld_err_en", false);
  {
    float be = pref.getFloat("bld_err", 2.0f);
    be = roundf(be * 10.0f) / 10.0f;
    if (be > 9.9f) be = 9.9f;
    if (be < -9.9f) be = -9.9f;
    if (fabsf(be) < 0.1f) be = 2.0f;
    blade_err_mm = be;
  }
  blade_err_armed = false;
  PEND.blade_err_enable = blade_err_enable;
  PEND.blade_err_mm = blade_err_mm;

  GUI.visible_raws = pref.getInt("vis_raws", 0);
  IOS.cut_prcs_timer = pref.getInt("cut_timer", 5000);
  if (IOS.cut_prcs_timer < 0) IOS.cut_prcs_timer = 0;
  if (IOS.cut_prcs_timer > 60000) IOS.cut_prcs_timer = 60000;
  STP.system_base_dir = pref.getInt("base_dir", 1);
  stopstate = pref.getBool("stop_nc", true);
  cuts_life_manual = pref.getUInt("cl_man", 0);
  cuts_life_list = pref.getUInt("cl_lst", 0);
  cuts_life_opt = pref.getUInt("cl_opt", 0);
  {
    uint32_t sum = cuts_life_manual + cuts_life_list + cuts_life_opt;
    uint32_t legacy = pref.getUInt("tot_cuts", 0);
    // مهاجرت: اگر سه مد خالی ولی tot_cuts قدیمی بود، همان را نگه دار
    total_cuts = (sum > 0) ? sum : legacy;
    if (sum == 0 && legacy > 0) cuts_life_manual = legacy;
  }
  user_cuts = pref.getUInt("user_cuts", 0);
  cuts_day_manual = pref.getUInt("cd_man", 0);
  cuts_day_list = pref.getUInt("cd_lst", 0);
  cuts_day_opt = pref.getUInt("cd_opt", 0);
  cuts_day_key = pref.getUInt("cd_key", 0);
  blade_cuts = pref.getUInt("bld_cuts", 0);
  blade_limit = pref.getUInt("bld_lim", 5000);
  if (blade_limit < 100) blade_limit = 5000;
  blade_reminder_enabled = pref.getBool("bld_rem", true);
  {
    String bdt = pref.getString("bld_dt", "");
    strncpy(blade_service_date, bdt.c_str(), sizeof(blade_service_date) - 1);
    blade_service_date[sizeof(blade_service_date) - 1] = 0;
  }
  {
    float ta = pref.getFloat("cpu_talarm", 75.0f);
    if (ta < 45.0f) ta = 45.0f;
    if (ta > 105.0f) ta = 105.0f;
    cpu_temp_alarm_c = ta;
  }
  warranty_active = pref.getBool("warr_on", false);
  {
    String wd = pref.getString("warr_dt", "");
    strncpy(warranty_date, wd.c_str(), sizeof(warranty_date) - 1);
    warranty_date[sizeof(warranty_date) - 1] = 0;
    if (!warranty_active) warranty_date[0] = 0;
  }
  admin_pin = pref.getString("admin_pin", "123456");
  if (admin_pin.length() < 5) admin_pin = "123456";
  g_oobe_done = pref.getBool("oobe_done", false);
  g_company_name = pref.getString("comp_name", "HAFFLER");
  if (!g_oobe_done || g_company_name == "HAFFLER") {
    Preferences p;
    p.begin("chone", true);
    if (!g_oobe_done && p.isKey("AdAsk")) g_oobe_done = p.getBool("AdAsk", false);
    if (g_company_name == "HAFFLER" && p.isKey("CoName")) {
      String cn = p.getString("CoName", "HAFFLER");
      if (cn.length() > 0 && cn.length() <= 22) g_company_name = cn;
    }
    p.end();
  }
  if (g_company_name.length() == 0 || g_company_name.length() > 22) g_company_name = "HAFFLER";
  day_lock_remain = constrain(pref.getInt("day_rem", 0), 0, 365);
  for (uint8_t i = 0; i < 5; ++i) { char k[8]; snprintf(k, sizeof(k), "dl_t%u", i + 1); day_lock_stage_days[i] = constrain(pref.getInt(k, i == 0 ? day_lock_remain : 0), 0, i == 0 ? 365 : 50); }
  day_lock_active_stage = constrain(pref.getUChar("dl_act", day_lock_remain > 0 ? 1 : 0), 0, 5);
  day_lock_grace10_used = pref.getBool("dl_g10", false);
  day_lock_t1_from_chain = pref.getBool("dl_t1c", false);
  device_locked = pref.getBool("dev_lock", false);
  soft_epoch_base = pref.getLong64("soft_ep", 0);
  soft_clock_valid = pref.getBool("clk_ok", false) && (soft_epoch_base > 0);
  soft_millis_base = millis();  // ادامه از همین لحظه با epoch ذخیره‌شده
  day_lock_last_midnight = pref.getLong64("day_mid", 0);
  stock_len_mm = constrain(pref.getInt("stock_len", 6000), 10, 100000);
  // مهاجرت: قبلاً putInt("kerf") بود → اگر float نامعتبر بود از int بخوان
  {
    float kf = pref.getFloat("kerf", -999.0f);
    if (kf < 0.0f || kf > 50.0f) kf = (float)pref.getInt("kerf", 3);
    kerf_mm = constrain(kf, 0.0f, 50.0f);
  }
  trim_mm = constrain(pref.getInt("trim", 0), 0, 500);
  jog_speed_interval = constrain(STP.global_speed, 8, 100);  // همیشه هم‌سرعت با اتوماتیک
  jog_init_speed = constrain(pref.getInt("jog_init", STP.global_init_speed), 0, 400);
  if (jog_init_speed > 0 && jog_init_speed < 40) jog_init_speed = 40;
  auto_ramp_ms = constrain(pref.getInt("auto_ramp", 1000), 0, 5000);
  jog_ramp_ms = constrain(pref.getInt("jog_ramp", 1000), 0, 5000);
  f3_stepper_test_pulses = constrain(pref.getInt("f3_t_puls", 50), 10, 1000);
  f3_stepper_test_period_ms = constrain(pref.getInt("f3_t_per", 25), 10, 1000);
  GUI.visible_raws = constrain(GUI.visible_raws, 0, LIST_MAX);
  Serial.println("STP.system_base_dir" + String(STP.system_base_dir));
  if (STP.initial_Position_p < 0) STP.initial_Position_p = 0;
  if (GUI.L1_GUI <= 0) GUI.L1_GUI = 100;
  if (GUI.L2_GUI < 0) GUI.L2_GUI = 0;
  // مقصد بوت = موقعیت ذخیره‌شده با دقت ۰٫۱mm؛ پالسِ خام در lst_pos از قبل دقیق ذخیره شده است.
  {
    float ppmm = axisPulsesPerMm();
    if (ppmm < 1.0f) ppmm = 1.0f;
    float cur_mm = roundf(((float)STP.currentPosition_p / ppmm) * 10.0f) / 10.0f;
    const float lim_lo = (float)STP.initial_Position_p / ppmm;
    const float lim_hi = STP.final_position_mm.toFloat();
    if (cur_mm < lim_lo) cur_mm = lim_lo;
    if (cur_mm > lim_hi) cur_mm = lim_hi;
    char cur_buf[16];
    if (fabsf(cur_mm - roundf(cur_mm)) < 0.05f)
      snprintf(cur_buf, sizeof(cur_buf), "%d", (int)lroundf(cur_mm));
    else
      snprintf(cur_buf, sizeof(cur_buf), "%.1f", (double)cur_mm);
    GUI.inputBuffer_sp = cur_buf;
    STP.Man_Sp_mm = cur_mm;
    STP.position_Setpoint_p = (int)lroundf(cur_mm * ppmm);
    if (STP.currentPosition_p < STP.initial_Position_p || STP.currentPosition_p > STP.final_position_p) {
      STP.currentPosition_p = STP.position_Setpoint_p;
    }
  }
  // load screen3 data
  for (int i = 0; i < LIST_MAX; i++) {
    char key_sp[16];
    char key_count[20];
    snprintf(key_sp, sizeof(key_sp), "sp_val_%d", i);
    snprintf(key_count, sizeof(key_count), "count_val_%d", i);
    AUT.list_sp_Val_p[i] = pref.getFloat(key_sp, 0);
    AUT.list_count_Val[i] = pref.getInt(key_count, 0);
    {
      char key_base[20];
      snprintf(key_base, sizeof(key_base), "cnt_base_%d", i);
      int base = pref.getInt(key_base, -1);
      if (base < 0) base = AUT.list_count_Val[i];  // مهاجرت: پایه = تعداد فعلی
      if (base < AUT.list_count_Val[i]) base = AUT.list_count_Val[i];
      AUT.list_count_base_Val[i] = constrain(base, 0, 999);
    }
    // کلیدهای قدیمی با پیش‌فرض 1.1 و تعداد صفر → صفر واقعی
    if (AUT.list_count_Val[i] == 0 && AUT.list_sp_Val_p[i] >= 1.05f && AUT.list_sp_Val_p[i] <= 1.15f)
      AUT.list_sp_Val_p[i] = 0;
    {
      float sp = roundf(AUT.list_sp_Val_p[i] * 10.0f) / 10.0f;
      char b[16];
      if (fabsf(sp - roundf(sp)) < 0.05f) snprintf(b, sizeof(b), "%d", (int)lroundf(sp));
      else snprintf(b, sizeof(b), "%.1f", (double)sp);
      GUI.inputBuffer_L_3_1_x[i] = b;
    }
    GUI.inputBuffer_L_3_2_x[i] = String(AUT.list_count_Val[i]);
    {
      char key_ang[20];
      snprintf(key_ang, sizeof(key_ang), "ang_%d", i);
      uint8_t ang = pref.getUChar(key_ang, 45);
      if (ang != 90) ang = 45;
      AUT.list_angle_Val[i] = ang;
    }
  }
  // حافظهٔ مقصد F1..F5 (صفحهٔ اصلی)
  for (int i = 0; i < 5; i++) {
    char k[10], kv[12];
    snprintf(k, sizeof(k), "fmem%d", i + 1);
    snprintf(kv, sizeof(kv), "fmemv%d", i + 1);
    main_f_mem_valid[i] = pref.getBool(kv, false);
    main_f_mem_mm[i] = pref.getFloat(k, 0.0f);
    if (!main_f_mem_valid[i]) main_f_mem_mm[i] = 0.0f;
  }
  pref.end();
}
