// task: task stop/start input readiang
#include "vars.h"
#define stst_pin 5
#define start1_pin 3  // I6
#define start2_pin 7  // I7
#define START_INPUT_ACTIVE_LEVEL LOW
#define START_DEBOUNCE_MS 30
#define START_DECISION_DELAY_MS 400
#define CLAMP_SETTLE_DELAY_MS 200
#define ANGLE_ROTATION_DELAY_MS 1500
#define BLADE_MOVE_TIMEOUT_MS 10000
#define SAW_IDLE_TIMEOUT_MS 300000
#define BLADE_GUARD_TIMEOUT_MS 5000
#define BLADE_LEAVE_TIMEOUT_MS 1500
#define BLADE_RETURN_TIMEOUT_MS 5000


volatile int analogSamples_stst = 0;
int stst_state = 0;  // 0 idle / 1 I2 guard / -1 I3 stop / 2 هر دو ورودی
int prev_stst_state = 0;
int temp_val_stst = 0;
// portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#include "cut_stats.h"
static bool prev_start1_active = false;
static bool prev_start2_active = false;
static bool start_edge_initialized = false;
static bool q3_latched = false;
static bool cut_cycle_active = false;
static bool cut_ready_for_confirm = false;
static bool spray_off_pending = false;
static unsigned long cut_wait_started_ms = 0;
static unsigned long cut_wait_duration_ms = 0;
static unsigned long spray_off_started_ms = 0;
static unsigned long start_decision_started_ms = 0;
static bool blade_guard_fault_latched = false;
static bool blade_guard_wait_active = false;
static unsigned long blade_guard_wait_started_ms = 0;
static uint8_t start_decision_mode = 0;  // 0 idle, 1 pending, 2 start1, 3 start2, 4 both, 5 wait release
static bool start1_raw_last = false;
static bool start2_raw_last = false;
static bool start1_physical_stable = false;
static bool start2_physical_stable = false;
static bool start1_stable = false;
static bool start2_stable = false;
static unsigned long start1_raw_changed_ms = 0;
static unsigned long start2_raw_changed_ms = 0;
static uint8_t start_release_pulse = 0;  // فشردن کوتاه تک‌کلید در پنجرهٔ تشخیص دوکلیده
static uint8_t start_pending_single = 0;
static bool panel_nav_start_suppressed = false;
static bool motor_fault_emg_latched = false;
static bool cut_start_rearm_required = false;
static bool cut_wrong_screen_notice_latched = false;
enum CutPhase : uint8_t {
  CUT_IDLE = 0,
  CUT_HOLDER_ENGAGED,
  CUT_CLAMP_SETTLE,
  CUT_WAIT_SAW_DELAY,
  CUT_WAIT_BLADE_LEAVE,
  CUT_BLADE_DOWN,
  CUT_WAIT_BLADE_RETURN,
  CUT_ROTATING_TO_NEG45,
  CUT_STAGE2_BLADE_UP,
  CUT_STAGE2_BLADE_DOWN,
  CUT_STAGE2_WAIT_RETURN,
  CUT_READY_CONFIRM,
  CUT_FAULT_WAIT_RETURN,
  CUT_FAULT_LATCHED
};
static CutPhase cut_phase = CUT_IDLE;
static uint32_t cut_phase_started_ms = 0;
static bool cut_fault_after_return = false;
static bool blade_return_timeout_latched = false;
static uint8_t cut_notice_last = CUT_UI_NOTICE_NONE;
static uint32_t cut_notice_last_ms = 0;

static uint8_t cut_notice_priority(uint8_t notice) {
  switch (notice) {
    case CUT_UI_NOTICE_HOLDER_ENGAGED:
    case CUT_UI_NOTICE_BLADE_RETURNING:
    case CUT_UI_NOTICE_BLADE_RETURNED:
    case CUT_UI_NOTICE_FAULT_RECOVERED:
      return 1;
    case CUT_UI_NOTICE_NONE:
      return 0;
    default:
      return 2;
  }
}

// [RT/Task8] Coalesces repeated held-button faults before Task2 renders a toast.
static void cut_request_notice(uint8_t notice) {
  if (notice == CUT_UI_NOTICE_NONE) return;
  const uint32_t now = millis();
  const uint8_t pending = cut_safety_ui_notice_req;
  const bool replaces_lower_priority =
    pending != CUT_UI_NOTICE_NONE && cut_notice_priority(notice) > cut_notice_priority(pending);
  if (pending != CUT_UI_NOTICE_NONE && !replaces_lower_priority) return;
  if (!replaces_lower_priority && notice == cut_notice_last &&
      (uint32_t)(now - cut_notice_last_ms) < 2500U) return;
  cut_notice_last = notice;
  cut_notice_last_ms = now;
  cut_safety_ui_notice_req = notice;
}

