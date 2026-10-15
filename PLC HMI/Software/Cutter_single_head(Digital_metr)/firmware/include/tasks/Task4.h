// task: usb keyboard handler
// *************************************************************************
// خط وحشتناک قرمز مالک: بدون دستور صریح جداگانه با نام «کیبرد USB»
// این فایل / EspUsbHost را ادیت نکن. soft-KB ≠ اجازهٔ دست زدن به اینجا.
// قانون: .cursor/rules/06-usb-keyboard-redline.mdc
// *************************************************************************

#include "EspUsbHostKeybord.h"
#include "usb/usb_host.h"

unsigned long lastKeyPressTime = 0;
const unsigned long debounceDelay = 50;
uint8_t prevKeyCode = 0;
// keyboardJogActive در main.ino تعریف شده

struct UsbKeyboardUiEvent {
  uint8_t ascii;
  uint8_t keycode;
  uint8_t modifier;
};
static const uint8_t USB_KEYBOARD_UI_QUEUE_LEN = 32;
static QueueHandle_t usb_keyboard_ui_queue = NULL;
static StaticQueue_t usb_keyboard_ui_queue_meta;
static uint8_t usb_keyboard_ui_queue_storage[
    USB_KEYBOARD_UI_QUEUE_LEN * sizeof(UsbKeyboardUiEvent)];
static volatile uint32_t usb_keyboard_ui_queue_drops = 0;

static void usb_keyboard_ui_queue_init() {
  if (usb_keyboard_ui_queue) return;
  usb_keyboard_ui_queue = xQueueCreateStatic(
      USB_KEYBOARD_UI_QUEUE_LEN, sizeof(UsbKeyboardUiEvent),
      usb_keyboard_ui_queue_storage, &usb_keyboard_ui_queue_meta);
}

static bool usb_keyboard_ui_enqueue(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
  if (!usb_keyboard_ui_queue) return false;
  const UsbKeyboardUiEvent ev = {ascii, keycode, modifier};
  if (xQueueSend(usb_keyboard_ui_queue, &ev, 0) == pdTRUE) return true;
  usb_keyboard_ui_queue_drops++;
  return false;
}

static void usb_keyboard_ui_queue_reset() {
  if (usb_keyboard_ui_queue) xQueueReset(usb_keyboard_ui_queue);
}

static bool isKeyboardJogLeft(uint8_t key_code) {
  return key_code == 0x5C || key_code == 0x50;
}

static bool isKeyboardJogRight(uint8_t key_code) {
  return key_code == 0x5E || key_code == 0x4F;
}

// کلید ۲ (پد یا ردیف بالا) = سرعت آهستهٔ جاگ
static bool isKeyboardJogFine(uint8_t key_code) {
  return key_code == 0x5A || key_code == 0x1F;
}

// پیش‌بینی توقف: فقط به اندازهٔ سرعت فعلی (نه کل ۵ثانیه وقتی هنوز آروم است)
static float jog_predict_decel_mm(int cur_interval_us, int stop_interval_us, int cruise_interval_us, int ramp_ms) {
  if (ramp_ms <= 0) return 0.0f;
  int a = constrain(cur_interval_us, 8, JOG_TIMER_MAX_INTERVAL_US);
  int b = constrain(stop_interval_us, 8, JOG_TIMER_MAX_INTERVAL_US);
  int c = constrain(cruise_interval_us, 8, JOG_TIMER_MAX_INTERVAL_US);
  float R0 = 500000.0f / (float)a;
  float R1 = 500000.0f / (float)b;
  float Rc = 500000.0f / (float)c;
  if (R0 <= R1 + 1.0f) return 0.0f;
  // زمان توقف ≈ نسبت پیشرفت شتاب به شیب کل
  float frac = (Rc > R1 + 1.0f) ? (R0 - R1) / (Rc - R1) : 1.0f;
  frac = constrain(frac, 0.0f, 1.0f);
  float T = ((float)ramp_ms / 1000.0f) * frac;
  if (T < 0.001f) return 0.0f;
  float pulses = T * (R0 - R1) / logf(R0 / R1);
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm < 1.0f) ppmm = 1.0f;
  return pulses / ppmm;
}

// مقصد بکاپ عوض‌جهت: عدد UI، ولی فقط داخل توقف فیزیکی — وگرنه موتور تا ابد می‌خزد
static float jog_compute_backup_mm() {
  float mm = (float)STP.currentPosition_p / (STP.drv_ppr / STP.Axis_Pitch);
  int cruise_iv = constrain(STP.global_speed, 8, 100);
  int configured_stop_iv = constrain(jog_init_speed, cruise_iv, 400);
  int stop_iv = (jog_ramp_ms > 0)
                  ? max(configured_stop_iv, (int)jog_live_interval_us)
                  : (int)jog_live_interval_us;
  float ahead = jog_predict_decel_mm((int)jog_live_interval_us, stop_iv, cruise_iv, jog_ramp_ms);
  int phys = (STP.system_set_dir * STP.system_base_dir >= 0) ? 1 : -1;
  float stop_mm = mm + (float)phys * ahead;
  float ui = STP.Man_Sp_mm;
  float bak;
  if (phys > 0) {
    bak = ui;
    if (bak > stop_mm) bak = stop_mm;  // جلوتر از توقف فیزیکی نرو
    if (bak < mm) bak = mm;            // عقب‌تر از موتور = همین‌جا وایسا
  } else {
    bak = ui;
    if (bak < stop_mm) bak = stop_mm;
    if (bak > mm) bak = mm;
  }
  return bak;
}

static void jog_write_dest_whole(int whole) {
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm < 1.0f) ppmm = 1.0f;
  int lim_lo = (int)lroundf((float)STP.initial_Position_p / ppmm);
  int lim_hi = (int)lroundf((float)STP.final_position_p / ppmm);
  if (whole < lim_lo) whole = lim_lo;
  if (whole > lim_hi) whole = lim_hi;
  STP.Man_Sp_mm = (float)whole;
  STP.position_Setpoint_p = (int)lroundf((float)whole * ppmm);
  // String فقط در Task2 — از Task11 رها کردن جویستیک با update_lable1_1 رقابت نکند
  jog_dest_buf_whole = whole;
  jog_dest_buf_pending = true;
  GUI.update_lable1_1 = true;
}

void jog_snap_dest_to_motor() {
  float mm = (float)STP.currentPosition_p / (STP.drv_ppr / STP.Axis_Pitch);
  jog_write_dest_whole((int)lroundf(mm));
  jog_grow_active = false;
}

static void jog_capture_forward_resume_dest() {
  if (jog_forward_resume_dest_valid) return;
  jog_forward_resume_dest_mm = STP.Man_Sp_mm;
  jog_forward_resume_dest_valid = true;
}

