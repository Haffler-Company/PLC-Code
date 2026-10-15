#pragma once
// Cut counters by mode (manual/list/opt) + daily — jog excluded

enum CutStatSrc : uint8_t {
  CUT_SRC_MANUAL = 0,
  CUT_SRC_LIST   = 1,
  CUT_SRC_OPT    = 2,
};

extern uint32_t cuts_life_manual;
extern uint32_t cuts_life_list;
extern uint32_t cuts_life_opt;
extern uint32_t cuts_day_manual;
extern uint32_t cuts_day_list;
extern uint32_t cuts_day_opt;
extern uint32_t cuts_day_key;

// Blade Life & Service Tracking
extern uint32_t blade_cuts;
extern uint32_t blade_limit;
extern bool blade_reminder_enabled;
extern char blade_service_date[24];

void cut_stats_load();
void cut_stats_save();
void cut_stats_ensure_day();
uint8_t cut_stats_detect_src();
void note_cut_event(uint8_t src);
void blade_service_reset(const char *date_str);
void blade_set_limit(uint32_t lim);
void blade_set_reminder(bool en);

static inline uint32_t cuts_life_sum() {
  return cuts_life_manual + cuts_life_list + cuts_life_opt;
}
static inline uint32_t cuts_day_sum() {
  return cuts_day_manual + cuts_day_list + cuts_day_opt;
}
static inline uint32_t blade_health_pct() {
  if (blade_limit == 0) return 100;
  if (blade_cuts >= blade_limit) return 0;
  return (uint32_t)(((uint64_t)(blade_limit - blade_cuts) * 100ULL) / blade_limit);
}