void read_dual_start_inputs() {
  const bool raw_start1 = (digitalRead(start1_pin) == START_INPUT_ACTIVE_LEVEL);
  const bool raw_start2 = (digitalRead(start2_pin) == START_INPUT_ACTIVE_LEVEL);
  unsigned long now_ms = millis();

  if (raw_start1 != start1_raw_last) {
    start1_raw_last = raw_start1;
    start1_raw_changed_ms = now_ms;
  }
  if (raw_start2 != start2_raw_last) {
    start2_raw_last = raw_start2;
    start2_raw_changed_ms = now_ms;
  }
  if ((now_ms - start1_raw_changed_ms) >= START_DEBOUNCE_MS)
    start1_physical_stable = start1_raw_last;
  if ((now_ms - start2_raw_changed_ms) >= START_DEBOUNCE_MS)
    start2_physical_stable = start2_raw_last;

  // Debounce متعلق به ترمینال فیزیکی است؛ بعد از پایدارشدن، نقش نگاشت‌شده آن را می‌خواند.
  pinmap_in_publish_one(6, start1_physical_stable);
  pinmap_in_publish_one(7, start2_physical_stable);
  start1_stable = pinmap_in_role_active(6);
  start2_stable = pinmap_in_role_active(7);

  bool any_start = start1_stable || start2_stable;
  bool both_starts = start1_stable && start2_stable;

  if (any_start) f3_pinmap_request_external_close();

  IOS.start1_button_state = false;
  IOS.start2_button_state = false;
  IOS.both_start_button_state = false;
  IOS.start_button_state = false;
  cut_start_inputs_released = !any_start;

  // ورود به Main با کلید نگه داشته هرگز نباید همان فشار قبلی را به برش تبدیل کند.
  if (cut_start_rearm_required) {
    if (!any_start) {
      cut_start_rearm_required = false;
      start_decision_mode = 0;
      start_pending_single = 0;
      start_release_pulse = 0;
      prev_start1_active = false;
      prev_start2_active = false;
      if (cut_phase == CUT_FAULT_LATCHED) cut_phase = CUT_IDLE;
      start_edge_initialized = true;
    }
    return;
  }

  // استارت‌های ۱/۲ جزو سناریوی پنج‌کلیدی نیستند: در MODE فقط از حالت انتخاب
  // خارج می‌شوند و همان فشار نباید گیره/سیکل برش را راه بیندازد.
  if (panel_five_nav_is_active()) {
    if (any_start) {
      GUI.flag_panel_nav_exit_key = true;
      panel_nav_start_suppressed = true;
    }
    if (!any_start) {
      start_decision_mode = 0;
      start_pending_single = 0;
      start_release_pulse = 0;
    }
    return;
  }
  if (panel_nav_start_suppressed) {
    if (!any_start) panel_nav_start_suppressed = false;
    return;
  }

  if (!any_start) {
    // کلید پنل ممکن است قبل از پایان پنجرهٔ 250ms رها شود. رویداد کوتاه
    // را یک دور نگه دار تا START گم نشود؛ دوکلیده همچنان فقط با هر دو کلید فعال است.
    if (start_decision_mode == 1) {
      start_release_pulse = start_pending_single;
    }
    start_decision_mode = 0;
    start_pending_single = 0;
    if (start_release_pulse == 1) {
      IOS.start1_button_state = true;
      IOS.start_button_state = true;
      start_release_pulse = 0;
    } else if (start_release_pulse == 2) {
      IOS.start2_button_state = true;
      IOS.start_button_state = true;
      start_release_pulse = 0;
    }
    return;
  }

  if (start_decision_mode == 0) {
    start_decision_mode = 1;
    start_pending_single = (start1_stable && !start2_stable) ? 1 :
                           (start2_stable && !start1_stable) ? 2 : 0;
    start_decision_started_ms = now_ms;
    return;
  }

  if (start_decision_mode == 1) {
    if (both_starts) {
      start_decision_mode = 4;
      start_pending_single = 0;
    } else if ((now_ms - start_decision_started_ms) < START_DECISION_DELAY_MS) {
      return;
    }

    if (both_starts) {
      start_decision_mode = 4;
    } else if (start1_stable) {
      start_decision_mode = 2;
    } else if (start2_stable) {
      start_decision_mode = 3;
    }
  }

  switch (start_decision_mode) {
    case 2:
      if (start1_stable && !start2_stable) {
        IOS.start1_button_state = true;
        IOS.start_button_state = true;
      } else {
        start_decision_mode = 5;
      }
      break;
    case 3:
      if (start2_stable && !start1_stable) {
        IOS.start2_button_state = true;
        IOS.start_button_state = true;
      } else {
        start_decision_mode = 5;
      }
      break;
    case 4:
      if (both_starts) {
        IOS.start1_button_state = true;
        IOS.start2_button_state = true;
        IOS.both_start_button_state = true;
        IOS.start_button_state = true;
      } else {
        start_decision_mode = 5;
      }
      break;
    default:
      break;
  }
}


static uint32_t cut_stats_today_key() {
  int y = 0, m = 0, d = 0, hh = 0, mi = 0, ss = 0;
  int64_t now = soft_now_epoch();
  if (now <= 0) return 0;
  epoch_to_civil(now, &y, &m, &d, &hh, &mi, &ss);
  if (y < 2000) return 0;
  return (uint32_t)(y * 10000u + (uint32_t)m * 100u + (uint32_t)d);
}