void jog_freeze_target_at_current() {
  // محاسبهٔ نهایی فقط یک‌بار، دقیقاً روی لبهٔ رها؛ سپس قفل مطلق.
  if (jog_grow_active) {
    // اصلاح مقصد حین رسیدن: همان عدد رشدکرده از مقصد قبلی را قفل کن؛
    // از موقعیت دورِ موتور دوباره نساز.
    int whole = (int)lroundf(STP.Man_Sp_mm);
    jog_write_dest_whole(whole);
    jog_release_seek_pending = true;
    jog_backup_active = false;
    jog_grow_active = false;
    jog_soft_restart = false;
    jog_keep_dest = true;
    jog_forward_resume_dest_valid = false;
    GUI.update_lable1_1 = true;
    return;
  }
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm < 1.0f) ppmm = 1.0f;
  float mm = (float)STP.currentPosition_p / ppmm;
  int cruise_iv = constrain(STP.global_speed, 8, 100);
  int configured_stop_iv = constrain(jog_init_speed, cruise_iv, 400);
  int stop_iv = (jog_ramp_ms > 0)
                  ? max(configured_stop_iv, (int)jog_live_interval_us)
                  : (int)jog_live_interval_us;
  float ahead = jog_predict_decel_mm((int)jog_live_interval_us, stop_iv,
                                     cruise_iv, jog_ramp_ms);
  int physical_dir = (STP.system_set_dir * STP.system_base_dir >= 0) ? 1 : -1;
  float stopped_mm = mm + (float)physical_dir * ahead;
  if (physical_dir > 0 && jog_forward_resume_dest_valid &&
      jog_forward_resume_dest_mm > stopped_mm) {
    stopped_mm = jog_forward_resume_dest_mm;
  }
  int whole = (physical_dir > 0) ? (int)ceilf(stopped_mm) : (int)floorf(stopped_mm);
  jog_write_dest_whole(whole);
  jog_release_seek_pending = true;
  jog_backup_active = false;
  jog_grow_active = false;
  jog_soft_restart = false;
  jog_keep_dest = true;
  jog_forward_resume_dest_valid = false;
  GUI.update_lable1_1 = true;
}

// تک‌استارت فیزیکی روی اصلی = همان Enter کیبورد (حرکت / توقف نرم جاگ)
void main_mirror_keyboard_enter_on_main() {
  if (GUI.scr_id != 1) return;
  if (GUI.lock_screen || sensor_alert_motion_lock) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (IOS.stop_button_state) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  // در جاگ: مثل Enter فقط توقف نرم — از جاگ خارج نشو
  if (GUI.NumLock) {
    if (keyboardJogActive || STP.jogEnabled) {
      STP.jogEnabled = false;
      keyboardJogActive = false;
      jog_freeze_target_at_current();
      GUI.buzzer_cmd_short = true;
    }
    return;
  }
  if (keyboardJogActive) {
    STP.jogEnabled = false;
    keyboardJogActive = false;
    jog_freeze_target_at_current();
  }
  STP.jogEnabled = false;
  keyboard_pending_enter = true;
  GUI.buzzer_cmd_short = true;
}

// جویستیک بدون NumLock فقط موتور را می‌برد؛ مقصد/کلید۲/چراغ جاگ کار نمی‌کند.
// ورود خودکار به همان حالت جاگ کیبرد (NumLock) — بدون toast هر بار.
void jog_ensure_numlock_mode() {
  if (GUI.NumLock) return;
  if (!STP.referenced || GUI.scr_id != 1) return;
  if (STP.flag_is_cutting || STP.begin_calibration) return;
  GUI.NumLock = true;
  jog_numlock_from_joystick = true;  // فقط مسیر جویستیک
  GUI.edit_mode_lable1_1 = false;
  jog_capture_forward_resume_dest();
  jog_snap_dest_to_motor();
  jog_keep_dest = false;
  AUT.selected_count_val = -1;
  GUI.update_lable1_3 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  clear_buffer_sp_flag = true;
  GUI.update_lable1_1 = true;
  usb_keyboard_set_numlock_led(false);  // جاگ = چراغ خاموش
}

// ورود/ادامهٔ جاگ — کیبورد و جویستیک
void jog_apply_engage(int new_dir) {
  const bool was_jog_enabled = STP.jogEnabled;
  const bool moving = STP.motorIs_moving && (movement_mode == 2 || movement_mode == 5);
  const bool active_seek = STP.motorIs_moving && movement_mode == 1;
  const bool seeking = STP.moveStepperCMD || active_seek;
  const bool retargeting_frozen_motion = (moving || seeking) && !was_jog_enabled;
  const bool dir_flip = (STP.system_set_dir != new_dir);
  const int previous_wish = jog_wish_dir;
  const int net = (new_dir * STP.system_base_dir >= 0) ? 1 : -1;
  const bool soft = (jog_ramp_ms > 0);
  const bool reverse_seek_handoff = active_seek && dir_flip && soft;
  jog_seek_speed_handoff = active_seek && !dir_flip && soft;

  if (!was_jog_enabled) jog_capture_forward_resume_dest();

  // وسط seek بعد از رها: قطع کن تا جاگ همان‌جهت فوری بگیرد
  if (seeking) {
    STP.mvm_interupt = true;
    STP.moveStepperCMD = false;
  }

  jog_wish_dir = new_dir;
  STP.jogEnabled = true;
  STP.in_position_flag = false;
  jog_release_seek_pending = false;
  jog_reverse_stop_active = false;

  if (retargeting_frozen_motion) {
    // موتور هنوز به مقصد قفل‌شدهٔ قبلی می‌رود؛ جاگ تازه باید مقصد را
    // از همان عدد قبلی کم/زیاد کند، نه از موقعیت فعلی موتور.
    jog_dest_backup_mm = STP.Man_Sp_mm;
    jog_grow_base_mm = STP.Man_Sp_mm;
    jog_grow_active = true;
    jog_grow_t0 = millis();
    jog_grow_peak_whole = (int)lroundf(jog_grow_base_mm);
    jog_keep_dest = true;
    jog_pred_dir_ms = 0;
  }

  if (dir_flip) {
    // عوض‌جهت حین حرکت: مقصد از عدد فعلی در جهت کلید رشد می‌کند؛
    // جهت فیزیکی فقط بعد از توقف تطبیقی و رسیدن/عبور مقصد عوض می‌شود.
    jog_soft_restart = false;
    if (moving || reverse_seek_handoff) {
      if (!retargeting_frozen_motion && previous_wish != new_dir) {
        jog_dest_backup_mm = STP.Man_Sp_mm;
        jog_grow_base_mm = STP.Man_Sp_mm;
        jog_grow_active = true;
        jog_grow_t0 = millis();
        jog_grow_peak_whole = (int)lroundf(jog_grow_base_mm);
        jog_keep_dest = true;
        jog_pred_dir_ms = 0;
      }
      jog_backup_active = true;
    } else {
      jog_backup_active = false;
      if (!retargeting_frozen_motion) jog_grow_active = false;
    }
    if (!moving && !seeking) {
      jog_snap_dest_to_motor();
      jog_keep_dest = false;
    }
  } else {
    // همان جهت / استارت: مقصد زنده همیشه = موتور + پیش‌بینی توقف (Task2).
    jog_backup_active = false;
    if (!retargeting_frozen_motion) jog_grow_active = false;
    jog_pred_dir_ms = 0;
    if (!jog_keep_dest) {
      jog_snap_dest_to_motor();
    }
    if (!moving && !active_seek) {
      jog_live_interval_us = (jog_ramp_ms > 0)
                               ? constrain(jog_init_speed, STP.global_speed, 400)
                               : constrain(STP.global_speed, 8, 100);
    }
    // همان‌جهت وسط کاهش سرعت: از سرعت جاری ادامه بده؛ شروع دوباره از init
    // همان مکث محسوس بعد از رها/گرفتن سریع را ایجاد می‌کرد.
    jog_soft_restart = soft && !moving && !active_seek;
  }

  // ساکن/همان‌جهت/آنی/قطع‌شده از seek: DIR فوری
  // seek فعالِ خلاف‌جهت باید DIR فیزیکی قبلی را تا تحویل به jogMotor نگه دارد.
  if (!reverse_seek_handoff && (!moving || !dir_flip || !soft || seeking)) {
    STP.system_set_dir = new_dir;
  }
  if (dir_flip) {
    jog_pred_dir = net;
    jog_pred_dir_ms = soft ? millis() : 0;
  } else {
    jog_pred_dir = net;
    if (jog_grow_active) jog_pred_dir_ms = 0;
  }
  GUI.update_lable1_1 = true;
}

static void jog_engage_dir(int new_dir) {
  jog_apply_engage(new_dir);
  keyboardJogActive = true;
}

static void keyboardStopJog() {
  // همیشه پرچم جاگ کیبرد را پایین بکش — حتی اگر فلگ داخلی از sync خارج شده باشد
  STP.jogEnabled = false;
  const bool was_kb = keyboardJogActive;
  keyboardJogActive = false;
  if (was_kb && GUI.NumLock && STP.referenced && GUI.scr_id == 1)
    jog_freeze_target_at_current();
}

// تست محدود روی برد بدون موتور: عادی → دقیق → عادی → رها.
// فقط با JQT فعال می‌شود، از همان مسیر واقعی جاگ عبور می‌کند و خودکار متوقف می‌شود.
static void prcs_jog_remote_test() {
  const uint8_t action = jog_remote_test_action;
  if (action != 0) {
    jog_remote_test_action = 0;
    const bool blocked = GUI.scr_id != 1 || !STP.referenced || device_locked ||
                         GUI.lock_screen || sensor_alert_motion_lock ||
                         IOS.stop_button_state || STP.flag_is_cutting ||
                         STP.begin_calibration || main_queue_mode_active();
    if (blocked && action != 5) {
      Serial0.println("JRC BLOCK");
      return;
    }
    if (action == 1 || action == 2) {
      jog_ensure_numlock_mode();
      jog_numlock_from_joystick = false;
      jog_engage_dir(action == 1 ? 1 : -1);
      jog_remote_test_active = true;
    } else if (action == 3 && jog_remote_test_active) {
      jog_fine_held = true;
    } else if (action == 4) {
      jog_fine_held = false;
    } else if (action == 5) {
      jog_fine_held = false;
      if (keyboardJogActive || STP.jogEnabled) keyboardStopJog();
      jog_remote_test_active = false;
      jog_remote_test_deadline_ms = 0;
    }
  }
  if (jog_remote_test_active && jog_remote_test_deadline_ms != 0 &&
      (int32_t)(millis() - jog_remote_test_deadline_ms) >= 0) {
    jog_fine_held = false;
    if (keyboardJogActive || STP.jogEnabled) keyboardStopJog();
    jog_remote_test_active = false;
    jog_remote_test_deadline_ms = 0;
    Serial0.println("JRC WATCHDOG_STOP");
  }
}

void prcs_jog_motion_test() {
  static uint8_t stage = 0;
  static uint32_t stage_t0 = 0;
  static uint32_t last_log_ms = 0;
  static uint32_t phase_ms = 1500;
  static uint8_t rapid_cycle = 0;
  static bool transition_seen = false;
  static float frozen_dest = 0.0f;

  prcs_jog_remote_test();
  if (jog_remote_test_active) return;

  if (jog_motion_test_request) {
    jog_motion_test_request = false;
    if (stage != 0) {
      Serial0.println("JOGT BUSY");
    } else {
      stage = 1;
      stage_t0 = millis();
      last_log_ms = 0;
      rapid_cycle = 0;
      transition_seen = false;
    }
  }
  if (stage == 0) return;

  const bool blocked = GUI.scr_id != 1 || !STP.referenced || device_locked ||
                       GUI.lock_screen || sensor_alert_motion_lock ||
                       IOS.stop_button_state || STP.flag_is_cutting ||
                       STP.begin_calibration || main_queue_mode_active();
  if (blocked) {
    jog_fine_held = false;
    if (keyboardJogActive || STP.jogEnabled) keyboardStopJog();
    Serial0.printf("JOGT ABORT scr=%d ref=%d lock=%d stop=%d cut=%d\n",
                   GUI.scr_id, STP.referenced ? 1 : 0,
                   (device_locked || GUI.lock_screen || sensor_alert_motion_lock) ? 1 : 0,
                   IOS.stop_button_state ? 1 : 0, STP.flag_is_cutting ? 1 : 0);
    stage = 0;
    return;
  }

  const uint32_t now = millis();
  if (stage == 1) {
    if (GUI.ui_screen_settle > 0) return;
    jog_ensure_numlock_mode();
    jog_numlock_from_joystick = false;
    jog_fine_held = false;
    jog_engage_dir(1);
    phase_ms = (uint32_t)constrain(jog_ramp_ms + 750, 1500, 6000);
    stage = 2;
    stage_t0 = now;
    Serial0.printf("JOGT START speed=%d init=%d ramp=%d phase=%lu\n",
                   STP.global_speed, jog_init_speed, jog_ramp_ms, (unsigned long)phase_ms);
  } else if (stage == 2 && (uint32_t)(now - stage_t0) >= phase_ms) {
    jog_fine_held = true;
    stage = 3;
    stage_t0 = now;
    Serial0.println("JOGT FINE");
  } else if (stage == 3 && (uint32_t)(now - stage_t0) >= phase_ms) {
    jog_fine_held = false;
    stage = 4;
    stage_t0 = now;
    Serial0.println("JOGT NORMAL");
  } else if (stage == 4 && (uint32_t)(now - stage_t0) >= phase_ms) {
    jog_fine_held = false;
    keyboardStopJog();
    stage = 5;
    stage_t0 = now;
    frozen_dest = STP.Man_Sp_mm;
    Serial0.printf("JOGT RELEASE1 dest=%.3f\n", (double)frozen_dest);
  } else if (stage == 5 && (uint32_t)(now - stage_t0) >= 500u) {
    transition_seen = false;
    jog_engage_dir(1);
    stage = 6;
    stage_t0 = now;
    const float test_ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
    const float test_pos_mm = (test_ppmm > 0.0f)
                                ? ((float)STP.currentPosition_p / test_ppmm) : 0.0f;
    Serial0.printf("JOGT REGRAB_SAME gap=%.3f\n",
                   (double)(frozen_dest - test_pos_mm));
  } else if (stage == 6) {
    if (!transition_seen && STP.motorIs_moving && movement_mode == 2) {
      transition_seen = true;
      Serial0.printf("JOGT REGRAB_SAME_OK latency=%lu\n",
                     (unsigned long)(now - stage_t0));
    }
    if (!transition_seen && (uint32_t)(now - stage_t0) >= 1500u) {
      Serial0.println("JOGT FAIL same-direction regrab blocked");
      keyboardStopJog();
      stage = 0;
      return;
    }
    if ((uint32_t)(now - stage_t0) >= phase_ms) {
      keyboardStopJog();
      stage = 7;
      stage_t0 = now;
      frozen_dest = STP.Man_Sp_mm;
      Serial0.printf("JOGT RELEASE2 dest=%.3f\n", (double)frozen_dest);
    }
  } else if (stage == 7 && (uint32_t)(now - stage_t0) >= 500u) {
    transition_seen = false;
    jog_engage_dir(-1);
    stage = 8;
    stage_t0 = now;
    Serial0.println("JOGT REGRAB_REVERSE");
  } else if (stage == 8) {
    if (!transition_seen && STP.motorIs_moving && movement_mode == 2 &&
        STP.system_set_dir == -1) {
      transition_seen = true;
      Serial0.printf("JOGT REVERSE_OK latency=%lu\n",
                     (unsigned long)(now - stage_t0));
      stage = 9;
      stage_t0 = now;
    } else if (!transition_seen && (uint32_t)(now - stage_t0) >= phase_ms * 2u) {
      Serial0.println("JOGT FAIL reverse blocked");
      keyboardStopJog();
      stage = 0;
      return;
    }
  } else if (stage == 9 && (uint32_t)(now - stage_t0) >= phase_ms) {
    jog_fine_held = true;
    stage = 10;
    stage_t0 = now;
    Serial0.println("JOGT REVERSE_FINE");
  } else if (stage == 10 && (uint32_t)(now - stage_t0) >= phase_ms) {
    jog_fine_held = false;
    stage = 11;
    stage_t0 = now;
    Serial0.println("JOGT REVERSE_NORMAL");
  } else if (stage == 11 && (uint32_t)(now - stage_t0) >= phase_ms) {
    keyboardStopJog();
    rapid_cycle = 0;
    stage = 12;
    stage_t0 = now;
    Serial0.println("JOGT RAPID_RELEASE");
  } else if (stage == 12 && (uint32_t)(now - stage_t0) >= 300u) {
    transition_seen = false;
    jog_fine_held = (rapid_cycle == 1);
    jog_engage_dir(-1);
    stage = 13;
    stage_t0 = now;
    Serial0.printf("JOGT RAPID_REGRAB cycle=%u fine=%d\n",
                   rapid_cycle + 1, jog_fine_held ? 1 : 0);
  } else if (stage == 13) {
    if (!transition_seen && STP.motorIs_moving && movement_mode == 2) {
      transition_seen = true;
      Serial0.printf("JOGT RAPID_OK cycle=%u latency=%lu\n",
                     rapid_cycle + 1, (unsigned long)(now - stage_t0));
    }
    if (!transition_seen && (uint32_t)(now - stage_t0) >= 1200u) {
      Serial0.printf("JOGT FAIL rapid cycle=%u\n", rapid_cycle + 1);
      jog_fine_held = false;
      keyboardStopJog();
      stage = 0;
      return;
    }
    if ((uint32_t)(now - stage_t0) >= 1000u) {
      jog_fine_held = false;
      keyboardStopJog();
      rapid_cycle++;
      stage_t0 = now;
      if (rapid_cycle < 3) {
        stage = 12;
      } else {
        stage = 14;
        Serial0.printf("JOGT FINAL_RELEASE dest=%.3f\n", (double)STP.Man_Sp_mm);
      }
    }
  } else if (stage == 14 &&
             ((!STP.motorIs_moving && !STP.moveStepperCMD) ||
              (uint32_t)(now - stage_t0) >= phase_ms * 2u)) {
    jog_fine_held = false;
    STP.jogEnabled = false;
    keyboardJogActive = false;
    Serial0.printf("JOGT DONE pos=%ld dest=%.3f moving=%d\n",
                   (long)STP.currentPosition_p, (double)STP.Man_Sp_mm,
                   STP.motorIs_moving ? 1 : 0);
    stage = 0;
    return;
  }

  if (last_log_ms == 0 || (uint32_t)(now - last_log_ms) >= 250u) {
    last_log_ms = now;
    const float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
    const float pos_mm = (ppmm > 0.0f) ? ((float)STP.currentPosition_p / ppmm) : 0.0f;
    Serial0.printf("JOGT S%u t=%lu iv=%d fine=%d jog=%d moving=%d pos=%.3f dest=%.3f\n",
                   stage, (unsigned long)(now - stage_t0), (int)jog_live_interval_us,
                   jog_fine_held ? 1 : 0, STP.jogEnabled ? 1 : 0,
                   STP.motorIs_moving ? 1 : 0, (double)pos_mm, (double)STP.Man_Sp_mm);
  }
}