void cut_stats_ensure_day() {
  uint32_t key = cut_stats_today_key();
  if (key == 0) return;
  if (cuts_day_key != key) {
    cuts_day_key = key;
    cuts_day_manual = 0;
    cuts_day_list = 0;
    cuts_day_opt = 0;
    save_cut_counters();
  }
}

uint8_t cut_stats_detect_src() {
  if (OPT.active) return CUT_SRC_OPT;
  if (AUT.selected_count_val >= 0 || GUI.scr_id == 3) return CUT_SRC_LIST;
  return CUT_SRC_MANUAL;
}

void note_cut_event(uint8_t src) {
  static uint32_t last_ms = 0;
  uint32_t now = millis();
  if (last_ms != 0 && (now - last_ms) < 1200u) return;
  last_ms = now;
  if (src > 2) src = CUT_SRC_MANUAL;
  cut_stats_ensure_day();
  total_cuts++;
  user_cuts++;
  blade_cuts++;
  if (src == CUT_SRC_LIST) { cuts_life_list++; cuts_day_list++; }
  else if (src == CUT_SRC_OPT) { cuts_life_opt++; cuts_day_opt++; }
  else { cuts_life_manual++; cuts_day_manual++; }
  save_cut_counters();
}

void cut_stats_load() {}

static uint8_t cut_active_angle() {
  if (AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX) {
    uint8_t a = AUT.list_angle_Val[AUT.selected_count_val];
    return (a == 90) ? 90 : 45;
  }
  if (OPT.active && OPT.cur < OPT.piece_count) {
    int b = OPT.piece_bar[OPT.cur];
    if (b >= 0 && b < OPT_MAX_BARS && OPT.bar_angle[b] == 90) return 90;
    return 45;
  }
  return (pinmap_out_read(IOS.lamp_pin) == LOW) ? 90 : 45;
}

void confirm_cut_cycle() {
  cut_ready_for_confirm = false;
  q3_latched = false;
  cut_phase = CUT_IDLE;
  cut_start_rearm_required = true;
  blade_guard_wait_active = false;
  pinmap_out_write(IOS.u_clip_pin, HIGH);
  pinmap_out_write(IOS.f_clip_pin, HIGH);
  pinmap_out_write(IOS.sawl_ift_pin, LOW); // reset angle to +45° (Q8 off)
  pinmap_out_write(IOS.q9_pin, LOW);
  if (cut_active_angle() != 90) {
    pinmap_out_write(IOS.lamp_pin, HIGH); // reset 90°
  }

  // CL-MAIN-04: سایز برشِ تمام‌شده را قبل از پیشروی صف ذخیره کن
  {
    float done_mm = STP.Man_Sp_mm;
    if (OPT.active && OPT.cur < OPT.piece_count) done_mm = OPT.piece_len[OPT.cur];
    cut_done_notify_mm = done_mm;
    cut_done_notify_req = true;
  }

  main_consume_active_queue_piece(true);

  snprintf(buff_currentPosition_mm, sizeof(buff_currentPosition_mm), "%d",
           (int)round((float)STP.currentPosition_p / (STP.drv_ppr / STP.Axis_Pitch)));
  // در مود بهینه‌سازی، لیبل پرینت شمارهٔ شاخه را هم دارد (ردیابی قطعات — الگوی کارگاهی)
  if (OPT.active && OPT.cur > 0 && OPT.cur <= OPT.piece_count) {
    int pi = OPT.cur - 1;
    snprintf(buff_currentPosition_mm, sizeof(buff_currentPosition_mm), "%d B%d",
             (int)OPT.piece_len[pi], OPT.piece_bar[pi] + 1);
  }
  GUI.flag_update_lable1_2 = true;
  IOS.cut_print_request = true;
  note_cut_event(cut_stats_detect_src());
}

void update_upper_clamp_from_starts() {
  if (GUI.scr_id != 1) return;  // گیره و تأیید برش فقط در صفحه اصلی
  if (cut_phase == CUT_FAULT_WAIT_RETURN || cut_phase == CUT_FAULT_LATCHED) return;
  bool start1 = IOS.start1_button_state;
  bool start2 = IOS.start2_button_state;

  if (!start_edge_initialized) {
    prev_start1_active = start1;
    prev_start2_active = start2;
    start_edge_initialized = true;
  }

  if (IOS.stop_button_state) {
    q3_latched = false;
    pinmap_out_write(IOS.u_clip_pin, HIGH);
  } else {
    bool start1_falling_edge = start1 && !prev_start1_active;
    bool start2_falling_edge = start2 && !prev_start2_active;

    if ((start1_falling_edge || start2_falling_edge) && (start1 ^ start2)) {
      // راهنما/مودال باز: فقط ببند — استارت بعدی کار کند
      // اگر مودال بهینه گیر کرده، استارت هم فلگ لغو را می‌زند تا قفل باز شود
      if (GUI.lock_screen) {
        GUI.flag_abort_opt_calc = true;
        GUI.buzzer_cmd_long = true;
        prev_start1_active = start1;
        prev_start2_active = start2;
        return;
      }
      // استارت ۱/۲ فقط سناریوی برش (گیره / تأیید برش) — حرکت استپر با کلید START پنل آنالوگ است
      if (cut_ready_for_confirm && cut_phase == CUT_READY_CONFIRM) {
        confirm_cut_cycle();
      } else if (cut_phase == CUT_IDLE) {
        q3_latched = !q3_latched;
        pinmap_out_write(IOS.u_clip_pin, q3_latched ? LOW : HIGH);
      }
    }
  }

  prev_start1_active = start1;
  prev_start2_active = start2;
}