static void keyboardAddDigit(String &buffer, char digit) {
  if (buffer.length() < 6) buffer += digit;
}

static void keyboardAddCountDigit(String &buffer, char digit) {
  if (buffer.length() < 3) buffer += digit;
}

static void keyboardHandleF1Digit(char digit) {
  if (!f1_num_entry_active()) return;
  if (settings_clear_on_type) {
    settings_num_buf = "";
    settings_clear_on_type = false;
  }
  if (digit == '.') {
    if (numpad_ctx != 4) return;
    if (settings_num_buf.indexOf('.') >= 0) return;
    if (settings_num_buf.length() == 0) settings_num_buf = "0";
  }
  int lim = (numpad_ctx == 3) ? 6 : 5;
  if ((int)settings_num_buf.length() < lim) settings_num_buf += digit;
  GUI.flag_sync_stock_kerf = true;
  f1_caret_lit = true;
  f1_caret_blink_ms = millis();
}

static void keyboardHandleF1Backspace() {
  if (!f1_num_entry_active()) return;
  settings_clear_on_type = false;
  if (settings_num_buf.length()) {
    settings_num_buf.remove(settings_num_buf.length() - 1);
    GUI.flag_sync_stock_kerf = true;
  }
}

static void keyboardHandleF1Commit() {
  if (!f1_num_entry_active()) return;
  GUI.flag_sync_stock_kerf = false;
  f1_num_entry_commit();
}

static void keyboardHandleDigit(char digit) {
  // قفل دستگاه روی هر صفحه — قبل از جاگ/لیست
  if (dev_lock_try_digit(digit)) return;
  // در جاگ: هر رقم غیرحرکت → درخواست خروج به حالت عدد
  if (GUI.scr_id == 1 && GUI.NumLock && !STP.begin_calibration) {
    request_exit_queue_mode_prompt(3);
    GUI.buzzer_cmd_long = true;
    return;
  }
  // در لیست/بهینه: رقم = درخواست خروج (نه تایپ مقصد)
  if (GUI.scr_id == 1 && main_queue_mode_active() && !GUI.NumLock && !STP.begin_calibration) {
    request_exit_queue_mode_prompt(1);
    GUI.buzzer_cmd_long = true;
    return;
  }
  bool inputMode_sp = !GUI.NumLock && !STP.begin_calibration && GUI.scr_id == 1;
  if (GUI.scr_id == 3) {
    // ممیز مخصوص متراژ است؛ روی تعداد، بافر صحیح را صفر یا ناقص نکن.
    if (digit == '.' || digit == ',') {
      for (int j = 0; j < LIST_MAX; j++) {
        if (AUT.list_edit_mode_count[j]) {
          GUI.buzzer_cmd_long = true;
          return;
        }
      }
    }
    bool handled = false;
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_sp[j]) {
        if (GUI.clear_buffer_aut_flag) GUI.inputBuffer_L_3_1_x[j] = "";
        GUI.clear_buffer_aut_flag = false;
        // همان محدودیت تاچ: سقف رقم = طول شاخه، یک ممیز، toast محدوده
        if (list_try_size_digit(GUI.inputBuffer_L_3_1_x[j], digit)) {
          GUI.flag_update_labels_3_1 = true;
        }
        handled = true;
        break;
      }
    }
    if (!handled) {
      for (int j = 0; j < LIST_MAX; j++) {
        if (AUT.list_edit_mode_count[j]) {
          if (GUI.clear_buffer_aut_flag) GUI.inputBuffer_L_3_2_x[j] = "";
          GUI.clear_buffer_aut_flag = false;
          keyboardAddCountDigit(GUI.inputBuffer_L_3_2_x[j], digit);
          GUI.flag_update_labels_3_2 = true;
          handled = true;
          break;
        }
      }
    }
    // بدون سلکت → اولین متراژ صفحه را بگیر و همان رقم را بزن
    if (!handled) {
      int first = GUI.list_page;
      if (first < 0) first = 0;
      if (first >= LIST_MAX) first = LIST_MAX - 1;
      if (first >= GUI.visible_raws && GUI.visible_raws > 0) first = GUI.visible_raws - 1;
      list_show_row(first);
      begin_screen3_sp_edit(first);
      if (GUI.clear_buffer_aut_flag) GUI.inputBuffer_L_3_1_x[first] = "";
      GUI.clear_buffer_aut_flag = false;
      if (list_try_size_digit(GUI.inputBuffer_L_3_1_x[first], digit)) {
        GUI.flag_update_labels_3_1 = true;
      }
      handled = true;
    }
    if (handled) GUI.flag_panel3_edit_labels_txt = true;
  } else if (GUI.scr_id == 1 && numpad_ctx == 5) {
    entry_add_digit(digit);
    return;
  } else if (inputMode_sp) {
    if (STP.motorIs_moving || STP.moveStepperCMD) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (AUT.selected_count_val != -1) return;
    clear_buffer_sp_flag = false;
    jog_keep_dest = false;  // تایپ دستی مقصد → ورود بعدی جاگ به موتور بچسبد
    jog_forward_resume_dest_valid = false;
    GUI.edit_mode_lable1_1 = true;  // تایپ = حالت عدد — کادر با بک‌اسپیس خالی نماند
    bool ok = false;
    if (xSemaphoreTake(lable1_1_Semaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (clear_buffer_sp_flag) {
        GUI.inputBuffer_sp = "";
      }
      ok = append_main_target_char(GUI.inputBuffer_sp, digit);
      xSemaphoreGive(lable1_1_Semaphore);
    }
    if (!ok) GUI.buzzer_cmd_long = true;
    GUI.update_lable1_1 = true;
  } else if (GUI.scr_id == 2 && f1_general_settings_active()) {
    // F1: کیبورد USB مستقیم به keyboardHandleF1Digit — بدون فراخوانی f1_edit_focus
    if (numpad_ctx != 3 && numpad_ctx != 4) {
      numpad_ctx = 3;
      settings_num_buf = String(stock_len_mm);
      settings_clear_on_type = true;
      GUI.flag_sync_stock_kerf = true;
    }
    keyboardHandleF1Digit(digit);
  } else if (pitch_cal_try_digit(digit)) {
  } else if (f4_usb_try_digit(digit)) {
  } else if (GUI.scr_id == 2 && !STP.begin_calibration && ui_active_settings_tab == 1) {
    // F2: USB حتی بدون لمس کادر → اول خط (مثل کیبورد تاچ)
    if (!(GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5))
      f2_begin_edit_start_line();
    f2_try_append_char(digit);
  } else if (STP.begin_calibration && STP.calib_stage == 2 && GUI.scr_id == 2) {
    if (GUI.clear_buffer_aut_flag) GUI.inputBuffer_initial_L1 = "";
    GUI.clear_buffer_aut_flag = false;
    keyboardAddDigit(GUI.inputBuffer_initial_L1, digit);
    GUI.flag_update_lables_2 = true;
  } else if ((STP.begin_calibration && STP.calib_stage == 4 && GUI.scr_id == 2) ||
             (!STP.begin_calibration && GUI.scr_id == 2 && GUI.enable_edit_lable2_3) ||
             (!STP.begin_calibration && GUI.scr_id == 2 && GUI.enable_edit_lable2_5)) {
    f2_try_append_char(digit);
  }
}

static void keyboardHandleBackspace() {
  if (dev_lock_try_backspace()) return;
  if (pitch_cal_try_backspace()) return;
  if (f4_usb_try_backspace()) return;
  if (GUI.scr_id == 1 && numpad_ctx == 5) {
    entry_backspace();
    return;
  }
  if (GUI.scr_id == 3) {
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_sp[j] && GUI.inputBuffer_L_3_1_x[j].length()) {
        GUI.inputBuffer_L_3_1_x[j].remove(GUI.inputBuffer_L_3_1_x[j].length() - 1);
        GUI.flag_update_labels_3_1 = true;
        return;
      }
    }
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_count[j] && GUI.inputBuffer_L_3_2_x[j].length()) {
        GUI.inputBuffer_L_3_2_x[j].remove(GUI.inputBuffer_L_3_2_x[j].length() - 1);
        GUI.flag_update_labels_3_2 = true;
        return;
      }
    }
  } else if (!GUI.NumLock && GUI.scr_id == 1) {
    if (STP.motorIs_moving || STP.moveStepperCMD) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (xSemaphoreTake(lable1_1_Semaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (GUI.inputBuffer_sp.length()) {
        GUI.edit_mode_lable1_1 = true;  // بک‌اسپیس تا خالی: کادر خاکستری نپرد
        GUI.inputBuffer_sp.remove(GUI.inputBuffer_sp.length() - 1);
        GUI.update_lable1_1 = true;
      }
      xSemaphoreGive(lable1_1_Semaphore);
    }
  } else if (f1_num_entry_active()) {
    keyboardHandleF1Backspace();
  } else if (STP.begin_calibration && STP.calib_stage == 2 && GUI.inputBuffer_initial_L1.length()) {
    GUI.inputBuffer_initial_L1.remove(GUI.inputBuffer_initial_L1.length() - 1);
    GUI.flag_update_lables_2 = true;
  } else if (((STP.begin_calibration && STP.calib_stage == 4) ||
              (!STP.begin_calibration && GUI.scr_id == 2 && GUI.enable_edit_lable2_3)) ||
             (!STP.begin_calibration && GUI.scr_id == 2 && GUI.enable_edit_lable2_5)) {
    f2_on_backspace();
  } else if (GUI.scr_id == 2 && !STP.begin_calibration && ui_active_settings_tab == 1) {
    if (!(GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5))
      f2_begin_edit_start_line();
    f2_on_backspace();
  } else if (panel_five_nav_keyboard_cancel()) {
    return;
  } else {
    keyboard_nav_page_back();
  }
}

static void keyboardHandleEnter() {
  // رمزها اول: قفل دستگاه / دروازه F4 / کالیبر — قبل از رفرنس یا ناوبری
  if (dev_lock_try_enter()) return;
  if (pitch_cal_try_enter()) return;
  if (f4_usb_try_enter()) return;
  if (GUI.scr_id == 1 && numpad_ctx == 5) {
    keyboard_pending_enter = true;
    return;
  }
  if (f1_num_entry_active() ||
      (GUI.scr_id == 2 && (GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5))) {
    keyboard_pending_enter = true;
    return;
  }
  // صفحه موقعیت‌یابی: Enter = دکمه سبز (نه صفحه اصلی)
  if (GUI.scr_id == 0 && !STP.begin_calibration) {
    ref_homing_try_start();
    return;
  }
  if (GUI.scr_id == 3) {
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_sp[j] || AUT.list_edit_mode_count[j]) {
        keyboard_pending_enter = true;
        return;
      }
    }
  }
  // صفحه اصلی
  if (GUI.scr_id == 1) {
    // در جاگ: Enter فقط توقف نرم — از جاگ خارج نشو
    if (GUI.NumLock) {
      if (keyboardJogActive || STP.jogEnabled) {
        STP.jogEnabled = false;
        keyboardJogActive = false;
        jog_freeze_target_at_current();
        GUI.buzzer_cmd_short = true;
      }
      return;
    }
    if (keyboardJogActive) {
      STP.jogEnabled = false;
      keyboardJogActive = false;
      jog_freeze_target_at_current();
    }
    STP.jogEnabled = false;
    keyboard_pending_enter = true;
    return;
  }
  // کنترل پویا (به‌ویژه پنجره انتخاب پایه F3) حتی پیش از reference
  // یک عملیات UI است و نباید کاربر را ناخواسته به صفحهٔ هومینگ بفرستد.
  if (panel_five_nav_keyboard_confirm()) return;
  // بدون رفرنس: فقط به صفحهٔ سبز برو — خودت هومینگ را شروع نکن (ضد ریکاوری هنگ)
  if (!STP.referenced && !STP.begin_calibration) {
    if (GUI.scr_id != 0) GUI.pending_goto_scr = 0;
    return;
  }
  keyboard_nav_queue_key(LV_KEY_ENTER);
}