static bool cut_required_io_mapped() {
  const int required_inputs[] = {2, 3, 4, 5, 6, 7};
  const int required_outputs[] = {3, 4, 5, 6, 8};
  for (uint8_t i = 0; i < sizeof(required_inputs) / sizeof(required_inputs[0]); ++i)
    if (!pinmap_in_role_connected(required_inputs[i])) return false;
  for (uint8_t i = 0; i < sizeof(required_outputs) / sizeof(required_outputs[0]); ++i)
    if (pinmap_out_get_source(required_outputs[i]) == PINMAP_OUT_NC) return false;
  return true;
}

static bool cut_blade_top() {
  return pinmap_in_role_active(5); // I5 is now blade top limit sensor
}

static uint32_t saw_last_active_ms = 0;
static void update_saw_idle_auto_off() {
  const uint32_t now = millis();
  if (IOS.saw_is_on) {
    if (cut_cycle_active || STP.flag_is_cutting) {
      saw_last_active_ms = now;
    } else {
      if (saw_last_active_ms == 0) saw_last_active_ms = now;
      if ((uint32_t)(now - saw_last_active_ms) >= SAW_IDLE_TIMEOUT_MS) {
        pinmap_out_write(IOS.saw_pin, HIGH);
        IOS.saw_is_on = false;
        GUI.flag_uncheck_saw_btns = true;
        saw_last_active_ms = 0;
      }
    }
  } else {
    saw_last_active_ms = 0;
  }
}

static bool cut_blade_home() {
  return pinmap_in_role_connected(4) && IOS.blade_return_state;
}

// [RT/Task8] Never release a held workpiece until I4 positively reports blade return.
static void cut_release_clamps_if_home() {
  if (!cut_blade_home()) return;
  q3_latched = false;
  pinmap_out_write(IOS.u_clip_pin, HIGH);
  pinmap_out_write(IOS.f_clip_pin, HIGH);
}

static void cut_process_outputs_safe() {
  pinmap_out_write(IOS.spray_pin, HIGH);
  pinmap_out_write(IOS.saw_pin, HIGH);
  pinmap_out_write(IOS.sawl_ift_pin, HIGH);
  IOS.saw_is_on = false;
  GUI.flag_uncheck_saw_btns = true;
}

void stop_active_cut_cycle(bool release_guard = false) {
  cut_cycle_active = false;
  STP.flag_is_cutting = false;
  spray_off_pending = false;
  blade_guard_wait_active = false;
  cut_process_outputs_safe();
  if (release_guard) cut_release_clamps_if_home();
}

// [RT/Task8] Leaving Main is safe only after the blade is known home.
static void cancel_cut_for_non_main_screen() {
  const bool process_owned = cut_cycle_active || cut_ready_for_confirm || q3_latched ||
                             spray_off_pending || STP.flag_is_cutting || IOS.saw_is_on ||
                             IOS.clamp_engaged || IOS.cut_confirm_pending;
  if (!process_owned) return;
  stop_active_cut_cycle(true);
  cut_ready_for_confirm = false;
  cut_phase = cut_blade_home() ? CUT_IDLE : CUT_FAULT_WAIT_RETURN;
  cut_fault_after_return = !cut_blade_home();
  cut_start_rearm_required = true;
  IOS.clamp_engaged = q3_latched;
  IOS.cut_confirm_pending = false;
}

static void cut_begin_return(bool fault_after_return, uint8_t notice) {
  blade_return_timeout_latched = false;
  pinmap_out_write(IOS.sawl_ift_pin, HIGH);  // Return command is immediate on either START release.
  pinmap_out_write(IOS.saw_pin, HIGH);
  IOS.saw_is_on = false;
  GUI.flag_uncheck_saw_btns = true;
  if (fault_after_return) {
    spray_off_pending = false;
    pinmap_out_write(IOS.spray_pin, HIGH);
  } else if (pinmap_out_read(IOS.spray_pin) == LOW) {
    spray_off_pending = true;
    spray_off_started_ms = millis();
  }
  cut_cycle_active = true;
  STP.flag_is_cutting = false;
  cut_ready_for_confirm = false;
  cut_fault_after_return = fault_after_return;
  cut_phase = fault_after_return ? CUT_FAULT_WAIT_RETURN : CUT_WAIT_BLADE_RETURN;
  cut_phase_started_ms = millis();
  if (notice != CUT_UI_NOTICE_NONE) cut_request_notice(notice);
}

void stop_cut_after_start_release() {
  if (cut_phase == CUT_HOLDER_ENGAGED || cut_phase == CUT_WAIT_SAW_DELAY) {
    stop_active_cut_cycle(true);
    cut_ready_for_confirm = false;
    cut_phase = CUT_IDLE;
    cut_start_rearm_required = true;
    cut_request_notice(CUT_UI_NOTICE_BLADE_RETURNING);
    return;
  }
  if (cut_phase == CUT_WAIT_BLADE_LEAVE || cut_phase == CUT_BLADE_DOWN)
    cut_begin_return(false, CUT_UI_NOTICE_BLADE_RETURNING);
}

void update_delayed_spray_off() {
  if (spray_off_pending && (millis() - spray_off_started_ms) >= 2000) {
    spray_off_pending = false;
    pinmap_out_write(IOS.spray_pin, HIGH);
  }
}

static void clear_start_inputs_for_stop() {
  IOS.start1_button_state = false;
  IOS.start2_button_state = false;
  IOS.both_start_button_state = false;
  IOS.start_button_state = false;
  start_decision_mode = 0;
}

static void force_estop_outputs_safe() {
  clear_start_inputs_for_stop();
  if (f3_stepper_test_busy || f3_stepper_test_cmd) {
    f3_stepper_test_stop_fault = true;
    f3_stepper_test_abort = true;
  }
  STP.jogEnabled = false;
  STP.moveStepperCMD = false;
  STP.goref_cmd = false;
  STP.go_sampling_CMD = false;
  STP.mvm_interupt = true;
  keyboardJogActive = false;
  // قطع پالس + EN درایور (HIGH=خاموش) — نه فقط فلگ
  stepperStopMotion(true);
  // اگر مودال بهینه گیر کرده، استوپ پنل قفل UI را هم باز کند تا استارت دوباره کار کند
  GUI.flag_abort_opt_calc = true;
  GUI.flag_close_numpad = true;
  stop_active_cut_cycle(true);
  cut_ready_for_confirm = false;
  cut_start_rearm_required = true;
  if (cut_blade_home()) {
    cut_phase = CUT_IDLE;
    cut_release_clamps_if_home();
  } else {
    cut_phase = CUT_FAULT_WAIT_RETURN;
    cut_phase_started_ms = millis();
    cut_fault_after_return = true;
  }
  pinmap_out_write(IOS.lamp_pin, HIGH);    // Q7 off
  pinmap_out_write(IOS.sawl_ift_pin, LOW); // Q8 off (45°)
  pinmap_out_write(IOS.q9_pin, LOW);        // Q9 off (90°)
  IOS.saw_is_on = false;
  IOS.clamp_engaged = q3_latched;
  IOS.cut_confirm_pending = false;
  GUI.flag_uncheck_saw_btns = true;
  GUI.flag_close_numpad = true;  // استوپ پنل → بستن نامپد مقصد/لیست
}

// STOP آنالوگ پنل (Task10/Task2) — همان مسیر ایمنی استوپ فیزیکی
void panel_request_stop_motion(void) {
  force_estop_outputs_safe();
}