static void keyboardHandleKey(uint8_t key_code) {
  unsigned long now = millis();
  if (key_code == 0 || (key_code == prevKeyCode && now - lastKeyPressTime < debounceDelay)) return;
  lastKeyPressTime = now;
  prevKeyCode = key_code;

  if (cut_start_wrong_screen_modal_active()) {
    GUI.buzzer_cmd_long = true;
    return;
  }

  // [USB/Task4] NumLock outside Main is a navigation request only.  Never
  // touch LVGL here: Task2 owns the actual screen load.  Device-lock,
  // updater and any live/locked machine state keep their established guard.
  if (key_code == 0x53 && GUI.scr_id != 1 && !device_locked &&
      !GUI.lock_screen && !g_update_mode) {
    const bool machine_busy = STP.motorIs_moving || STP.moveStepperCMD ||
                              STP.goref_cmd || STP.begin_calibration ||
                              STP.flag_is_cutting || IOS.saw_is_on ||
                              IOS.clamp_engaged || IOS.cut_confirm_pending;
    if (machine_busy) {
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.pending_goto_scr = 1;
    GUI.buzzer_cmd_short = true;
    return;
  }

  // Enter همیشه اول — حتی اگر motorIs_moving گیر کرده باشد
  if (key_code == 0x58 || key_code == 0x28) { keyboardHandleEnter(); return; }

  bool numMode = GUI.NumLock && STP.referenced;
  // حین decel جوگ هم باید بشود جهت عوض کرد / دوباره گرفت — فقط حرکت برنامه‌ای را بلاک کن
  const bool jogArrow = numMode && !STP.begin_calibration && GUI.scr_id == 1 &&
                        (isKeyboardJogLeft(key_code) || isKeyboardJogRight(key_code));
  const bool jogFine = numMode && !STP.begin_calibration && GUI.scr_id == 1 &&
                       isKeyboardJogFine(key_code);
  // موتور در حرکت جاگ: از مد خارج نشو؛ کلید غیرجاگ → همان پیام خروج
  if (STP.motorIs_moving && !jogArrow && !jogFine) {
    if (numMode && !STP.begin_calibration && GUI.scr_id == 1) {
      request_exit_queue_mode_prompt(3);
      GUI.buzzer_cmd_long = true;
    }
    return;
  }

  if (jogArrow) {
    if (main_queue_mode_active()) {
      request_exit_queue_mode_prompt(2);
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (STP.flag_is_cutting) { GUI.buzzer_cmd_long = true; return; }
    jog_engage_dir(isKeyboardJogLeft(key_code) ? -1 : 1);
    return;
  }

  if (numMode && !STP.begin_calibration && GUI.scr_id == 1) {
    // رقم‌ها و بقیه در جاگ پایین‌تر هندل می‌شوند
  }

  if (!STP.flag_is_cutting) {
    if (key_code == 0x4F || key_code == 0x51) { keyboard_nav_queue_key(LV_KEY_NEXT); return; }
    if (key_code == 0x50 || key_code == 0x52) { keyboard_nav_queue_key(LV_KEY_PREV); return; }
  }

  const char numMap[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };
  const uint8_t keyMapPad[] = { 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62 };
  const uint8_t keyMapTop[] = { 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27 };
  for (int i = 0; i < 10; i++) {
    if (key_code == keyMapPad[i] || key_code == keyMapTop[i]) {
      // رقم ۲ در NumLock جاگ = fine speed، نه تایپ
      if (numMode && isKeyboardJogFine(key_code)) return;
      keyboardHandleDigit(numMap[i]);
      return;
    }
  }

  // ممیز پد عددی: هدف اصلی، F2 اول/آخر خط، بعد F1
  if (key_code == 0x63) {
    if (!GUI.NumLock && !STP.begin_calibration && GUI.scr_id == 1 && AUT.selected_count_val == -1) {
      keyboardHandleDigit('.');
      return;
    }
    if (GUI.scr_id == 2 && !STP.begin_calibration && ui_active_settings_tab == 1) {
      if (!(GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5))
        f2_begin_edit_start_line();
      f2_try_append_char('.');
      return;
    }
    if (GUI.scr_id == 2 && (GUI.enable_edit_lable2_3 || GUI.enable_edit_lable2_5)) {
      f2_try_append_char('.');
      return;
    }
    if (f1_try_num_digit('.')) return;
  }
  if (key_code == 0x2A) { keyboardHandleBackspace(); return; }

  if (key_code == 0x53 && GUI.scr_id == 1) {
    // NumLock هرگز مجاز نیست دستگاه هوم‌نشده را referenced اعلام کند.
    if (!STP.referenced) {
      GUI.buzzer_cmd_long = true;
      GUI.pending_goto_scr = 0;
      return;
    }
    if (STP.flag_is_cutting) { GUI.buzzer_cmd_long = true; return; }
    // وسط حرکت جاگ: NumLock خروج ندهد — فقط پیام
    if (GUI.NumLock && STP.motorIs_moving) {
      request_exit_queue_mode_prompt(3);
      GUI.buzzer_cmd_long = true;
      return;
    }
    if (main_queue_mode_active() && !GUI.NumLock) {
      request_exit_queue_mode_prompt(2);
      GUI.buzzer_cmd_long = true;
      return;
    }
    GUI.NumLock = !GUI.NumLock;
    GUI.edit_mode_lable1_1 = !GUI.NumLock;
    jog_numlock_from_joystick = false;  // ورود/خروج با کیبرد — نه جویستیک
    if (GUI.NumLock) {
      // ورود به جاگ: مقصد = عدد وسط
      jog_capture_forward_resume_dest();
      jog_snap_dest_to_motor();
      jog_keep_dest = false;
    } else {
      // خروج قطعی از جاگ — بدون دست زدن به LVGL از Task4 (باعث هنگ می‌شد)
      STP.jogEnabled = false;
      keyboardJogActive = false;
      jog_forward_resume_dest_valid = false;
      if (STP.motorIs_moving && !STP.moveStepperCMD) STP.mvm_interupt = true;
    }
    AUT.selected_count_val = -1;
    GUI.update_lable1_3 = true;
    GUI.flag_update_panel3_2_x_clr = true;
    clear_buffer_sp_flag = true;
    GUI.buzzer_cmd_short = true;
    GUI.update_lable1_1 = true;
    usb_keyboard_set_numlock_led(!GUI.NumLock);  // فقط صف LED؛ اعمال در حلقهٔ Task4
    // کادر قرمز را Task2 با prcs_panel1_2_clr همگام می‌کند — اینجا LVGL ممنوع
  }
}

static void keyboardMarkKeyHandled(uint8_t key_code) {
  if (key_code == 0) return;
  prevKeyCode = key_code;
  lastKeyPressTime = millis();
}

// HID ErrorRollOver (۰x۰۱ در همه اسلات‌ها) — گزارش ناقص؛ وضعیت قبلی را نگه دار
static bool hid_keyboard_report_is_rollover(const hid_keyboard_report_t &report) {
  return report.keycode[0] == 0x01;
}

static void update_jog_fine_held_from_report(const hid_keyboard_report_t &report) {
  if (hid_keyboard_report_is_rollover(report)) return;
  bool hasFine = false;
  for (int i = 0; i < 6; i++) {
    if (isKeyboardJogFine(report.keycode[i])) {
      hasFine = true;
      break;
    }
  }
  // فقط خود کلید ۲ — مستقل از ۴/۶؛ با رها کردن ۲ فوراً خاموش
  jog_fine_held = hasFine && GUI.NumLock && GUI.scr_id == 1 && STP.referenced;
}

static char usb_keyboard_event_char(const UsbKeyboardUiEvent &ev) {
  if (ev.ascii >= 0x20 || ev.ascii == '\b' || ev.ascii == '\r' ||
      ev.ascii == '\n' || ev.ascii == 127)
    return (char)ev.ascii;
  static const char num_map[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
  static const uint8_t pad_map[] = {0x59, 0x5A, 0x5B, 0x5C, 0x5D,
                                    0x5E, 0x5F, 0x60, 0x61, 0x62};
  static const uint8_t top_map[] = {0x1E, 0x1F, 0x20, 0x21, 0x22,
                                    0x23, 0x24, 0x25, 0x26, 0x27};
  for (int i = 0; i < 10; ++i)
    if (ev.keycode == pad_map[i] || ev.keycode == top_map[i]) return num_map[i];
  if (ev.keycode == 0x63) return '.';
  if (ev.keycode == 0x58 || ev.keycode == 0x28) return '\r';
  if (ev.keycode == 0x2A) return '\b';
  return 0;
}

void usb_keyboard_ui_drain() {
  if (!usb_keyboard_ui_queue) return;
  uint32_t dropped = usb_keyboard_ui_queue_drops;
  if (dropped) {
    usb_keyboard_ui_queue_drops = 0;
    Serial.printf("[USB-KBD] UI queue dropped=%u\n", (unsigned)dropped);
  }

  UsbKeyboardUiEvent ev;
  for (int handled = 0;
       handled < 12 && xQueueReceive(usb_keyboard_ui_queue, &ev, 0) == pdTRUE;
       ++handled) {
    char c = usb_keyboard_event_char(ev);
    if (c == ',') c = '.';

    // راهنمای راه‌اندازی اولیه دستگاه (OOBE Wizard) در اولین بوت پس از پاکسازی حافظه
    if (oobe_is_active()) {
      if (c == '\r' || c == '\n' || ev.keycode == 0x28 || ev.keycode == 0x58) {
        oobe_try_enter();
      } else if (c == '\b' || c == 127 || ev.keycode == 0x2A) {
        oobe_try_backspace();
      } else if (c >= 0x20) {
        oobe_try_char(c);
      }
      continue;
    }

    // کلید "/" برای بازگشت سریع به صفحه اصلی
    if (c == '/' || ev.keycode == 0x54 || ev.keycode == 0x38) {
      if (panel_five_nav_is_active()) panel_five_nav_exit();
      if (!STP.flag_is_cutting && !IOS.saw_is_on && !IOS.clamp_engaged) {
        GUI.pending_goto_scr = 1;
        ui_toast("صفحه اصلی", "Home Screen", 1000);
        continue;
      }
    }

    // Password textareas are still drained by update_flow_tick, but their
    // producer now runs on Task2 instead of the USB host core.
    if (g_update_mode && uf_kbd_active()) {
      if (c) uf_wp_push_char(c);
      continue;
    }

    // کلید NumLock خارج از صفحه اصلی: فعال‌سازی / خروج از نشانگر ناوبری بدون تاچ
    if (ev.keycode == 0x53 && GUI.scr_id != 1) {
      if (panel_five_nav_is_active()) {
        panel_five_nav_exit();
        ui_toast("خروج از نشانگر", "Cursor OFF", 1000);
      } else {
        panel_five_nav_mode_next();
        ui_toast("حالت ناوبری فعال", "Cursor ON", 1000);
      }
      continue;
    }

    // ناوبری کامل با جهت‌های کیبورد USB در حالت فعال بودن نشانگر
    if (panel_five_nav_is_active()) {
      if (c == '8' || ev.keycode == 0x52 || ev.keycode == 0x60) { // بالا
        panel_five_nav_prev();
        continue;
      }
      if (c == '2' || ev.keycode == 0x51 || ev.keycode == 0x5A) { // پایین
        panel_five_nav_next();
        continue;
      }
      if (c == '4' || ev.keycode == 0x50 || ev.keycode == 0x5C) { // چپ
        panel_five_nav_prev();
        continue;
      }
      if (c == '6' || ev.keycode == 0x4F || ev.keycode == 0x5E) { // راست
        panel_five_nav_next();
        continue;
      }
      if (c == '+' || ev.keycode == 0x57) { // افزایش
        panel_five_nav_adjust(+1);
        continue;
      }
      if (c == '-' || ev.keycode == 0x56) { // کاهش
        panel_five_nav_adjust(-1);
        continue;
      }
      if (c == '\r' || c == '\n' || ev.keycode == 0x28 || ev.keycode == 0x58) { // اینتر / انتخاب
        panel_five_nav_confirm();
        return;
      }
      if (c == '\b' || c == 127 || ev.keycode == 0x29 || ev.keycode == 0x2A || ev.keycode == 0x4C) { // انصراف
        panel_five_nav_cancel_level();
        continue;
      }
    }

    if (c >= '0' && c <= '9') {
      keyboardHandleDigit(c);
      continue;
    }
    if (c == '.') {
      keyboardHandleDigit('.');
      continue;
    }
    if (c == '\r' || c == '\n') {
      keyboardHandleEnter();
      // commit در prcs_keyboard_enter بلافاصله بعد از drain انجام می‌شود.
      // کاراکتر بعدی را برای تیک بعد نگه دار تا وارد فیلد قبلی نشود.
      return;
    }
    if (c == '\b' || c == 127) {
      keyboardHandleBackspace();
      continue;
    }
    if (c == '-') {
      if (GUI.scr_id == 1 && numpad_ctx == 5) {
        entry_add_digit('-');
        continue;
      }
      (void)f4_usb_try_digit('-');
      continue;
    }

    if (ev.keycode == 0x4F || ev.keycode == 0x51 ||
        ev.keycode == 0x50 || ev.keycode == 0x52 ||
        ev.keycode == 0x2B) {
      if (STP.flag_is_cutting) continue;
      const int dir = (ev.keycode == 0x50 || ev.keycode == 0x52) ? -1 : 1;
      const bool dynamic_nav = panel_five_nav_is_active() ||
                               f3_pinmap_dialog_is_open() ||
                               GUI.scr_id == 2 || GUI.scr_id == 3 ||
                               (!GUI.NumLock && GUI.scr_id == 1);
      if (!dynamic_nav || !panel_five_nav_keyboard_step(dir))
        keyboard_nav_queue_key(dir > 0 ? LV_KEY_NEXT : LV_KEY_PREV);
      continue;
    }

    if (ev.keycode == 0x29) {
      if (!panel_five_nav_keyboard_cancel()) keyboard_nav_page_back();
    }
  }
}

class MyEspUsbHostKeybord : public EspUsbHostKeybord {
public:
  void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t last_report) override {
    (void)last_report;
    // ErrorRollOver: نادیده بگیر — ولی هرگز جلوی stop با گزارش خالی را نگیر
    if (hid_keyboard_report_is_rollover(report)) {
      jog_fine_held = false;
      if (keyboardJogActive) keyboardStopJog();
      return;
    }

    bool anyKey = false;
    bool hasJogArrow = false;
    for (int i = 0; i < 6; i++) {
      uint8_t k = report.keycode[i];
      if (k == 0) continue;
      anyKey = true;
      if (isKeyboardJogLeft(k) || isKeyboardJogRight(k)) hasJogArrow = true;
    }
    update_jog_fine_held_from_report(report);

    // رها کردن همه کلیدها → حتماً جاگ بایستد
    // مالک: runaway تا انتهای خط غیرقابل‌قبول است (OWNER_PROFILE_FA / درس 1.5.106)
    if (!anyKey) {
      jog_fine_held = false;
      prevKeyCode = 0;
      if (keyboardJogActive) keyboardStopJog();
      return;
    }

    // رها کردن ۴/۶ (حتی اگر ۲ هنوز پایین است) → توقف جاگ
    if (!hasJogArrow && keyboardJogActive) {
      keyboardStopJog();
    }
  }

  void onGone(const usb_host_client_event_msg_t *eventMsg) override {
    (void)eventMsg;
    prevKeyCode = 0;
    lastKeyPressTime = 0;
    jog_fine_held = false;
    keyboardStopJog();
    usb_keyboard_ui_queue_reset();
    usb_mouse_reset_state();
    // soft-KB در تیک F4 با !usb_keyboard_present() باز می‌شود
  }

  void onMouseMove(hid_mouse_report_t report) override {
    // نسبت قبلی حفظ: X≈۱:۱، Y≈۱/۸ ؛ هر دو محور ۲۰٪ آهسته‌تر (×۰٫۸)
    static int mouse_acc_x = 0;
    static int mouse_acc_y = 0;
    mouse_acc_x += (int)report.x * 4;
    mouse_acc_y += (int)report.y;
    int dx = mouse_acc_x / 5;          // 4/5 = 80%
    mouse_acc_x -= dx * 5;
    int dy = mouse_acc_y / 10;         // (1/8)×0.8 = 1/10
    mouse_acc_y -= dy * 10;
    if (dx != 0 || dy != 0) usb_mouse_move_pointer(dx, dy);
    if (report.wheel != 0) usb_mouse_add_wheel((int)report.wheel);
  }

  void onMouseButtons(hid_mouse_report_t report, uint8_t last_buttons) override {
    const bool left = (report.buttons & MOUSE_BUTTON_LEFT) != 0;
    usb_mouse_set_left_button(left);
    const bool right_edge = !(last_buttons & MOUSE_BUTTON_RIGHT) && (report.buttons & MOUSE_BUTTON_RIGHT);
    const bool back_edge =
        (!(last_buttons & MOUSE_BUTTON_BACKWARD) && (report.buttons & MOUSE_BUTTON_BACKWARD)) ||
        (!(last_buttons & MOUSE_BUTTON_FORWARD) && (report.buttons & MOUSE_BUTTON_FORWARD));
    if (right_edge || back_edge) usb_mouse_report_buttons(left, right_edge, back_edge);
  }

  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) override {
    const bool jog_mode = GUI.NumLock && STP.referenced &&
                          !STP.begin_calibration && GUI.scr_id == 1;
    const bool jog_blocked =
        device_locked || GUI.lock_screen || sensor_alert_motion_lock ||
        IOS.stop_button_state || STP.flag_is_cutting ||
        main_queue_mode_active();

    // فقط کنترل‌های زمان‌حساس حرکت در Task4 می‌مانند. هر مسیر UI/LVGL
    // از صف به Task2 تحویل می‌شود تا دو هسته هم‌زمان به LVGL دست نزنند.
    if (jog_mode &&
        (isKeyboardJogLeft(keycode) || isKeyboardJogRight(keycode))) {
      if (jog_blocked) {
        if (keyboardJogActive || STP.jogEnabled) keyboardStopJog();
        GUI.buzzer_cmd_long = true;
        return;
      }
      keyboardHandleKey(keycode);
      keyboardMarkKeyHandled(keycode);
      return;
    }
    if (jog_mode && isKeyboardJogFine(keycode)) {
      if (jog_blocked) {
        if (keyboardJogActive || STP.jogEnabled) keyboardStopJog();
        GUI.buzzer_cmd_long = true;
        return;
      }
      jog_fine_held = true;
      keyboardMarkKeyHandled(keycode);
      return;
    }
    if (jog_mode && (keycode == 0x58 || keycode == 0x28)) {
      // Enter در جاگ باید حتی هنگام شلوغی UI فوراً توقف را درخواست کند.
      if (keyboardJogActive || STP.jogEnabled) {
        STP.jogEnabled = false;
        keyboardJogActive = false;
        jog_freeze_target_at_current();
        GUI.buzzer_cmd_short = true;
      }
      keyboardMarkKeyHandled(keycode);
      return;
    }
    if (keycode == 0x53 && GUI.scr_id == 1) {
      // NumLock مالک مستقیم حالت جاگ است و داخل این مسیر LVGL فراخوانی نمی‌شود.
      keyboardHandleKey(keycode);
      keyboardMarkKeyHandled(keycode);
      return;
    }
    (void)usb_keyboard_ui_enqueue(ascii, keycode, modifier);
  }

  void onKey(usb_transfer_t *transfer) override {
    (void)transfer;
  }
};

static MyEspUsbHostKeybord usbHost;

// HID re-enumeration briefly clears isReady/hasHidKeyboard.  Keep the last
// confirmed keyboard state for a short, UI-only grace window so AUTO soft-KB
// cannot flash open during a healthy reconnect.  A real unplug still falls
// back to the touch keyboard after the bounded delay.
static volatile uint32_t usb_keyboard_last_ready_ms = 0;
static const uint32_t USB_KEYBOARD_DISCONNECT_GRACE_MS = 1200UL;

bool usb_keyboard_present() {
  // فقط کیبورد واقعی؛ دانگل موس هرگز کیبرد محسوب نمی‌شود.
  const bool ready = usbHost.isReady && usbHost.hasHidKeyboard &&
                     usbHost.deviceHandle != NULL;
  const uint32_t now = millis();
  if (ready) {
    usb_keyboard_last_ready_ms = now;
    return true;
  }
  const uint32_t last = usb_keyboard_last_ready_ms;
  return last != 0 && (uint32_t)(now - last) < USB_KEYBOARD_DISCONNECT_GRACE_MS;
}

static int8_t usb_numlock_led_last = -1;
static volatile int8_t usb_numlock_led_cmd = -1;  // -1=idle 0/1=set 2/3=force off/on

static void usb_keyboard_apply_numlock_led(bool on, bool force) {
  // Never send HID SET_REPORT to a mouse-only device.  It can disturb an
  // otherwise healthy pointer and keyboardInterface is invalid in that case.
  if (!usbHost.isReady || !usbHost.hasHidKeyboard ||
      usbHost.keyboardInterface < 0 || !usbHost.deviceHandle ||
      !usbHost.clientHandle) return;
  const int8_t want = on ? 1 : 0;
  if (!force && usb_numlock_led_last == want) return;
  usb_transfer_t *transfer = NULL;
  if (usb_host_transfer_alloc(9, 0, &transfer) != ESP_OK || !transfer) return;
  transfer->num_bytes = 9;
  transfer->data_buffer[0] = 0x21;
  transfer->data_buffer[1] = 0x09;
  transfer->data_buffer[2] = 0x00;
  transfer->data_buffer[3] = 0x02;
  transfer->data_buffer[4] = (usbHost.keyboardInterface >= 0)
    ? (uint8_t)usbHost.keyboardInterface : 0x00;
  transfer->data_buffer[5] = 0x00;
  transfer->data_buffer[6] = 0x01;
  transfer->data_buffer[7] = 0x00;
  transfer->data_buffer[8] = on ? 0x01 : 0x00;
  transfer->device_handle = usbHost.deviceHandle;
  transfer->bEndpointAddress = 0x00;
  transfer->callback = EspUsbHost::_onReceiveControl;
  transfer->context = &usbHost;
  esp_err_t err = usb_host_transfer_submit_control(usbHost.clientHandle, transfer);
  if (err != ESP_OK) {
    usb_host_transfer_free(transfer);
    return;
  }
  usb_numlock_led_last = want;
}

void usb_keyboard_set_numlock_led_force(bool on) {
  // از هر تسک فقط صف؛ USB فقط در Task4
  usb_numlock_led_cmd = on ? 3 : 2;
}

void usb_keyboard_set_numlock_led(bool on) {
  usb_numlock_led_cmd = on ? 1 : 0;
}

void Task4(void *pvParameters) {
  vTaskDelay(300 / portTICK_PERIOD_MS);
  Serial.println("Task4 started");
  usb_keyboard_ui_queue_init();
  usbHost.begin();
  while (1) {
    usbHost.task();
    int8_t cmd = usb_numlock_led_cmd;
    if (cmd >= 0) {
      usb_numlock_led_cmd = -1;
      usb_keyboard_apply_numlock_led((cmd == 1 || cmd == 3), cmd >= 2);
    }
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}