void update_cut_cycle_from_starts() {
  update_saw_idle_auto_off();
  if (GUI.scr_id != 1) return;
  const bool both_starts = IOS.both_start_button_state;
  const uint32_t now = millis();

  // I2 is supervised throughout every state which can move the blade.
  if (cut_phase >= CUT_HOLDER_ENGAGED && cut_phase <= CUT_STAGE2_WAIT_RETURN &&
      !IOS.guard_switch_state) {
    cut_start_rearm_required = true;
    cut_begin_return(true, CUT_UI_NOTICE_GUARD_OPEN);
    return;
  }

  // Handle blade return waiting
  if (cut_phase == CUT_WAIT_BLADE_RETURN || cut_phase == CUT_STAGE2_WAIT_RETURN || cut_phase == CUT_FAULT_WAIT_RETURN) {
    if (cut_blade_home()) {
      blade_return_timeout_latched = false;

      // If Stage 1 of a 45° cut completed, rotate cylinder to -45°
      if (cut_phase == CUT_WAIT_BLADE_RETURN && !cut_fault_after_return && cut_active_angle() == 45) {
        // Rotate angle cylinder Q8 to -45° (HIGH = active)
        pinmap_out_write(IOS.sawl_ift_pin, HIGH);
        pinmap_out_write(IOS.q9_pin, LOW);
        cut_phase = CUT_ROTATING_TO_NEG45;
        cut_phase_started_ms = now;
        return;
      }

      // Stage 2 or 90° completed
      pinmap_out_write(IOS.saw_pin, HIGH);
      IOS.saw_is_on = false;
      GUI.flag_uncheck_saw_btns = true;
      const bool was_cutting = cut_cycle_active;
      cut_cycle_active = false;

      if (!cut_fault_after_return) {
        cut_phase = CUT_READY_CONFIRM;
        cut_ready_for_confirm = true;
        if (was_cutting) cut_request_notice(CUT_UI_NOTICE_BLADE_RETURNED);
      } else {
        cut_release_clamps_if_home();
        pinmap_out_write(IOS.sawl_ift_pin, LOW); // reset angle to +45° (Q8 off)
        pinmap_out_write(IOS.q9_pin, LOW);       // reset 90° (Q9 off)
        if (cut_active_angle() != 90) pinmap_out_write(IOS.lamp_pin, HIGH); // reset 90°
        cut_ready_for_confirm = false;
        cut_phase = cut_start_inputs_released ? CUT_IDLE : CUT_FAULT_LATCHED;
        cut_start_rearm_required = true;
        if (was_cutting) cut_request_notice(CUT_UI_NOTICE_FAULT_RECOVERED);
      }
      return;
    }

    // 10s Timeout check
    if (!blade_return_timeout_latched &&
        (uint32_t)(now - cut_phase_started_ms) >= BLADE_MOVE_TIMEOUT_MS) {
      blade_return_timeout_latched = true;
      cut_fault_after_return = true;
      cut_phase = CUT_FAULT_WAIT_RETURN;
      cut_start_rearm_required = true;
      cut_request_notice(CUT_UI_NOTICE_BLADE_RETURN_TIMEOUT);
    }
    return;
  }

  // Handle angle rotation to -45°
  if (cut_phase == CUT_ROTATING_TO_NEG45) {
    if ((uint32_t)(now - cut_phase_started_ms) >= ANGLE_ROTATION_DELAY_MS) {
      if (both_starts) {
        // Operator is still holding starts -> execute second cut at -45°
        pinmap_out_write(IOS.sawl_ift_pin, LOW);
        pinmap_out_write(IOS.spray_pin, LOW);
        cut_phase = CUT_STAGE2_BLADE_UP;
        cut_phase_started_ms = now;
      }
    }
    return;
  }

  // If user releases dual start during blade motion, immediately return blade down
  if (!both_starts) {
    if (cut_phase == CUT_WAIT_BLADE_LEAVE || cut_phase == CUT_BLADE_DOWN) {
      pinmap_out_write(IOS.sawl_ift_pin, HIGH); // bring blade down
      cut_phase = CUT_WAIT_BLADE_RETURN;
      cut_phase_started_ms = now;
    } else if (cut_phase == CUT_STAGE2_BLADE_UP) {
      pinmap_out_write(IOS.sawl_ift_pin, HIGH); // bring blade down
      cut_phase = CUT_STAGE2_WAIT_RETURN;
      cut_phase_started_ms = now;
    } else if (cut_cycle_active) {
      stop_cut_after_start_release();
    }
    return;
  }

  if (cut_phase == CUT_READY_CONFIRM) {
    // If operator holds dual start again at confirmation stage -> allow blade extension
    pinmap_out_write(IOS.sawl_ift_pin, LOW);
    return;
  }

  if (cut_phase == CUT_FAULT_LATCHED) return;
  if (IOS.stop_button_state) { cut_request_notice(CUT_UI_NOTICE_STOP); return; }
  if (device_locked) { cut_request_notice(CUT_UI_NOTICE_DEVICE_LOCKED); return; }
  if (GUI.lock_screen) { cut_request_notice(CUT_UI_NOTICE_UI_LOCKED); return; }
  if (!cut_required_io_mapped()) { cut_request_notice(CUT_UI_NOTICE_PINMAP); return; }
  if (!IOS.guard_switch_state) { cut_request_notice(CUT_UI_NOTICE_GUARD_OPEN); return; }
  if (!cut_blade_home()) { cut_request_notice(CUT_UI_NOTICE_BLADE_NOT_HOME); return; }

  // 1. Initial trigger: engage clamp
  if (cut_phase == CUT_IDLE) {
    const uint8_t cur_angle = cut_active_angle();
    if (cur_angle == 90) {
      pinmap_out_write(IOS.lamp_pin, LOW);  // Q7 active: releases 45° cylinder for manual 90°
      if (!cut_angle_90_confirmed) {
        cut_angle_90_prompt_req = true;
        cut_start_rearm_required = true;
        cut_request_notice(CUT_UI_NOTICE_ANGLE_90_CONFIRM_NEEDED);
        return;
      }
    } else {
      pinmap_out_write(IOS.lamp_pin, HIGH); // Q7 inactive: 45° cylinder in circuit
      cut_angle_90_confirmed = false;
    }

    cut_cycle_active = true;
    STP.flag_is_cutting = true;
    cut_ready_for_confirm = false;
    cut_fault_after_return = false;
    blade_return_timeout_latched = false;
    sensor_trip_which = 0;
    if (sensor_alert_motion_lock || sensor_alert_is_open()) sensor_alert_close_now();
    limit_recal_flag = false;
    ref_wrong_dir_flag = false;
    q3_latched = true;

    // Angle setup (45°: Q8 LOW, 90°: Q9 HIGH)
    if (cur_angle == 90) {
      pinmap_out_write(IOS.sawl_ift_pin, LOW);
      pinmap_out_write(IOS.q9_pin, HIGH);
    } else {
      pinmap_out_write(IOS.sawl_ift_pin, LOW);
      pinmap_out_write(IOS.q9_pin, LOW);
    }

    pinmap_out_write(IOS.u_clip_pin, LOW);  // Q3 clamp engaged
    cut_phase = CUT_CLAMP_SETTLE;
    cut_phase_started_ms = now;
    cut_request_notice(CUT_UI_NOTICE_HOLDER_ENGAGED);
    return;
  }

  // 2. 200ms clamp pressure settle delay
  if (cut_phase == CUT_CLAMP_SETTLE && (uint32_t)(now - cut_phase_started_ms) >= CLAMP_SETTLE_DELAY_MS) {
    pinmap_out_write(IOS.f_clip_pin, LOW);

    // Check if saw motor was already running
    if (IOS.saw_is_on) {
      cut_wait_duration_ms = 0; // No warmup delay if already running
    } else {
      pinmap_out_write(IOS.saw_pin, LOW);
      IOS.saw_is_on = true;
      GUI.flag_check_saw_btns = true;
      cut_wait_duration_ms = max(IOS.cut_prcs_timer, 0);
    }

    cut_wait_started_ms = now;
    cut_phase = CUT_WAIT_SAW_DELAY;
    return;
  }

  // 3. Saw warmup delay -> raise blade for stage 1 cut
  if (cut_phase == CUT_WAIT_SAW_DELAY && (uint32_t)(now - cut_wait_started_ms) >= cut_wait_duration_ms) {
    pinmap_out_write(IOS.sawl_ift_pin, LOW); // Blade lift UP
    pinmap_out_write(IOS.spray_pin, LOW);    // Spray ON
    cut_phase = CUT_WAIT_BLADE_LEAVE;
    cut_phase_started_ms = now;
    return;
  }

  // 4. Stage 1 Blade travel UP: check top sensor I5 or timeout
  if (cut_phase == CUT_WAIT_BLADE_LEAVE || cut_phase == CUT_BLADE_DOWN) {
    if (cut_blade_top() || (uint32_t)(now - cut_phase_started_ms) >= 3000) {
      // Reached top sensor -> bring blade down
      pinmap_out_write(IOS.sawl_ift_pin, HIGH); // Blade DOWN
      cut_phase = CUT_WAIT_BLADE_RETURN;
      cut_phase_started_ms = now;
      return;
    }

    // 10s Timeout check
    if ((uint32_t)(now - cut_phase_started_ms) >= BLADE_MOVE_TIMEOUT_MS) {
      cut_start_rearm_required = true;
      cut_begin_return(true, CUT_UI_NOTICE_BLADE_LEAVE_TIMEOUT);
    }
    return;
  }

  // 5. Stage 2 Blade travel UP (-45° cut): check top sensor I5
  if (cut_phase == CUT_STAGE2_BLADE_UP) {
    if (cut_blade_top() || (uint32_t)(now - cut_phase_started_ms) >= 3000) {
      // Reached top sensor -> bring blade down
      pinmap_out_write(IOS.sawl_ift_pin, HIGH); // Blade DOWN
      cut_phase = CUT_STAGE2_WAIT_RETURN;
      cut_phase_started_ms = now;
      return;
    }

    // 10s Timeout check
    if ((uint32_t)(now - cut_phase_started_ms) >= BLADE_MOVE_TIMEOUT_MS) {
      cut_start_rearm_required = true;
      cut_begin_return(true, CUT_UI_NOTICE_BLADE_LEAVE_TIMEOUT);
    }
    return;
  }
}
void average_stst() {
  analogSamples_stst = 0;
  for (int i = 0; i < 10; i++) {
    if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(20)) == pdTRUE) {
      temp_val_stst = analogRead(stst_pin);
      xSemaphoreGive(adcSemaphore);
    }
    analogSamples_stst = analogSamples_stst + temp_val_stst;
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  analogSamples_stst = analogSamples_stst / 10;
}


void read_input_stst() {
  if (!stopstate) {  // normally open
    average_stst();
    if (!((analogSamples_stst > 1820) && (analogSamples_stst < 2300))) {
      vTaskDelay(40 / portTICK_PERIOD_MS);
      average_stst();
      if (!((analogSamples_stst > 1820) && (analogSamples_stst < 2300))) {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        if (analogSamples_stst > 1500 && analogSamples_stst < 1820) {
          stst_state = 1;  // I2 guard switch
          return;
        } else if (analogSamples_stst > 2300 && analogSamples_stst < 3000) {
          stst_state = 2;  // بازهٔ مستقل سخت‌افزار برای I2+I3
          return;
        } else if (analogSamples_stst > 3800 && analogSamples_stst < 4100) {
          stst_state = -1;  // I3 stop
          return;
        }
      }
    }
    stst_state = 0;
    return;
  }

  // normally closed
  average_stst();
  if (!((analogSamples_stst > 3800) && (analogSamples_stst < 4100))) {
    vTaskDelay(30 / portTICK_PERIOD_MS);
    average_stst();
    if (!((analogSamples_stst > 3800) && (analogSamples_stst < 4100))) {
      vTaskDelay(30 / portTICK_PERIOD_MS);
      average_stst();
      if (!((analogSamples_stst > 3800) && (analogSamples_stst < 4100))) {
        if (analogSamples_stst > 1500 && analogSamples_stst < 2100) {
          stst_state = -1;  // I3 stop
          return;
        } else if (analogSamples_stst > 2100 && analogSamples_stst < 3000) {
          stst_state = 1;  // I2 guard switch
          return;
        }
      }
    }
  }
  stst_state = 0;
}

void determine_inputs_stst() {
  prev_stst_state = stst_state;
  pinmap_in_publish_pair(2, stst_state == 1 || stst_state == 2,
                         3, stst_state == -1 || stst_state == 2);
  const bool mapped_stop = pinmap_in_role_active(3);
  const bool mapped_guard = pinmap_in_role_active(2);
  const bool stop_rising = mapped_stop && !IOS.stop_button_state;
  const bool stop_falling = !mapped_stop && IOS.stop_button_state;
  IOS.stop_button_state = mapped_stop;
  IOS.guard_switch_state = mapped_guard;

  if (mapped_stop) {
      if (stop_rising) {
      if (STP.motorIs_moving || STP.moveStepperCMD || keyboardJogActive || STP.jogEnabled)
        flag_panel_stop_motion_toast = true;
       force_estop_outputs_safe();
       IOS.start_button_state = false;
       stop_active_cut_cycle(true);
       IOS.saw_is_on = false;
       GUI.flag_uncheck_saw_btns = true;
       cut_request_notice(pinmap_in_role_connected(3) ? CUT_UI_NOTICE_STOP : CUT_UI_NOTICE_PINMAP);
      // در تنظیمات: تغییر NC/NO ممکن است استاپ را لحظه‌ای «فعال» نشان دهد؛
      // اسپلش استاپ + lock_screen جلوی دوباره زدن کلید استاپ را می‌گیرد.
      if (GUI.scr_id == 2) {
        GUI.flag_del_emg = true;
      } else {
        GUI.flag_cast_emg = true;
      }
      }
  } else if (stop_falling) {
    GUI.flag_del_emg = true;
  }
}

void Task8(void *pvParameters) {
  vTaskDelay(500 / portTICK_PERIOD_MS);
  Serial.println("task8 start");
  pinMode(stst_pin, INPUT);
  pinMode(start1_pin, INPUT);
  pinMode(start2_pin, INPUT);
  while (1) {
    // ایمنی: در جریانِ بروزرسانیِ F5-بوت، ورودی‌های start/stop خوانده نمی‌شوند تا
    // هیچ سیکلِ برشی شروع نشود (خروجی‌ها در init بالا OFF شده‌اند).
    if (g_update_mode) { vTaskDelay(50 / portTICK_PERIOD_MS); continue; }
    read_input_stst();
    vTaskDelay(1 / portTICK_PERIOD_MS);
    determine_inputs_stst();
    read_dual_start_inputs();
    if (IOS.stop_button_state) {
      force_estop_outputs_safe();
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    // خطای موتور (I5): توقف ایمنی فوری — مثل استوپ
    if (IOS.motor_fault_state) {
      force_estop_outputs_safe();
      stop_active_cut_cycle(true);
      pinmap_out_write(IOS.saw_pin, HIGH);
      pinmap_out_write(IOS.sawl_ift_pin, HIGH);
      pinmap_out_write(IOS.spray_pin, HIGH);
      IOS.saw_is_on = false;
      GUI.flag_uncheck_saw_btns = true;
      if (!motor_fault_emg_latched) {
        motor_fault_emg_latched = true;
        cut_request_notice(CUT_UI_NOTICE_MOTOR_FAULT);
        GUI.flag_cast_emg = true;
        GUI.buzzer_cmd_long = true;
      }
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    if (motor_fault_emg_latched) {
      motor_fault_emg_latched = false;
      GUI.flag_del_emg = true;
    }
    if (GUI.scr_id != 1) {
      const bool any_cut_start = !cut_start_inputs_released;
      if (any_cut_start) {
        cut_start_rearm_required = true;
        if (!cut_wrong_screen_notice_latched) {
          cut_wrong_screen_notice_latched = true;
          cut_start_ui_notice_req = 1;
          GUI.buzzer_cmd_long = true;
        }
      } else {
        cut_wrong_screen_notice_latched = false;
      }
      cancel_cut_for_non_main_screen();
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    if (cut_start_inputs_released) cut_wrong_screen_notice_latched = false;
    update_upper_clamp_from_starts();
    update_cut_cycle_from_starts();
    update_delayed_spray_off();
    IOS.clamp_engaged = q3_latched;               // برای گارد ورود به تنظیمات
    IOS.cut_confirm_pending = cut_ready_for_confirm;
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
