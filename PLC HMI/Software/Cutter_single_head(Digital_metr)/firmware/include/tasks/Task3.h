//task: stepper motor control
#include "esp_rom_sys.h"
#include "soc/gpio_struct.h"
#include <math.h>

hw_timer_t* timer = NULL;
volatile bool pinState = false;
volatile int totalPulses = 0;      // for manual movement count
volatile bool isr_clutch = false;  //ingagement of timer isr
volatile int movement_mode = 1;    // 1: manual movement, 2: jog, 3: reference, 4: sampling
volatile int ref_stage = 0;
volatile int32_t jog_control_watchdog_pulses = 0;
static const int MOTION_RAMP_MIN_MS = 0;
static const int MOTION_RAMP_MAX_MS = 5000;
static const int STEPPER_PULSE_HIGH_US = 3;
static uint32_t stepper_pulse_mask = 0;
static bool stepper_pulse_high_reg = false;
volatile int32_t stepper_ui_update_threshold_pulses = 1;

static inline float stepperPulsesPerMm();
static inline int stepperIntervalUsForSpeed(int intervalUs);

static inline void stepperInitFastPulsePin() {
  int pin = IOS.pul_pin;
  if (pin < 0) {
    stepper_pulse_high_reg = false;
    stepper_pulse_mask = 0;
    return;
  }
  stepper_pulse_high_reg = pin >= 32;
  stepper_pulse_mask = (uint32_t)1UL << (stepper_pulse_high_reg ? (pin - 32) : pin);
}

static inline void IRAM_ATTR stepperPulseHigh() {
  if (stepper_pulse_high_reg) GPIO.out1_w1ts.val = stepper_pulse_mask;
  else GPIO.out_w1ts = stepper_pulse_mask;
}

static inline void IRAM_ATTR stepperPulseLow() {
  if (stepper_pulse_high_reg) GPIO.out1_w1tc.val = stepper_pulse_mask;
  else GPIO.out_w1tc = stepper_pulse_mask;
}

static inline bool stepperStopRequested() {
  // نبود I0/I1 حرکت عادی را قفل نمی‌کند؛ حدود نرم تنظیم‌شده همچنان مقصد را محدود می‌کنند.
  // موقعیت‌یابی خودکار بدون I0 جداگانه در UI مسدود می‌شود.
  if (critical_task_start_failed || IOS.stop_button_state ||
      STP.mvm_interupt || sensor_alert_motion_lock) return true;
  const bool toward_plus = (STP.system_set_dir * STP.system_base_dir == 1);
  const bool ref_raw = IOS.ref_sensor_state;
  const bool up_raw = IOS.up_lim_sensor_state;
  if (toward_plus) {
    if (up_raw) return true;
    if (ref_raw && limit_escape_sensor != 1) return true;
  } else {
    if (ref_raw) return true;
    if (up_raw && limit_escape_sensor != 2) return true;
  }
  return false;
}
// کدام سنسور الان مسیر را می‌بندد؟ (۱ اول خط / ۲ آخر خط / ۰ هیچ)
static inline int IRAM_ATTR stepperSensorTripNow() {
  if (sensor_alert_motion_lock) return 0;
  const bool toward_plus = (STP.system_set_dir * STP.system_base_dir == 1);
  if (toward_plus) {
    if (IOS.up_lim_sensor_state) return 2;
    if (IOS.ref_sensor_state && limit_escape_sensor != 1) return 1;
  } else {
    if (IOS.ref_sensor_state) return 1;
    if (IOS.up_lim_sensor_state && limit_escape_sensor != 2) return 2;
  }
  return 0;
}
static inline void IRAM_ATTR stepperLatchTripIfAny() {
  int t = stepperSensorTripNow();
  if (t > 0) sensor_trip_which = (int8_t)t;
}
static inline uint64_t stepperTimerPeriodUs(int halfIntervalUs) {
  halfIntervalUs = stepperIntervalUsForSpeed(halfIntervalUs);
  uint64_t periodUs = (uint64_t)halfIntervalUs * 2ULL;
  uint64_t minPeriodUs = (uint64_t)STEPPER_PULSE_HIGH_US + 3ULL;
  return (periodUs < minPeriodUs) ? minPeriodUs : periodUs;
}
static inline void stepperUpdateUiPulseGate() {
  int32_t threshold = (int32_t)(stepperPulsesPerMm() + 0.5f);
  if (threshold < 1) threshold = 1;
  stepper_ui_update_threshold_pulses = threshold;
}
static inline void IRAM_ATTR stepperEmitPulse() {
  stepperPulseHigh();
  esp_rom_delay_us(STEPPER_PULSE_HIGH_US);
  stepperPulseLow();
}
static inline void IRAM_ATTR stepperUpdatePositionFromPulse(int32_t* lastGuiPosition) {
  STP.currentPosition_p += (STP.system_set_dir * STP.system_base_dir == 1) ? 1 : -1;
  // ISR فقط شمارندهٔ واقعی پالس را نگه می‌دارد؛ نمونه‌برداری UI در Task2 انجام می‌شود.
  // شمارش تک‌تک پالس‌ها حفظ می‌شود، بدون محاسبه/نوشتن بین‌هسته‌ای اضافی در هر پالس.
  (void)lastGuiPosition;
}
static inline void stepperStopIsr() {
  stepperPulseLow();
  pinState = false;
  isr_clutch = false;
}
static inline bool IRAM_ATTR stepperJogControlStale() {
  int32_t limit = stepper_ui_update_threshold_pulses * 2;
  if (limit < 64) limit = 64;
  return ++jog_control_watchdog_pulses > limit;
}
static inline bool stepperArmMotion(int mode, int requestedDir, bool dirPinHigh, int startSpeed) {
  if (critical_task_start_failed || sensor_alert_motion_lock || !pinmap_out_stepper_ready()) {
    if (!pinmap_out_stepper_ready()) pinmap_notice_request_motion_block();
    isr_clutch = false;
    return false;  // تا تأیید پیام سنسور یا کامل‌بودن سه اتصال درایو، هیچ پالسی نرود
  }
  STP.system_set_dir = requestedDir;
  // دیوار سنسور فقط برای جاگ/دستی — رفرنس/نمونه‌برداری باید بتواند به سنسور نزدیک شود
  if (mode != 3 && mode != 4 && stepperStopRequested()) {
    isr_clutch = false;
    stepperLatchTripIfAny();
    return false;
  }
  movement_mode = mode;
  pinState = false;
  stepperPulseLow();
  stepperUpdateUiPulseGate();
  digitalWrite(IOS.dir_pin, dirPinHigh ? HIGH : LOW);
  digitalWrite(IOS.enb_pin, LOW);
  timerAlarm(timer, stepperTimerPeriodUs(startSpeed), true, 0);
  timerWrite(timer, 0);
  timerStart(timer);
  return true;
}
static inline void stepperStopMotion(bool disableDriver = true) {
  isr_clutch = false;
  pinState = false;
  stepperPulseLow();
  timerStop(timer);
  if (disableDriver && IOS.enb_pin >= 0) digitalWrite(IOS.enb_pin, HIGH);
}
static inline int backlashCompensationPulses() {
  // مالک: حرکت سمت چپ → ۵mm از مقصد رد شو، بعد برگرد روی مقصد (لقی تسمه/چرخ‌دنده)
  const float backlash_mm = 5.0f;
  int pulses = (int)round(backlash_mm * axisPulsesPerMm());
  return (pulses < 1) ? 1 : pulses;
}
// فقط اسنپ سنسور/حد — برای لقی موتور صدا نزن (مقصد UI پرش می‌کند)
static void sync_dest_ui_from_pulses(int pulse_pos) {
  const float ppmm = stepperPulsesPerMm();
  if (ppmm < 1.0f) return;
  const int whole = (int)lroundf((float)pulse_pos / ppmm);
  STP.Man_Sp_mm = (float)whole;
  STP.position_Setpoint_p = pulse_pos;
  jog_dest_buf_whole = whole;
  jog_dest_buf_pending = true;
  GUI.update_lable1_1 = true;
}
static inline float stepperPulsesPerMm() {
  return axisPulsesPerMm();
}
static void apply_limit_sensor_recal(int which);  // fwd
// بعد از حرکت: فقط برخورد واقعی حین حرکت در جهت دیوار.
// پارک روی سنسور / حرکت صفر پالس / برش → ارور جعلی نساز.
static void process_sensor_trip_after_motion(bool toward_plus) {
  if (STP.flag_is_cutting) {
    sensor_trip_which = 0;
    return;
  }
  int trip = (int)sensor_trip_which;
  sensor_trip_which = 0;
  // fallback فقط برای دیوار هم‌جهت — نه سنسور مخالف (پارک/خروج = ارور جهت اشتباه جعلی)
  if (trip != 1 && trip != 2) {
    if (!toward_plus && IOS.ref_sensor_state) trip = 1;
    else if (toward_plus && IOS.up_lim_sensor_state) trip = 2;
  }
  if (trip != 1 && trip != 2) return;

  jog_release_seek_pending = false;
  STP.moveStepperCMD = false;
  apply_limit_sensor_recal(trip);

  // جهت اشتباه: به سنسور مخالف خورد (سیم‌کشی/جهت موتور)
  const bool wrong = (toward_plus && trip == 1) || (!toward_plus && trip == 2);
  if (wrong) {
    ref_wrong_dir_flag = true;
    ref_wrong_dir_sensor = trip;
    limit_recal_flag = false;  // پیام جهت/سیم‌کشی غالب است
  }
}
// which: 1=سنسور اول خط(ref)  2=سنسور آخر خط(up) — همیشه snap دقیق روی عدد تنظیمات
static void apply_limit_sensor_recal(int which) {
  const float ppmm = stepperPulsesPerMm();
  if (ppmm < 1.0f) return;
  // عدد نمایشی تنظیمات (صحیح) — نه پالس قدیمی که ممکن است ۱–۲mm اختلاف بسازد
  const int snap_mm_i = (which == 1)
    ? (int)lroundf((float)STP.initial_Position_p / ppmm)
    : STP.final_position_mm.toInt();
  const int expect_p = (int)lroundf((float)snap_mm_i * ppmm);
  if (which == 1) STP.initial_Position_p = expect_p;
  else STP.final_position_p = expect_p;

  int err_p = STP.currentPosition_p - expect_p;
  if (err_p < 0) err_p = -err_p;
  limit_recal_err_mm = (int)lroundf((float)err_p / ppmm);
  limit_recal_which = which;
  limit_recal_flag = true;
  limit_escape_sensor = which;  // اجازهٔ خروج از روی همین سنسور بعد از توقف

  // قطع کامل پالس قبل از قفل عدد — وگرنه ۱ پالس اضافه → ۱۰۱ / ۵۹۹۸
  isr_clutch = false;
  pinState = false;
  stepperPulseLow();
  timerStop(timer);
  if (IOS.enb_pin >= 0) digitalWrite(IOS.enb_pin, HIGH);
  STP.currentPosition_p = expect_p;
  STP.Man_Sp_mm = (float)snap_mm_i;
  STP.position_Setpoint_p = expect_p;
  jog_grow_active = false;
  jog_keep_dest = true;
  jog_grow_peak_whole = snap_mm_i;
  jog_dest_buf_whole = snap_mm_i;
  jog_dest_buf_pending = true;
  if (xSemaphoreTake(lable1_1_Semaphore, portMAX_DELAY) == pdTRUE) {
    GUI.inputBuffer_sp = String(snap_mm_i);
    xSemaphoreGive(lable1_1_Semaphore);
  }
  GUI.update_lable1_1 = true;
  GUI.flag_update_lable1_2 = true;
}
// حد نرم تنظیمات (نه سنسور فیزیکی) — toast یک‌بار تا وقتی از همان حد دور شوی
static int s_jog_soft_limit_latched = 0;
static uint32_t escape_away_t0 = 0;

// دور شدن از سنسور فشرده بیش از ۵ثانیه بدون آزاد شدن → هشدار گیر سنسور/موتور
static inline bool escapeAwayWatchTick() {
  if (limit_escape_sensor != 1 && limit_escape_sensor != 2) {
    escape_away_t0 = 0;
    return false;
  }
  const bool pressed = (limit_escape_sensor == 1) ? IOS.ref_sensor_state : IOS.up_lim_sensor_state;
  if (!pressed) {
    escape_away_t0 = 0;
    return false;
  }
  if (!STP.motorIs_moving && !isr_clutch) {
    escape_away_t0 = 0;
    return false;
  }
  const bool toward_plus = (STP.system_set_dir * STP.system_base_dir == 1);
  const bool away =
      (limit_escape_sensor == 1 && toward_plus) ||
      (limit_escape_sensor == 2 && !toward_plus);
  if (!away) {
    escape_away_t0 = 0;
    return false;
  }
  if (escape_away_t0 == 0) escape_away_t0 = millis();
  if ((millis() - escape_away_t0) < 5000UL) return false;
  escape_away_t0 = 0;
  sensor_stuck_which = limit_escape_sensor;
  sensor_stuck_escape_flag = true;
  STP.mvm_interupt = true;
  isr_clutch = false;
  return true;
}

static void notify_jog_soft_travel_limit(int which) {
  if (s_jog_soft_limit_latched == which) return;
  s_jog_soft_limit_latched = which;
  jog_soft_limit_which = which;
  jog_soft_limit_flag = true;
  // اگر سنسور فیزیکی هم فشرده باشد، اجازهٔ خروج بعد از تأیید مودال
  limit_escape_sensor = which;
  const float ppmm = stepperPulsesPerMm();
  const float snap_mm = (which == 1)
    ? ((float)STP.initial_Position_p / ppmm)
    : STP.final_position_mm.toFloat();
  STP.Man_Sp_mm = snap_mm;
  jog_grow_active = false;
  jog_keep_dest = true;
  jog_grow_peak_whole = (int)lroundf(snap_mm);
  if (xSemaphoreTake(lable1_1_Semaphore, 0) == pdTRUE) {
    GUI.inputBuffer_sp = String((int)lroundf(snap_mm));
    xSemaphoreGive(lable1_1_Semaphore);
  }
  GUI.update_lable1_1 = true;
}
static inline int stepperMinIntervalUs() {
  // حداقل بازه مجاز — با اسلایدر GUI هماهنگ (slider max = 8µs)
  // Leadshine DM: max 200kHz (5µs)، ESP32-S3 ISR: تا 8µs قابل اطمینان
  if (STP.drv_ppr <= 6400)  return 8;
  if (STP.drv_ppr <= 12800) return 8;
  return 4;  // 25600 ppr
}
static inline int stepperIntervalUsForSpeed(int intervalUs) {
  return constrain(intervalUs, stepperMinIntervalUs(), JOG_TIMER_MAX_INTERVAL_US);
}
// کلید ۲: نرخ پالس نامی ÷۶ (بازه بزرگ‌تر = کندتر)
static inline int jog_effective_cruise_us(int nominalUs) {
  nominalUs = stepperIntervalUsForSpeed(nominalUs);
  if (!jog_fine_held) return nominalUs;
  return constrain(nominalUs * 6, nominalUs, JOG_TIMER_MAX_INTERVAL_US);
}
static inline int stepperRampTimeMs(int accelTime) {
  if (accelTime <= 0) return 0;
  return constrain(accelTime, MOTION_RAMP_MIN_MS, MOTION_RAMP_MAX_MS);
}
static inline float stepperPulseRateFromInterval(int intervalUs) {
  intervalUs = stepperIntervalUsForSpeed(intervalUs);
  return 500000.0f / (float)intervalUs;
}
static inline int stepperIntervalFromPulseRate(float pulseRate) {
  float maxRate = 500000.0f / (float)stepperMinIntervalUs();
  pulseRate = constrain(pulseRate, 500000.0f / (float)JOG_TIMER_MAX_INTERVAL_US, maxRate);
  return (int)round(500000.0f / pulseRate);
}
static inline int stepperSmoothInterval(int fromInterval, int toInterval, float progress) {
  progress = constrain(progress, 0.0f, 1.0f);
  // شیب نمایی سرعت: rate = from * (to/from)^t
  float fromRate = stepperPulseRateFromInterval(fromInterval);
  float toRate = stepperPulseRateFromInterval(toInterval);
  float rate;
  if (fromRate < 1.0f) fromRate = 1.0f;
  if (toRate < 1.0f) toRate = 1.0f;
  if (fabsf(fromRate - toRate) < 0.5f) {
    rate = toRate;
  } else {
    rate = fromRate * powf(toRate / fromRate, progress);
  }
  return stepperIntervalFromPulseRate(rate);
}

static inline int jogReverseNormalDecelMs(int fromInterval, int stopInterval,
                                          int cruiseInterval, int rampMs) {
  float rStop = stepperPulseRateFromInterval(stopInterval);
  float rCruise = stepperPulseRateFromInterval(cruiseInterval);
  float rNow = stepperPulseRateFromInterval(fromInterval);
  float frac = (rCruise > rStop + 0.5f) ? (rNow - rStop) / (rCruise - rStop) : 1.0f;
  int duration = (int)lroundf((float)rampMs * constrain(frac, 0.0f, 1.0f));
  return (duration < 1) ? 1 : duration;
}

static inline int jogReverseNormalTravelPulses(int fromInterval, int stopInterval,
                                               int durationMs) {
  float rFrom = stepperPulseRateFromInterval(fromInterval);
  float rStop = stepperPulseRateFromInterval(stopInterval);
  float seconds = (float)durationMs / 1000.0f;
  float travel;
  if (fabsf(rFrom - rStop) < 0.5f) {
    travel = rStop * seconds;
  } else {
    travel = seconds * (rFrom - rStop) / logf(rFrom / rStop);
  }
  int pulses = (int)lroundf(travel);
  return (pulses < 1) ? 1 : pulses;
}

static inline int stepperProfileInterval(int completedPulses, int remainingPulses,
                                        int accelRampPulses, int decelRampPulses,
                                        int cruiseInterval, int initInterval) {
  if (accelRampPulses <= 0 && decelRampPulses <= 0) return cruiseInterval;

  int interval = cruiseInterval;
  if (accelRampPulses > 0 && completedPulses < accelRampPulses) {
    float progress = (float)completedPulses / (float)accelRampPulses;
    interval = max(interval, stepperSmoothInterval(initInterval, cruiseInterval, progress));
  }
  if (decelRampPulses > 0 && remainingPulses <= decelRampPulses) {
    int r = max(remainingPulses - 1, 0);
    float progress = 1.0f - ((float)r / (float)decelRampPulses);
    interval = max(interval, stepperSmoothInterval(cruiseInterval, initInterval, progress));
  }

  return constrain(interval, cruiseInterval, initInterval);
}
static inline int stepperRampPulses(int travelPulses, int cruiseSpeed, int initSpeed, int accelTime, bool finalApproach) {
  (void)finalApproach;
  if (travelPulses <= 2) return 1;
  int accelMs = stepperRampTimeMs(accelTime);
  if (accelMs <= 0) return 0;  // بدون شیب
  // انتگرال نرخ نمایی در بازهٔ T: T*(R1-R0)/ln(R1/R0) — زمان واقعی ≈ accelMs
  float startRate = stepperPulseRateFromInterval(initSpeed);
  float cruiseRate = stepperPulseRateFromInterval(cruiseSpeed);
  float T = (float)accelMs / 1000.0f;
  int timedRamp;
  if (fabsf(cruiseRate - startRate) < 1.0f) {
    timedRamp = (int)round(cruiseRate * T);
  } else {
    timedRamp = (int)round(T * (cruiseRate - startRate) / logf(cruiseRate / startRate));
  }
  if (timedRamp < 1) timedRamp = 1;
  int ramp = timedRamp;
  if (ramp > (travelPulses / 2)) ramp = travelPulses / 2;
  return (ramp < 1) ? 1 : ramp;
}
// یک پای حرکت؛ پرچم motorIs_moving / mvm_interupt را والد (manualMovement) مدیریت می‌کند
// تا بین دو پای جبران‌لقی تاچ‌استاپ از دست نرود.
// softApproachMm>0: کروز تا N mm آخر، بعد شیب نرم (seek بعد از جاگ)
static bool runManualMoveLeg(int targetPulse, int targetSpeed, int init_speed, int accelTime,
                             bool finalApproach = false, bool disableDriver = true,
                             float softApproachMm = 0.0f) {
  STP.position_Setpoint_p = targetPulse;
  // استاپ/اینتراپت فوری — ولی خام سنسور اینجا نبند (خروج از حداقل/سنسور با escape)
  if (STP.mvm_interupt || IOS.stop_button_state) {
    stepperStopMotion(true);
    return false;
  }
  if (STP.currentPosition_p == targetPulse) {
    return true;
  }
  int travelPulses = 0;
  if (STP.currentPosition_p <= targetPulse) {
    STP.system_set_dir = +1 * STP.system_base_dir;
    travelPulses = targetPulse - STP.currentPosition_p;
  } else {
    STP.system_set_dir = -1 * STP.system_base_dir;
    travelPulses = STP.currentPosition_p - targetPulse;
  }
  // فقط اجازهٔ دور شدن از سنسور فشرده — نه حرکت به داخل دیوار
  {
    const bool toward_plus = (STP.system_set_dir * STP.system_base_dir == 1);
    if (IOS.ref_sensor_state && toward_plus && limit_escape_sensor == 0)
      limit_escape_sensor = 1;
    else if (IOS.up_lim_sensor_state && !toward_plus && limit_escape_sensor == 0)
      limit_escape_sensor = 2;
  }
  if (stepperStopRequested()) {
    stepperLatchTripIfAny();
    stepperStopMotion(true);
    return false;
  }
  totalPulses = travelPulses;
  targetSpeed = stepperIntervalUsForSpeed(targetSpeed);
  init_speed = constrain(stepperIntervalUsForSpeed(init_speed), targetSpeed, JOG_TIMER_MAX_INTERVAL_US);
  int cruiseSpeed = targetSpeed;
  int accelRampPulses = stepperRampPulses(travelPulses, cruiseSpeed, init_speed, accelTime, finalApproach);
  int decelRampPulses = accelRampPulses;
  if (softApproachMm > 0.05f && accelTime > 0) {
    float ppmm = stepperPulsesPerMm();
    if (ppmm < 1.0f) ppmm = 1.0f;
    int approach_p = (int)lroundf(softApproachMm * ppmm);
    if (approach_p < 1) approach_p = 1;
    // قبلی: decel را به ۱۵mm کوتاه می‌کرد → با سرعت بالا روی مقصد یک‌دفعه می‌ایستاد
    // درست: شیب توقف زمان‌محور را کوتاه نکن؛ فقط اگر خیلی کوتاه بود حداقل approach بگذار
    if (decelRampPulses < approach_p) {
      int room = travelPulses - accelRampPulses;
      if (room > 1) {
        decelRampPulses = approach_p;
        if (decelRampPulses > room) decelRampPulses = room;
      }
    }
    if (accelRampPulses + decelRampPulses > travelPulses) {
      int room = travelPulses - decelRampPulses;
      if (room < 1) {
        decelRampPulses = travelPulses / 2;
        if (decelRampPulses < 1) decelRampPulses = 1;
        accelRampPulses = travelPulses - decelRampPulses;
      } else if (accelRampPulses > room) {
        accelRampPulses = room;
      }
    }
  }
  int activeSpeed = init_speed;
  vTaskDelay(10 / portTICK_PERIOD_MS);
  if (STP.mvm_interupt || IOS.stop_button_state) {
    stepperStopMotion(true);
    return false;
  }
  if (!stepperArmMotion(1, STP.system_set_dir, STP.system_set_dir == 1, activeSpeed)) {
    stepperLatchTripIfAny();
    stepperStopMotion(true);
    return false;
  }
  isr_clutch = true;
  while (isr_clutch) {
    if (escapeAwayWatchTick()) {
      STP.mvm_interupt = true;
      stepperStopMotion(true);
      isr_clutch = false;
      break;
    }
    // جاگ وسط seek بعد از رها: فوری abort تا jogMotor بگیرد
    if (STP.jogEnabled && (GUI.NumLock || IOS.joystick_state_L || IOS.joystick_state_R)) {
      STP.mvm_interupt = true;
      stepperStopMotion(true);
      isr_clutch = false;
      break;
    }
    int remainingPulses = totalPulses;
    int completedPulses = travelPulses - remainingPulses;
    int newSpeed = stepperProfileInterval(completedPulses, remainingPulses,
                                          accelRampPulses, decelRampPulses,
                                          cruiseSpeed, init_speed);
    newSpeed = constrain(newSpeed, targetSpeed, init_speed);
    if (newSpeed != activeSpeed) {
      activeSpeed = newSpeed;
      jog_live_interval_us = activeSpeed;
      timerAlarm(timer, stepperTimerPeriodUs(activeSpeed), true, 0);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  // پایان موفق حتی اگر روی سنسور مقصد ایستاده باشیم (escape نباید reached را باطل کند)
  const bool aborted = STP.mvm_interupt || IOS.stop_button_state || (totalPulses > 0);
  const bool reached_target = (totalPulses <= 0) && !STP.mvm_interupt && !IOS.stop_button_state;
  // روی abort همیشه درایور را قطع کن؛ بین دو پای موفق لقی، EN را نگه دار
  stepperStopMotion(aborted ? true : disableDriver);
  vTaskDelay(2 / portTICK_PERIOD_MS);
  vTaskDelay(5 / portTICK_PERIOD_MS);
  return reached_target;
}
//******************************************************//
void IRAM_ATTR onTimer() {
  if (isr_clutch) {
    switch (movement_mode) {
      case (1):  //manual mode
        if (!pinState) {
          if (stepperStopRequested() || totalPulses <= 0) {
            stepperStopIsr();
            break;
          }
          stepperPulseHigh();
          pinState = true;
        } else {
          stepperPulseLow();
          pinState = false;
          STP.currentPosition_p += (STP.system_set_dir * STP.system_base_dir == 1) ? 1 : -1;
          static int32_t last_gui_update_position = 0;  // Persistent between calls
          if (abs(STP.currentPosition_p - last_gui_update_position) >= STP.drv_ppr / 10) {
            GUI.flag_update_lable1_2 = true;
            last_gui_update_position = STP.currentPosition_p;
          }
          totalPulses--;
          if (stepperStopRequested() || totalPulses <= 0) stepperStopIsr();
        }
        break;
      case (2): {  // jog mode
        bool jogLimitReached = false;
        if (STP.system_base_dir == 1) {
          if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == -1) jogLimitReached = true;
          if (STP.currentPosition_p >= STP.final_position_p && STP.system_set_dir == 1) jogLimitReached = true;
        } else if (STP.system_base_dir == -1) {
          if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == 1) jogLimitReached = true;
          if (STP.currentPosition_p >= STP.final_position_p && STP.system_set_dir == -1) jogLimitReached = true;
        }
        if (jogLimitReached) {
          stepperStopIsr();
          break;
        }
        // توقف نرم: چک !jogEnabled از ISR حذف شد — رهاکردن جوگ در jogMotor به مود 5 (decel) می‌رود.
        // توقف‌های ایمنی (سنسور/استاپ/interrupt) همچنان فوری از طریق stepperStopRequested.
        if (!pinState) {
          if (stepperStopRequested()) {
            stepperStopIsr();
            break;
          }
          stepperPulseHigh();
          pinState = true;
        } else {
          stepperPulseLow();
          pinState = false;
          STP.currentPosition_p += (STP.system_set_dir * STP.system_base_dir == 1) ? 1 : -1;
          static int32_t last_gui_update_position = 0;  // Persistent between calls
          if (abs(STP.currentPosition_p - last_gui_update_position) >= STP.drv_ppr / 10) {
            GUI.flag_update_lable1_2 = true;
            last_gui_update_position = STP.currentPosition_p;
          }
          if (stepperStopRequested()) stepperStopIsr();
        }
        break;
      }
      case (3):  // ref mode
        if (!pinState) {
          if (IOS.up_lim_sensor_state || IOS.stop_button_state || STP.mvm_interupt) {
            stepperStopIsr();
            break;
          }
          stepperPulseHigh();
          pinState = true;
        } else {
          stepperPulseLow();
          pinState = false;
          STP.currentPosition_p += (STP.system_set_dir * STP.system_base_dir == 1) ? 1 : -1;
          static int32_t last_gui_update_position = 0;  // Persistent between calls
          if (abs(STP.currentPosition_p - last_gui_update_position) >= STP.drv_ppr / 10) {
            GUI.flag_update_lable1_2 = true;
            last_gui_update_position = STP.currentPosition_p;
          }
          if (IOS.up_lim_sensor_state || IOS.stop_button_state || STP.mvm_interupt) stepperStopIsr();  // break if emg
          if (ref_stage == 1 && IOS.ref_sensor_state) stepperStopIsr();                                //break if stage 1 done
          if (ref_stage == 2 && !IOS.ref_sensor_state) stepperStopIsr();                               //break if stage 2 done
        }
        break;
      case (4):  // sampling mode
        if (!pinState) {
          if (stepperStopRequested() || totalPulses <= 0) {
            stepperStopIsr();
            break;
          }
          stepperPulseHigh();
          pinState = true;
        } else {
          stepperPulseLow();
          pinState = false;
          // FIX Bug2: system_base_dir اضافه شد (مثل سایر mode‌ها)
          STP.currentPosition_p += (STP.system_set_dir * STP.system_base_dir == 1) ? 1 : -1;
          static int32_t last_gui_update_position = 0;  // Persistent between calls
          if (abs(STP.currentPosition_p - last_gui_update_position) >= STP.drv_ppr / 10) {
            GUI.flag_update_lable1_2 = true;
            last_gui_update_position = STP.currentPosition_p;
          }
          totalPulses--;
          if (stepperStopRequested() || totalPulses <= 0) stepperStopIsr();
        }
        break;
      case (5): {  // jog decel mode (Bug3 fix) — مثل jog ولی بدون چک jogEnabled
        if (jog_reverse_stop_active &&
            ((jog_reverse_stop_dir > 0 && STP.currentPosition_p >= jog_reverse_stop_target_p) ||
             (jog_reverse_stop_dir < 0 && STP.currentPosition_p <= jog_reverse_stop_target_p))) {
          jog_reverse_stop_active = false;
          stepperStopIsr();
          break;
        }
        bool jogDecelLimitReached = false;
        if (STP.system_base_dir == 1) {
          if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == -1) jogDecelLimitReached = true;
          if (STP.currentPosition_p >= STP.final_position_p  && STP.system_set_dir == 1)  jogDecelLimitReached = true;
        } else if (STP.system_base_dir == -1) {
          if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == 1)  jogDecelLimitReached = true;
          if (STP.currentPosition_p >= STP.final_position_p  && STP.system_set_dir == -1) jogDecelLimitReached = true;
        }
        if (jogDecelLimitReached) { stepperStopIsr(); break; }
        if (!pinState) {
          if (stepperStopRequested()) { stepperStopIsr(); break; }
          stepperPulseHigh();
          pinState = true;
        } else {
          stepperPulseLow();
          pinState = false;
          STP.currentPosition_p += (STP.system_set_dir * STP.system_base_dir == 1) ? 1 : -1;
          if (jog_reverse_stop_active &&
              ((jog_reverse_stop_dir > 0 && STP.currentPosition_p >= jog_reverse_stop_target_p) ||
               (jog_reverse_stop_dir < 0 && STP.currentPosition_p <= jog_reverse_stop_target_p))) {
            jog_reverse_stop_active = false;
            stepperStopIsr();
            break;
          }
          static int32_t last_decel_pos = 0;
          if (abs(STP.currentPosition_p - last_decel_pos) >= STP.drv_ppr / 10) {
            GUI.flag_update_lable1_2 = true;
            last_decel_pos = STP.currentPosition_p;
          }
          if (stepperStopRequested()) stepperStopIsr();
        }
        break;
      }
    }
  }
}

void IRAM_ATTR onTimerFast() {
  if (!isr_clutch) return;
  if (critical_task_start_failed || sensor_alert_motion_lock) { stepperStopIsr(); return; }

  switch (movement_mode) {
    case 1: {  // manual
      static int32_t lastGuiPosition = 0;
      if (stepperStopRequested() || totalPulses <= 0) {
        stepperLatchTripIfAny();
        stepperStopIsr();
        break;
      }
      stepperEmitPulse();
      stepperUpdatePositionFromPulse(&lastGuiPosition);
      totalPulses--;
      if (stepperStopRequested() || totalPulses <= 0) {
        stepperLatchTripIfAny();
        stepperStopIsr();
      }
      break;
    }

    case 2: {  // jog
      if (stepperJogControlStale()) {
        stepperStopIsr();
        break;
      }
      bool limitReached = false;
      if (STP.system_base_dir == 1) {
        if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == -1) limitReached = true;
        if (STP.currentPosition_p >= STP.final_position_p && STP.system_set_dir == 1) limitReached = true;
      } else if (STP.system_base_dir == -1) {
        if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == 1) limitReached = true;
        if (STP.currentPosition_p >= STP.final_position_p && STP.system_set_dir == -1) limitReached = true;
      }
      if (limitReached || stepperStopRequested()) {
        stepperLatchTripIfAny();
        stepperStopIsr();
        break;
      }
      stepperEmitPulse();
      static int32_t lastGuiPosition = 0;
      stepperUpdatePositionFromPulse(&lastGuiPosition);
      if (stepperStopRequested()) {
        stepperLatchTripIfAny();
        stepperStopIsr();
      }
      break;
    }

    case 3: {  // reference — از escape استفاده نکن؛ منطق مرحله‌ای خودش
      if (IOS.up_lim_sensor_state || IOS.stop_button_state || STP.mvm_interupt || sensor_alert_motion_lock) {
        stepperStopIsr();
        break;
      }
      stepperEmitPulse();
      static int32_t lastGuiPosition = 0;
      stepperUpdatePositionFromPulse(&lastGuiPosition);
      if (IOS.up_lim_sensor_state || IOS.stop_button_state || STP.mvm_interupt || sensor_alert_motion_lock)
        stepperStopIsr();
      if (ref_stage == 1 && IOS.ref_sensor_state) stepperStopIsr();
      if (ref_stage == 2 && !IOS.ref_sensor_state) stepperStopIsr();
      break;
    }

    case 4: {  // sampling
      static int32_t lastGuiPosition = 0;
      const bool pitch_endstop_run = pitch_cal_stage == PC_SAMPLE && pitch_cal_has_end_sensor;
      const bool pitch_end_release = pitch_cal_stage == PC_END_RELEASE;
      if (pitch_endstop_run) {
        if (IOS.stop_button_state || STP.mvm_interupt || IOS.ref_sensor_state ||
            IOS.up_lim_sensor_state || sensor_alert_motion_lock) {
          if (IOS.up_lim_sensor_state) sensor_trip_which = 2;
          stepperStopIsr();
          break;
        }
      } else if (pitch_end_release) {
        if (IOS.stop_button_state || STP.mvm_interupt || IOS.ref_sensor_state ||
            !IOS.up_lim_sensor_state || sensor_alert_motion_lock) {
          stepperStopIsr();
          break;
        }
      } else if (stepperStopRequested() || totalPulses <= 0) {
        stepperStopIsr();
        break;
      }
      stepperEmitPulse();
      stepperUpdatePositionFromPulse(&lastGuiPosition);
      if (pitch_endstop_run) {
        totalPulses++;
        if (IOS.stop_button_state || STP.mvm_interupt || IOS.ref_sensor_state ||
            IOS.up_lim_sensor_state || sensor_alert_motion_lock) {
          if (IOS.up_lim_sensor_state) sensor_trip_which = 2;
          stepperStopIsr();
        }
      } else if (pitch_end_release) {
        if (IOS.stop_button_state || STP.mvm_interupt || IOS.ref_sensor_state ||
            !IOS.up_lim_sensor_state || sensor_alert_motion_lock) stepperStopIsr();
      } else {
        totalPulses--;
        if (stepperStopRequested() || totalPulses <= 0) stepperStopIsr();
      }
      break;
    }

    case 5: {  // jog decel
      if (stepperJogControlStale()) {
        stepperStopIsr();
        break;
      }
      if (jog_reverse_stop_active &&
          ((jog_reverse_stop_dir > 0 && STP.currentPosition_p >= jog_reverse_stop_target_p) ||
           (jog_reverse_stop_dir < 0 && STP.currentPosition_p <= jog_reverse_stop_target_p))) {
        jog_reverse_stop_active = false;
        stepperStopIsr();
        break;
      }
      bool limitReached = false;
      if (STP.system_base_dir == 1) {
        if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == -1) limitReached = true;
        if (STP.currentPosition_p >= STP.final_position_p && STP.system_set_dir == 1) limitReached = true;
      } else if (STP.system_base_dir == -1) {
        if (STP.currentPosition_p <= STP.initial_Position_p && STP.system_set_dir == 1) limitReached = true;
        if (STP.currentPosition_p >= STP.final_position_p && STP.system_set_dir == -1) limitReached = true;
      }
      if (limitReached || stepperStopRequested()) {
        stepperLatchTripIfAny();
        stepperStopIsr();
        break;
      }
      stepperEmitPulse();
      static int32_t lastGuiPosition = 0;
      stepperUpdatePositionFromPulse(&lastGuiPosition);
      if (jog_reverse_stop_active &&
          ((jog_reverse_stop_dir > 0 && STP.currentPosition_p >= jog_reverse_stop_target_p) ||
           (jog_reverse_stop_dir < 0 && STP.currentPosition_p <= jog_reverse_stop_target_p))) {
        jog_reverse_stop_active = false;
        stepperStopIsr();
        break;
      }
      if (stepperStopRequested()) {
        stepperLatchTripIfAny();
        stepperStopIsr();
      }
      break;
    }
  }
}
void manualMovement(int targetSpeed, float setpointin, int init_speed, int accelTime,
                    float softApproachMm = 0.0f, bool allowBacklash = true,
                    bool cancelIfJogEngaged = false) {
  STP.mvm_interupt = false;
  // رها→گرفتن دوباره ممکن است درست قبل از شروع seek رخ دهد.
  // بعد از پاک‌کردن interrupt دوباره jogEnabled را ببین تا درخواست تازه خورده نشود.
  if (cancelIfJogEngaged && STP.jogEnabled) {
    STP.moveStepperCMD = false;
    return;
  }
  movement_mode = 1;  // run ISR in manual mode
  // مقصد UI = setpointin ثابت. لقی ۵mm فقط مسیر موتور — Man_Sp_mm را دست نزن (پرش ممنوع).
  // اصلاح خطای فعال فقط روی هدف واقعی موتور اعمال می‌شود؛ مقصد UI ثابت می‌ماند.
  const float ui_mm = setpointin;
  const float dest_mm = blade_err_motor_target_mm(ui_mm);
  int targetPulse = (int)lroundf(stepperPulsesPerMm() * dest_mm);
  targetPulse = constrain(targetPulse, STP.initial_Position_p, STP.final_position_p);
  STP.position_Setpoint_p = targetPulse;
  STP.motorIs_moving = true;
  bool reached_target = false;
  const int start_pos = STP.currentPosition_p;
  const int back_pulses = backlashCompensationPulses();
  bool reverseMove = STP.currentPosition_p > targetPulse;  // چپ = عدد کوچکتر
  bool forced_left = false;
  if (force_left_backlash_settle) {
    force_left_backlash_settle = false;
    forced_left = true;
    reverseMove = true;  // نشست جاگ چپ: موتور لقی؛ UI ثابت
  }
  // لقی ۵mm فقط چپ. راست / برگشت اصلاحی بعد از جاگ راست: مستقیم روی مقصد (۰mm رد)
  const bool useBacklash = forced_left || (allowBacklash && reverseMove);

  if (start_pos == targetPulse && !useBacklash) {
    reached_target = true;
    stepperStopMotion(true);
    STP.motorIs_moving = false;
    STP.mvm_interupt = false;
    sensor_trip_which = 0;
    GUI.flag_update_lable1_2 = true;
    vTaskDelay(10 / portTICK_PERIOD_MS);
    STP.moveStepperCMD = false;
    save_last_pos_p();
    if (AUT.selected_count_val != -1) AUT.in_position_flag = true;
    if (AUT.selected_count_val == -1) STP.in_position_flag = true;
    vTaskDelay(10 / portTICK_PERIOD_MS);
    return;
  }

  if (useBacklash) {
    int compensationTarget = targetPulse - back_pulses;  // موتور ۵mm رد
    compensationTarget = constrain(compensationTarget, STP.initial_Position_p, STP.final_position_p);
    if (compensationTarget < targetPulse) {
      bool reached_compensation = runManualMoveLeg(compensationTarget, targetSpeed, init_speed, accelTime, false, false, 0.0f);
      if (escapeAwayWatchTick()) reached_compensation = false;
      if (reached_compensation && !STP.mvm_interupt) {
        STP.currentPosition_p = compensationTarget;
        reached_target = runManualMoveLeg(targetPulse, targetSpeed, init_speed, accelTime, true, true, softApproachMm);
        if (escapeAwayWatchTick()) reached_target = false;
      } else {
        reached_target = false;
        stepperStopMotion(true);
      }
    } else {
      reached_target = (start_pos == targetPulse)
        ? true
        : runManualMoveLeg(targetPulse, targetSpeed, init_speed, accelTime, true, true, softApproachMm);
    }
  } else {
    reached_target = runManualMoveLeg(targetPulse, targetSpeed, init_speed, accelTime, reverseMove, true, softApproachMm);
  }

  stepperStopMotion(true);
  STP.motorIs_moving = false;
  STP.mvm_interupt = false;
  if (reached_target) STP.currentPosition_p = targetPulse;
  const bool handed_to_new_jog = cancelIfJogEngaged && STP.jogEnabled;
  if (!handed_to_new_jog) {
    // مقصد UI قفل روی همان عدد صفحه؛ position_Setpoint_p هدف واقعی اصلاح‌شده است.
    STP.Man_Sp_mm = ui_mm;
    STP.position_Setpoint_p = targetPulse;
  }

  if (STP.currentPosition_p != start_pos || sensor_trip_which != 0)
    process_sensor_trip_after_motion(!reverseMove);

  GUI.flag_update_lable1_2 = true;
  vTaskDelay(10 / portTICK_PERIOD_MS);
  STP.moveStepperCMD = false;
  save_last_pos_p();
  if (!STP.jogEnabled) GUI.buzzer_cmd_long = true;
  if (AUT.selected_count_val != -1) AUT.in_position_flag = reached_target;
  if (AUT.selected_count_val == -1) STP.in_position_flag = reached_target;
  vTaskDelay(10 / portTICK_PERIOD_MS);
}
void jogMotor(int targetSpeed, int dirin, int accelTime, int init_speed) {
  // استارت/استاپ نرم جوگ؛ init_speed==0 یا accelTime==0 = آنی (سرعت نامی فوری، رها=ایست فوری)
  const bool seekSpeedHandoff = jog_seek_speed_handoff;
  jog_seek_speed_handoff = false;
  targetSpeed = stepperIntervalUsForSpeed(targetSpeed);
  const bool instant = (init_speed <= 0) || (accelTime <= 0);
  if (instant) {
    init_speed = targetSpeed;
    accelTime = 0;
  } else {
    init_speed = constrain(stepperIntervalUsForSpeed(init_speed), targetSpeed, 400);
  }
  // ابتدا/انتهای شیب از تنظیم سرعت راه‌اندازی می‌آید؛ عدد ثابت نیست.
  const int softStopInterval = init_speed;
  if (seekSpeedHandoff && !instant) {
    // ادامهٔ هم‌جهت وسط seek: از سرعت لحظه‌ای ادامه بده؛
    // زمان شتاب همان ramp تنظیم‌شده و سرعت توقف همان مقدار تنظیم‌شده می‌ماند.
    init_speed = constrain((int)jog_live_interval_us, targetSpeed, JOG_TIMER_MAX_INTERVAL_US);
  }
  // رها→دوباره همان جهت: از خزیدن، حتی اگر init از بیرون چیز دیگری گفته باشد
  if (jog_soft_restart && !instant) {
    jog_soft_restart = false;
    init_speed = softStopInterval;
  }
  STP.mvm_interupt = false;
  jog_reverse_stop_active = false;
  STP.motorIs_moving = true;
  if (jog_wish_dir == 0) jog_wish_dir = dirin;
  STP.system_set_dir = dirin;
  // قبل از هر پالس: اگر روی سنسور اول/آخر و درخواست به داخل دیوار است → صفر پالس
  {
    const bool toward_plus = (dirin * STP.system_base_dir == 1);
    if (IOS.ref_sensor_state && toward_plus) limit_escape_sensor = 1;
    else if (IOS.up_lim_sensor_state && !toward_plus) limit_escape_sensor = 2;
    if (stepperStopRequested()) {
      stepperLatchTripIfAny();
      stepperStopMotion(true);
      STP.motorIs_moving = false;
      STP.jogEnabled = false;
      process_sensor_trip_after_motion(toward_plus);
      return;
    }
  }
  vTaskDelay(5 / portTICK_PERIOD_MS);
  // دوباره چک — ADC ممکن است دیرتر بیاید
  if (stepperStopRequested()) {
    stepperLatchTripIfAny();
    stepperStopMotion(true);
    STP.motorIs_moving = false;
    STP.jogEnabled = false;
    process_sensor_trip_after_motion(dirin * STP.system_base_dir == 1);
    return;
  }
  movement_mode = 2;
  jog_control_watchdog_pulses = 0;
  if (!stepperArmMotion(2, dirin, dirin * STP.system_base_dir == 1, init_speed)) {
    stepperStopMotion(true);
    STP.motorIs_moving = false;
    STP.jogEnabled = false;
    process_sensor_trip_after_motion(dirin * STP.system_base_dir == 1);
    return;
  }
  isr_clutch = true;
  int rampMs = instant ? 0 : stepperRampTimeMs(accelTime);
  int accelRampMs = rampMs;
  int activeSpeed = init_speed;
  int accelFrom = init_speed;
  int cruiseGoal = jog_effective_cruise_us(targetSpeed);
  int lastCruiseGoal = cruiseGoal;
  bool fine_latched = jog_fine_held;
  jog_live_interval_us = activeSpeed;
  unsigned long accel_t0 = millis();
  bool decel = false;
  unsigned long decel_t0 = 0;
  int decelFrom = init_speed;
  int decelMs = rampMs;
  int decelStopInterval = softStopInterval;
  bool rev_pending = false;
  int pending_dir = dirin;
  int reverseNormalDecelMs = 1;
  int reverseNormalTravelPulses = 1;
  int32_t reverseStartPositionP = STP.currentPosition_p;
  int reverseTravelDir = (dirin * STP.system_base_dir >= 0) ? 1 : -1;
  uint64_t reverseProgressScaled = 0;
  uint32_t reverseProgressLastMs = 0;
  while (isr_clutch) {
    jog_control_watchdog_pulses = 0;
    if (escapeAwayWatchTick()) break;
    // جهت درخواستی UI (ممکن است هنوز روی DIR فیزیکی اعمال نشده باشد)
    const int wish = (jog_wish_dir == 1 || jog_wish_dir == -1) ? jog_wish_dir : STP.system_set_dir;
    // هدف کروز: نامی یا ÷۶ با کلید ۲ — هر تغییر با همان زمان شیب تنظیم‌شده.
    const bool fine_now = jog_fine_held;
    cruiseGoal = jog_effective_cruise_us(targetSpeed);
  // کلید۲ نیز مسیر عادی است؛ نه توقف معکوس. پس حق استفاده از شیب سریع‌تر را ندارد.
  if (!decel && !rev_pending &&
      (cruiseGoal != lastCruiseGoal || fine_now != fine_latched)) {
    accelFrom = activeSpeed;
    accel_t0 = millis();
    accelRampMs = (instant || rampMs <= 0) ? 0 : rampMs;
    lastCruiseGoal = cruiseGoal;
    fine_latched = fine_now;
    movement_mode = 2;
  }
    // وسط decel رها: اگر دوباره گرفت → از خزیدن شیب دوباره (نه ادامه از سرعت فعلی)
    if (jog_soft_restart && !instant && rampMs > 0 && STP.jogEnabled && !STP.mvm_interupt) {
      jog_soft_restart = false;
      decel = false;
      rev_pending = false;
      jog_backup_active = false;
      dirin = wish;
      STP.system_set_dir = dirin;
      jog_wish_dir = dirin;
      const int restartInterval = max(softStopInterval, cruiseGoal);
      activeSpeed = restartInterval;
      accelFrom = restartInterval;
      accel_t0 = millis();
      accelRampMs = rampMs;
      lastCruiseGoal = cruiseGoal;
      fine_latched = jog_fine_held;
      movement_mode = 2;
      if (!stepperArmMotion(2, dirin, dirin * STP.system_base_dir == 1, restartInterval)) {
        STP.mvm_interupt = true;
        break;
      }
      isr_clutch = true;
      jog_live_interval_us = activeSpeed;
      timerAlarm(timer, stepperTimerPeriodUs(activeSpeed), true, 0);
      jog_pred_dir = (dirin * STP.system_base_dir >= 0) ? 1 : -1;
      jog_pred_dir_ms = 0;
      GUI.update_lable1_1 = true;
    }
    int desired = activeSpeed;
    if (!decel) {
      if (!STP.jogEnabled) {
        if (instant || rampMs <= 0) break;
        decel = true;
        rev_pending = false;
        decel_t0 = millis();
        decelFrom = activeSpeed;
        decelStopInterval = max(softStopInterval, decelFrom);
        float rInit = stepperPulseRateFromInterval(decelStopInterval);
        float rTarget = stepperPulseRateFromInterval(targetSpeed);
        float rNow = stepperPulseRateFromInterval(decelFrom);
        float frac = (rTarget > rInit + 0.5f) ? (rNow - rInit) / (rTarget - rInit) : 1.0f;
        decelMs = (int)lroundf((float)rampMs * constrain(frac, 0.0f, 1.0f));
        if (decelMs < 1) decelMs = 1;
        float ppmm = axisPulsesPerMm();
        if (ppmm < 1.0f) ppmm = 1.0f;
        const int stop_target_p = (int)lroundf(STP.Man_Sp_mm * ppmm);
        const int physical_dir = (dirin * STP.system_base_dir >= 0) ? 1 : -1;
        jog_reverse_stop_target_p = stop_target_p;
        jog_reverse_stop_dir = (int8_t)physical_dir;
        jog_reverse_stop_active =
          ((stop_target_p - STP.currentPosition_p) * physical_dir) > 0;
        movement_mode = 5;
      } else if (instant || rampMs <= 0) {
        // بدون شیب تنظیمات: فقط همان لحظه مجاز است
        if (wish != dirin) {
          jog_backup_active = false;
          dirin = wish;
          STP.system_set_dir = dirin;
          jog_wish_dir = dirin;
          if (!stepperArmMotion(2, dirin, dirin * STP.system_base_dir == 1, activeSpeed)) {
            STP.mvm_interupt = true;
            break;
          }
          isr_clutch = true;
          jog_release_seek_pending = false;
          jog_pred_dir = (dirin * STP.system_base_dir >= 0) ? 1 : -1;
          jog_pred_dir_ms = 0;
          GUI.update_lable1_1 = true;
        }
        desired = cruiseGoal;
      } else {
        // عوض جهت با شیب: توقف تطبیقی ۱×..۲×؛ شروع جهت جدید با شیب تنظیم‌شده.
        if (wish != dirin) {
          pending_dir = wish;
          rev_pending = true;
          jog_backup_active = true;
          jog_reverse_stop_active = false;
          decel = true;
          decel_t0 = millis();
          decelFrom = activeSpeed;
          decelStopInterval = max(softStopInterval, decelFrom);
          reverseNormalDecelMs =
            jogReverseNormalDecelMs(decelFrom, decelStopInterval, targetSpeed, rampMs);
          reverseNormalTravelPulses =
            jogReverseNormalTravelPulses(decelFrom, decelStopInterval, reverseNormalDecelMs);
          reverseStartPositionP = STP.currentPosition_p;
          reverseTravelDir = (dirin * STP.system_base_dir >= 0) ? 1 : -1;
          reverseProgressScaled = 0;
          reverseProgressLastMs = decel_t0;
          decelMs = reverseNormalDecelMs;
          movement_mode = 5;
        } else {
          float p = (accelRampMs > 0) ? ((float)(millis() - accel_t0) / (float)accelRampMs) : 1.0f;
          if (p > 1.0f) p = 1.0f;
          desired = stepperSmoothInterval(accelFrom, cruiseGoal, p);
        }
      }
    }
    if (decel) {
      if (rev_pending && STP.jogEnabled && !STP.mvm_interupt &&
          !IOS.stop_button_state && wish == dirin) {
        // کاربر پیش از توقف به جهت فیزیکی فعلی برگشت؛ معکوس‌کردن فوراً لغو شود.
        jog_reverse_stop_active = false;
        jog_backup_active = false;
        rev_pending = false;
        decel = false;
        movement_mode = 2;
        accelFrom = activeSpeed;
        accel_t0 = millis();
        accelRampMs = rampMs;
        desired = activeSpeed;
      } else if (STP.jogEnabled && !STP.mvm_interupt && !IOS.stop_button_state && !rev_pending) {
        if (wish != dirin) {
          pending_dir = wish;
          rev_pending = true;
          jog_backup_active = true;
          jog_reverse_stop_active = false;
          decel_t0 = millis();
          decelFrom = activeSpeed;
          decelStopInterval = max(softStopInterval, decelFrom);
          reverseNormalDecelMs =
            jogReverseNormalDecelMs(decelFrom, decelStopInterval, targetSpeed, rampMs);
          reverseNormalTravelPulses =
            jogReverseNormalTravelPulses(decelFrom, decelStopInterval, reverseNormalDecelMs);
          reverseStartPositionP = STP.currentPosition_p;
          reverseTravelDir = (dirin * STP.system_base_dir >= 0) ? 1 : -1;
          reverseProgressScaled = 0;
          reverseProgressLastMs = decel_t0;
          decelMs = reverseNormalDecelMs;
          movement_mode = 5;
        } else {
          jog_reverse_stop_active = false;
          // ادامهٔ همان جهت وسط decel: اگر soft_restart نبود، از سرعت فعلی؛ وگرنه بالاتر هندل شد
          decel = false;
          movement_mode = 2;
          accelFrom = activeSpeed;
          accel_t0 = millis();
          accelRampMs = rampMs;
          desired = activeSpeed;
        }
      } else {
        float p;
        if (rev_pending) {
          const int32_t targetP = STP.position_Setpoint_p;
          const int32_t targetTravelP =
            (targetP - reverseStartPositionP) * reverseTravelDir;
          int reverseBrakePermille;
          if (targetTravelP <= 0) {
            reverseBrakePermille = 2000;
          } else if (targetTravelP >= reverseNormalTravelPulses) {
            reverseBrakePermille = 1000;
          } else {
            reverseBrakePermille =
              (int)(((int64_t)reverseNormalTravelPulses * 1000 + targetTravelP / 2) /
                    targetTravelP);
            reverseBrakePermille = constrain(reverseBrakePermille, 1000, 2000);
          }
          const uint32_t nowMs = millis();
          const uint32_t deltaMs = nowMs - reverseProgressLastMs;
          reverseProgressLastMs = nowMs;
          reverseProgressScaled += (uint64_t)deltaMs * (uint32_t)reverseBrakePermille;
          p = (float)reverseProgressScaled /
              ((float)reverseNormalDecelMs * 1000.0f);
        } else {
          p = (float)(millis() - decel_t0) / (float)decelMs;
        }
        if (p > 1.0f) p = 1.0f;
        desired = stepperSmoothInterval(decelFrom, decelStopInterval, p);
        if (p >= 1.0f) {
          if (rev_pending && STP.jogEnabled && !STP.mvm_interupt && !IOS.stop_button_state) {
            // اگر مقصد زنده هنوز در مسیر فعلی است، با سرعت توقف تا خود مقصد ادامه بده.
            // اگر مقصد رد شده است، همین‌جا توقف و جهت را عوض کن.
            if (jog_backup_active && rampMs > 0) {
              const int net = dirin * STP.system_base_dir;
              desired = decelStopInterval;
              activeSpeed = decelStopInterval;
              jog_live_interval_us = activeSpeed;
              timerAlarm(timer, stepperTimerPeriodUs(activeSpeed), true, 0);
              jog_reverse_stop_dir = (net >= 0) ? 1 : -1;
              while (isr_clutch && STP.jogEnabled && !STP.mvm_interupt && !IOS.stop_button_state) {
                jog_control_watchdog_pulses = 0;
                const int wish2 = (jog_wish_dir == 1 || jog_wish_dir == -1) ? jog_wish_dir : pending_dir;
                if (wish2 == dirin) {
                  jog_reverse_stop_active = false;
                  jog_backup_active = false;
                  rev_pending = false;
                  decel = false;
                  movement_mode = 2;
                  accelFrom = activeSpeed;
                  accel_t0 = millis();
                  accelRampMs = rampMs;
                  break;
                }
                const int32_t liveTargetP = STP.position_Setpoint_p;
                const int32_t liveGapP =
                  (liveTargetP - STP.currentPosition_p) * jog_reverse_stop_dir;
                if (liveGapP <= 0) break;
                jog_reverse_stop_target_p = liveTargetP;
                jog_reverse_stop_active = true;
                vTaskDelay(1 / portTICK_PERIOD_MS);
              }
              jog_reverse_stop_active = false;
              if (!STP.jogEnabled || STP.mvm_interupt || IOS.stop_button_state) {
                jog_backup_active = false;
                break;
              }
              if (!rev_pending) {
                // لغو عوض‌جهت — حلقهٔ اصلی ادامه
                desired = activeSpeed;
                continue;
              }
            }
            // توقف کامل؛ جهت جدید بدون مسیر لقی ۵mm آرم می‌شود.
            stepperStopMotion(false);
            rev_pending = false;
            jog_backup_active = false;
            dirin = pending_dir;
            STP.system_set_dir = dirin;
            jog_wish_dir = dirin;
            const int restartInterval = max(softStopInterval, jog_effective_cruise_us(targetSpeed));
            if (!stepperArmMotion(2, dirin, dirin * STP.system_base_dir == 1, restartInterval)) {
              STP.mvm_interupt = true;
              break;
            }
            isr_clutch = true;
            decel = false;
            movement_mode = 2;
            activeSpeed = restartInterval;
            accelFrom = restartInterval;
            accel_t0 = millis();
            accelRampMs = rampMs;
            lastCruiseGoal = jog_effective_cruise_us(targetSpeed);
            cruiseGoal = lastCruiseGoal;
            fine_latched = jog_fine_held;
            jog_release_seek_pending = false;
            jog_pred_dir = (dirin * STP.system_base_dir >= 0) ? 1 : -1;
            jog_pred_dir_ms = 0;  // بدون fade؛ مقصد از قبل با grow روان است
            GUI.update_lable1_1 = true;
            desired = activeSpeed;
          } else {
            jog_backup_active = false;
            if (jog_reverse_stop_active && isr_clutch && !STP.jogEnabled &&
                !STP.mvm_interupt && !IOS.stop_button_state) {
              desired = decelStopInterval;
            } else {
              break;
            }
          }
        }
      }
    }
    if (desired != activeSpeed) {
      activeSpeed = desired;
      jog_live_interval_us = activeSpeed;
      timerAlarm(timer, stepperTimerPeriodUs(activeSpeed), true, 0);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  jog_live_interval_us = softStopInterval;
  jog_reverse_stop_active = false;
  stepperStopMotion();
  process_sensor_trip_after_motion(dirin * STP.system_base_dir == 1);
  // حد نرم تنظیمات (بدون سنسور فیزیکی) — فقط اگر سنسور فیزیکی نبود
  if (!limit_recal_flag && !ref_wrong_dir_flag &&
      !IOS.stop_button_state && !STP.mvm_interupt) {
    if (STP.currentPosition_p <= STP.initial_Position_p)
      notify_jog_soft_travel_limit(1);
    else if (STP.currentPosition_p >= STP.final_position_p)
      notify_jog_soft_travel_limit(2);
    else
      s_jog_soft_limit_latched = 0;
  }
  vTaskDelay(5 / portTICK_PERIOD_MS);
  save_last_pos_p();
  vTaskDelay(5 / portTICK_PERIOD_MS);
  GUI.flag_update_lable1_2 = true;
  vTaskDelay(5 / portTICK_PERIOD_MS);
  STP.motorIs_moving = false;
  if (jog_release_seek_pending && GUI.NumLock && STP.referenced && !STP.flag_is_cutting &&
      !STP.mvm_interupt && !IOS.stop_button_state && !sensor_alert_motion_lock &&
      !limit_recal_flag && !ref_wrong_dir_flag && !sensor_stuck_escape_flag) {
    jog_release_seek_pending = false;
    float ppmm = axisPulsesPerMm();
    // فقط تصمیمِ نیاز به seek: همان هدف واقعی اصلاح‌شده‌ای که manualMovement می‌گیرد.
    // قبلاً توقف دقیق روی مقصد UI، seek آفست را اشتباهاً حذف می‌کرد.
    int target_p = blade_err_motor_target_p(STP.Man_Sp_mm);
    int err = STP.currentPosition_p - target_p;
    if (err < 0) err = -err;
    // رهاسازی وسط توقف تعویض جهت، نشست انتقالی است و لقی ۵mm ندارد.
    // بعد از کامل‌شدن تعویض جهت، قانون عادی چپ/راست بدون تغییر اجرا می‌شود.
    const bool jogged_left = !rev_pending && (dirin * STP.system_base_dir) < 0;
    jog_seek_allow_backlash = jogged_left;
    force_left_backlash_settle = jogged_left;
    if (err > 0 || jogged_left) {
      STP.moveStepperCMD = true;
      jog_seek_soft = true;
      STP.in_position_flag = false;
      AUT.in_position_flag = false;
    } else {
      STP.moveStepperCMD = false;
      jog_seek_soft = false;
      GUI.flag_update_lable1_2 = true;
    }
  } else {
    jog_release_seek_pending = false;
  }
}
void Reference_Pos() {
  // Independent safety gate: no Home pulse if I0 was disconnected after UI confirmation.
  if (!pinmap_in_role_connected(0)) {
    STP.goref_cmd = false;
    STP.moveStepperCMD = false;
    STP.motorIs_moving = false;
    isr_clutch = false;
    stepperStopMotion();
    return;
  }
  // مهلت کل موقعیت‌یابی: 100%→۳۰ث ، ۵۰%→۶۰ث (متناسب با اسلایدر F2)
  // حداقل سرعت مؤثر ۲۵٪ تا مهلت از ~۲ دقیقه بیشتر نشود (قبلاً با اسلایدر خیلی پایین تا ۵۰د طول می‌کشید)
  // STP.global_speed: 8=سریع‌ترین(100%) … 100=کند(0%)
  int spd_pct = (int)map(constrain(STP.global_speed, 8, 100), 100, 8, 0, 100);
  if (spd_pct < 25) spd_pct = 25;
  if (spd_pct > 100) spd_pct = 100;
  const unsigned long REF_TIMEOUT_MS = (30000UL * 100UL) / (unsigned long)spd_pct;
  // رویکرد = سرعت تنظیم کاربر؛ برگشت = خزش آهسته تا رها شدن سنسور
  const int REF_FAST_US  = stepperIntervalUsForSpeed(STP.global_speed);
  const int REF_CREEP_US = 200;
  // حین رفرنس ISR پوزیشن را جابه‌جا می‌کند؛ اگر شکست خورد باید همان عدد قبل برگردد (نه منفی/خراب)
  const int32_t pos_before_ref = STP.currentPosition_p;
  STP.mvm_interupt = false;
  STP.goref_cmd = false;
  movement_mode = 3;  // run isr in ref mode
  STP.motorIs_moving = true;
  bool ref_hit_upper_lim = false;  // سنسور آخر خط → جهت/جای سنسور اشتباه
  bool ref_timed_out = false;
  const unsigned long ref_deadline = millis() + REF_TIMEOUT_MS;
  vTaskDelay(100 / portTICK_PERIOD_MS);
  // مرحله ۱: رویکرد با سرعت کاربر به سمت سنسور اول خط
  ref_stage = 1;
  stepperArmMotion(3, -STP.system_base_dir, STP.system_base_dir == -1, REF_FAST_US);
  isr_clutch = true;
  while (isr_clutch) {
    if ((int32_t)(millis() - ref_deadline) >= 0) {
      STP.mvm_interupt = true;
      ref_timed_out = true;
      stepperStopIsr();
      vTaskDelay(10 / portTICK_PERIOD_MS);
      break;
    }
    if (IOS.stop_button_state || STP.mvm_interupt) {
      STP.mvm_interupt = true;
      stepperStopIsr();
      vTaskDelay(10 / portTICK_PERIOD_MS);
      break;
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  bool ref_aborted = STP.mvm_interupt || IOS.stop_button_state || IOS.up_lim_sensor_state;
  if (IOS.up_lim_sensor_state) ref_hit_upper_lim = true;
  // مرحله ۲: برگشت آهسته تا سنسور رها شود → همان‌جا صفر و خروج (بدون مرحلهٔ سوم)
  if (!ref_aborted) {
    ref_stage = 2;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    stepperArmMotion(3, STP.system_base_dir, STP.system_base_dir == 1, REF_CREEP_US);
    isr_clutch = true;
    while (isr_clutch) {
      if ((int32_t)(millis() - ref_deadline) >= 0) {
        STP.mvm_interupt = true;
        ref_timed_out = true;
        stepperStopIsr();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        break;
      }
      if (IOS.stop_button_state || STP.mvm_interupt) {
        STP.mvm_interupt = true;
        stepperStopIsr();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        break;
      }
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    ref_aborted = STP.mvm_interupt || IOS.stop_button_state || IOS.up_lim_sensor_state;
    if (IOS.up_lim_sensor_state) ref_hit_upper_lim = true;
    // اگر هنوز سنسور فشرده است، رهاسازی کامل نشده
    if (!ref_aborted && IOS.ref_sensor_state) ref_aborted = true;
  }
  stepperStopMotion();
  // بعد از تایم‌اوت مطمئن شو ISR واقعاً رها شده (جلوگیری از گیر کردن clutch)
  if (ref_timed_out) {
    unsigned long wait_until = millis() + 300UL;
    while (isr_clutch && (int32_t)(millis() - wait_until) < 0) {
      isr_clutch = false;
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    isr_clutch = false;
  }
  if (!ref_aborted && !STP.begin_calibration) {  // if microswitch hit naturally
    vTaskDelay(10 / portTICK_PERIOD_MS);
    STP.currentPosition_p = STP.initial_Position_p;
    STP.referenced = true;
    STP.in_position_flag = true;
    // مقصد = موقعیت فعلی بعد از رفرنس (طول اولیه)
    {
      float ppmm = axisPulsesPerMm();
      if (ppmm < 1.0f) ppmm = 1.0f;
      int cur_mm = (int)lroundf((float)STP.currentPosition_p / ppmm);
      GUI.inputBuffer_sp = String(cur_mm);
      STP.Man_Sp_mm = (float)cur_mm;
      STP.position_Setpoint_p = STP.initial_Position_p;
      GUI.update_lable1_1 = true;
    }
    GUI.flag_update_lable1_2 = true;
    vTaskDelay(20 / portTICK_PERIOD_MS);
    save_last_pos_p();
    STP.motorIs_moving = false;
    STP.mvm_interupt = false;
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_COMPLETE);  // اعلام موفقیت موقعیت‌یابی
    vTaskDelay(10 / portTICK_PERIOD_MS);
    GUI.edit_mode_lable1_1 = (!GUI.NumLock) ? true : false;
    // تعویض صفحه فقط در Task2 — نه loadScreen مستقیم از تسک حرکت
    GUI.pending_goto_scr = 1;
  } else if (!ref_aborted && STP.begin_calibration) {  //for initial calib
    vTaskDelay(10 / portTICK_PERIOD_MS);
    STP.calib_stage = 4;
    STP.referenced = true;
    STP.in_position_flag = true;
    GUI.flag_update_lable1_2 = true;
    vTaskDelay(20 / portTICK_PERIOD_MS);
    save_last_pos_p();
    STP.motorIs_moving = false;
    STP.mvm_interupt = false;
    GUI.edit_mode_lable2_3 = true;
    GUI.buzzer_cmd_long = true;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  } else {
    // شکست رفرنس: موقعیت نمایشی/منطقی را به قبل از حرکت برگردان (موتور ممکن است جابه‌جا شده باشد
    // استپ پنل/امرجنسی قبل از پاک‌کردن mvm_interupt ثبت شود
    const bool user_abort = IOS.stop_button_state || STP.mvm_interupt;
    // محاسبه دقیق موقعیت: پالس‌های طی‌شده توسط ISR به پوزیشن اضافه/کم شده‌اند
    // موقعیت واقعی موتور حفظ می‌شود تا کالیبراسیون و جایگاه دستگاه از دست نرود.
    GUI.flag_update_lable1_2 = true;
    save_last_pos_p();
    STP.goref_cmd = false;
    STP.moveStepperCMD = false;
    STP.jogEnabled = false;
    STP.motorIs_moving = false;
    STP.mvm_interupt = false;
    ref_stage = 0;
    if (ref_hit_upper_lim) {
      ref_wrong_dir_flag = true;  // به سنسور آخر خط خورد
      ref_wrong_dir_sensor = 2;
    } else if (ref_timed_out) {
      ref_failed_flag = true;
      ref_fail_reason = 1;       // مهلت تمام — موتور/اتصال
    }
    GUI.buzzer_cmd_long = true;
    GUI.lock_screen = false;
    // اگر در صفحه تنظیمات یا F2 بود یا کاربر آگاهانه متوقف کرد، کاربر را به صفحه اول پرتاب نکن
    if (!STP.begin_calibration && !user_abort && GUI.scr_id != 2) {
      STP.referenced = false;
      GUI.pending_goto_scr = 0;  // فقط در خطای واقعی بوت به صفحه موقعیت‌یابی برو
    }
  }
}
void go_sampling(int speedin, int setpoint, int dirin) {
  STP.mvm_interupt = false;
  movement_mode = 4;  // run isr in sampling mode
  totalPulses = setpoint;
  STP.motorIs_moving = true;
  vTaskDelay(10 / portTICK_PERIOD_MS);
  stepperArmMotion(4, dirin, dirin * STP.system_base_dir == 1, speedin);
  isr_clutch = true;
  while (isr_clutch) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  stepperStopMotion();
  STP.calib_stage = 2;
  STP.go_sampling_CMD = false;
  GUI.buzzer_cmd_long = true;
  GUI.flag_update_lable1_2 = true;
  vTaskDelay(10 / portTICK_PERIOD_MS);
  save_last_pos_p();
  STP.motorIs_moving = false;
  STP.mvm_interupt = false;
  GUI.edit_mode_lable2_2 = true;
  vTaskDelay(50 / portTICK_PERIOD_MS);
}

// ===== F4 pitch calibration motion =====
static unsigned long pitchCalTravelTimeoutMs(int32_t commanded_pulses, int interval_us) {
  if (commanded_pulses < 1) commanded_pulses = 1;
  const double pulses = (double)commanded_pulses;
  const double rate = (double)stepperPulseRateFromInterval(interval_us);
  double ms = (rate > 1.0) ? (pulses * 1000.0 / rate) : 0.0;
  ms = ms * 1.25 + 5000.0;  // 25% motion margin + settle/start allowance
  if (ms < 50000.0) ms = 50000.0;
  if (ms > 3600000.0) ms = 3600000.0;
  return (unsigned long)lround(ms);
}

static int32_t pitchCalConfiguredSpanPulses() {
  const float span_mm = fabsf(STP.final_position_mm.toFloat() - pitch_cal_first_line_mm());
  const float ppmm = axisPulsesPerMm();
  if (!isfinite(span_mm) || !isfinite(ppmm) || span_mm < 1.0f || ppmm < 1.0f) return 1;
  const double pulses = (double)span_mm * (double)ppmm;
  return (pulses > 2147480000.0) ? 2147480000L : (int32_t)lround(pulses);
}

static int32_t pitchCalFifteenRevPulses() {
  if (STP.drv_ppr <= 0 || STP.drv_ppr > (2147480000L / 15L)) return 0;
  return (int32_t)STP.drv_ppr * 15L;
}

static inline bool pitchCalWaitForStop(unsigned long timeout_ms, bool *timed_out) {
  const unsigned long t0 = millis();
  while (isr_clutch) {
    if ((unsigned long)(millis() - t0) > timeout_ms) {
      if (timed_out) *timed_out = true;
      STP.mvm_interupt = true;
      break;
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  return !STP.mvm_interupt && !IOS.stop_button_state && !pitch_cal_cancel_requested;
}

// [RT/Task3] I0 seek at half configured pulse-rate, fixed 100 ms stop, then I0 release.
void pitch_cal_do_home() {
  static const int CREEP_US = 200;
  const int FAST_US = stepperIntervalUsForSpeed(STP.global_speed) * 2;
  const unsigned long TO = pitchCalTravelTimeoutMs(pitchCalConfiguredSpanPulses(), FAST_US);
  pitch_cal_motion_busy = true;
  STP.mvm_interupt = false;
  STP.motorIs_moving = true;
  STP.jogEnabled = false;
  STP.moveStepperCMD = false;
  bool wrong = IOS.up_lim_sensor_state;
  bool timed_out = false;
  movement_mode = 3;

  ref_stage = 1;
  if (!IOS.ref_sensor_state) {
    if (!stepperArmMotion(3, -STP.system_base_dir, STP.system_base_dir == -1, FAST_US)) {
      stepperStopMotion();
      pitch_cal_task3_fail(PCERR_BUSY);
      return;
    }
    isr_clutch = true;
    pitchCalWaitForStop(TO, &timed_out);
  }
  stepperStopMotion();
  bool abort = STP.mvm_interupt || IOS.stop_button_state || pitch_cal_cancel_requested || IOS.up_lim_sensor_state;
  if (IOS.up_lim_sensor_state) wrong = true;

  if (!abort) {
    ref_stage = 2;
    vTaskDelay(100 / portTICK_PERIOD_MS);  // controlled stop before reversal
    if (IOS.stop_button_state || pitch_cal_cancel_requested) abort = true;
  }
  if (!abort) {
    if (!stepperArmMotion(3, STP.system_base_dir, STP.system_base_dir == 1, CREEP_US)) {
      stepperStopMotion();
      pitch_cal_task3_fail(PCERR_BUSY);
      return;
    }
    isr_clutch = true;
    pitchCalWaitForStop(TO, &timed_out);
    abort = STP.mvm_interupt || IOS.stop_button_state || pitch_cal_cancel_requested || IOS.up_lim_sensor_state;
    if (IOS.up_lim_sensor_state) wrong = true;
    if (!abort && IOS.ref_sensor_state) { abort = true; pitch_cal_err = PCERR_SENSOR_STUCK; }
  }

  stepperStopMotion();
  if (abort) {
    pitch_cal_task3_fail(wrong ? PCERR_WRONG_DIR :
                           (pitch_cal_err == PCERR_SENSOR_STUCK ? PCERR_SENSOR_STUCK :
                           ((IOS.stop_button_state || pitch_cal_cancel_requested) ? PCERR_ABORT :
                            (timed_out ? PCERR_TIMEOUT : PCERR_ABORT))));
    return;
  }

  STP.currentPosition_p = STP.initial_Position_p;
  STP.referenced = true;
  STP.in_position_flag = true;
  GUI.flag_update_lable1_2 = true;
  save_last_pos_p();
  STP.motorIs_moving = false;
  STP.mvm_interupt = false;
  pitch_cal_motion_busy = false;
  GUI.buzzer_cmd_short = true;
  pitch_cal_stage = PC_SAMPLE;
}

// [RT/Task3] I1 path counts to I1, then releases I1 in a separate controlled stage.
// Without I1 the same ISR executes exactly 15*drv_ppr pulses and then requests mm.
void pitch_cal_do_sample() {
  const int sample_us = stepperIntervalUsForSpeed(STP.global_speed) * 2;
  const int32_t exact_pulses = pitch_cal_has_end_sensor ? 0 : pitchCalFifteenRevPulses();
  if (!pitch_cal_has_end_sensor && exact_pulses <= 0) {
    stepperStopMotion();
    pitch_cal_task3_fail(PCERR_BAD_MEASURE);
    return;
  }
  const int32_t commanded_pulses = pitch_cal_has_end_sensor ? pitchCalConfiguredSpanPulses() : exact_pulses;
  const unsigned long TO = pitchCalTravelTimeoutMs(commanded_pulses, sample_us);
  pitch_cal_motion_busy = true;
  STP.mvm_interupt = false;
  STP.motorIs_moving = true;
  const int32_t pos0 = STP.currentPosition_p;
  movement_mode = 4;
  totalPulses = pitch_cal_has_end_sensor ? 0 : exact_pulses;
  sensor_trip_which = 0;
  if (!stepperArmMotion(4, STP.system_base_dir, STP.system_base_dir == 1,
                        sample_us)) {
    stepperStopMotion();
    pitch_cal_task3_fail(PCERR_BUSY);
    return;
  }
  isr_clutch = true;
  bool timed_out = false;
  pitchCalWaitForStop(TO, &timed_out);
  stepperStopMotion();
  int32_t moved = abs(STP.currentPosition_p - pos0);
  STP.motorIs_moving = false;
  GUI.flag_update_lable1_2 = true;
  if (!pitch_cal_has_end_sensor) {
    const bool physical_limit = IOS.up_lim_sensor_state || IOS.ref_sensor_state;
    if (physical_limit || IOS.stop_button_state || STP.mvm_interupt || pitch_cal_cancel_requested ||
        moved != exact_pulses) {
      STP.referenced = false;
      pitch_cal_task3_fail(physical_limit ? PCERR_UNEXPECTED_END :
                             ((IOS.stop_button_state || pitch_cal_cancel_requested) ? PCERR_ABORT :
                              (timed_out ? PCERR_TIMEOUT : PCERR_ABORT)));
      return;
    }
    pitch_cal_pulses = moved;
    STP.mvm_interupt = false;
    pitch_cal_motion_busy = false;
    GUI.buzzer_cmd_short = true;
    pitch_cal_stage = PC_ENTER_MM;
    return;
  }
  const bool reached_end = IOS.up_lim_sensor_state || sensor_trip_which == 2;
  if (!reached_end || IOS.stop_button_state || STP.mvm_interupt || pitch_cal_cancel_requested ||
      moved < (STP.drv_ppr / 2)) {
    STP.referenced = false;
    pitch_cal_task3_fail((IOS.stop_button_state || pitch_cal_cancel_requested) ? PCERR_ABORT :
                         (timed_out ? PCERR_TIMEOUT : PCERR_ABORT));
    return;
  }
  pitch_cal_pulses = moved;  // raw I0-release -> I1-press count; I1 release is subtracted next.
  vTaskDelay(100 / portTICK_PERIOD_MS);  // controlled stop before end-sensor release
  if (IOS.stop_button_state || pitch_cal_cancel_requested || !IOS.up_lim_sensor_state) {
    STP.referenced = false;
    pitch_cal_task3_fail((IOS.stop_button_state || pitch_cal_cancel_requested) ? PCERR_ABORT : PCERR_SENSOR_STUCK);
    return;
  }
  STP.mvm_interupt = false;
  pitch_cal_motion_busy = false;
  pitch_cal_stage = PC_END_RELEASE;
}

// [RT/Task3] Creep out of I1, subtract release pulses, then use configured I0/I1 sizes.
void pitch_cal_do_end_release() {
  static const int CREEP_US = 200;
  const unsigned long TO = pitchCalTravelTimeoutMs((STP.drv_ppr > 0) ? STP.drv_ppr : 1, CREEP_US);
  if (!IOS.up_lim_sensor_state) { stepperStopMotion(); pitch_cal_task3_fail(PCERR_SENSOR_STUCK); return; }
  pitch_cal_motion_busy = true;
  STP.mvm_interupt = false;
  STP.motorIs_moving = true;
  const int32_t pos0 = STP.currentPosition_p;
  movement_mode = 4;
  totalPulses = 0;
  sensor_trip_which = 0;
  if (!stepperArmMotion(4, -STP.system_base_dir, STP.system_base_dir == -1, CREEP_US)) {
    stepperStopMotion(); pitch_cal_task3_fail(PCERR_BUSY); return;
  }
  isr_clutch = true;
  bool timed_out = false;
  pitchCalWaitForStop(TO, &timed_out);
  stepperStopMotion();
  const int32_t release = abs(STP.currentPosition_p - pos0);
  STP.motorIs_moving = false;
  pitch_cal_motion_busy = false;
  if (IOS.stop_button_state || STP.mvm_interupt || pitch_cal_cancel_requested || IOS.ref_sensor_state || IOS.up_lim_sensor_state || release < 1) {
    STP.referenced = false;
    pitch_cal_task3_fail(IOS.ref_sensor_state ? PCERR_WRONG_DIR :
                         ((IOS.stop_button_state || pitch_cal_cancel_requested) ? PCERR_ABORT :
                          ((IOS.up_lim_sensor_state || timed_out) ? PCERR_SENSOR_STUCK : PCERR_BAD_MEASURE)));
    return;
  }
  pitch_cal_end_release_pulses = release;
  pitch_cal_pulses -= release;
  const float end_mm = STP.final_position_mm.toFloat();
  if (!pitch_cal_compute_from_measure(end_mm)) {
    STP.referenced = false;
    pitch_cal_task3_fail(PCERR_BAD_MEASURE);
    return;
  }
  STP.mvm_interupt = false;
  GUI.buzzer_cmd_short = true;
  pitch_cal_stage = PC_RESULT;
}

static bool stepper_bind_runtime_pins() {
  if (IOS.pul_pin < 0 || IOS.dir_pin < 0 || IOS.enb_pin < 0) {
    stepper_pulse_mask = 0;
    pinmap_out_finish_stepper_rebind(false);
    return false;
  }
  digitalWrite(IOS.dir_pin, HIGH);
  digitalWrite(IOS.enb_pin, HIGH);
  pinMode(IOS.pul_pin, OUTPUT);
  pinMode(IOS.dir_pin, OUTPUT);
  pinMode(IOS.enb_pin, OUTPUT);
  stepperInitFastPulsePin();
  stepperPulseLow();
  pinmap_out_finish_stepper_rebind(true);
  return true;
}

// F3: تست دستی پین درایو — پالس‌های تنظیم‌شده راست، ۱s صبر، همان تعداد پالس چپ (بدون تغییر عدد موقعیت)
// دوباره زدن وسط تست: برگشت همان نت پالس‌ها به نقطهٔ شروع + EN خاموش
static inline bool f3_stepper_test_safety_stop_now() {
  if (IOS.stop_button_state || f3_stepper_test_stop_fault) {
    f3_stepper_test_stop_fault = true;
    return true;
  }
  return sensor_alert_motion_lock || g_update_mode;
}

static bool f3_stepper_test_wait_ms(uint32_t delay_ms, bool allow_manual_abort) {
  const uint32_t started = millis();
  while ((uint32_t)(millis() - started) < delay_ms) {
    if (f3_stepper_test_safety_stop_now()) return false;
    if (allow_manual_abort && f3_stepper_test_abort) return false;
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  return true;
}

static void f3_run_stepper_pin_test() {
  static bool busy = false;
  if (busy) return;
  if (g_update_mode || device_locked || sensor_alert_motion_lock || IOS.stop_button_state ||
      f3_stepper_test_stop_fault ||
      !pinmap_out_stepper_ready() ||
      STP.flag_is_cutting || STP.motorIs_moving || STP.jogEnabled || STP.moveStepperCMD ||
      STP.begin_calibration || pitch_cal_busy()) {
    GUI.buzzer_cmd_long = true;
    f3_stepper_test_busy = false;
    f3_stepper_test_abort = false;
    return;
  }
  f3_stepper_test_busy = true;
  f3_stepper_test_abort = false;
  f3_stepper_test_recovery_needed = false;
  f3_stepper_test_recovery_net = 0;
  f3_stepper_test_pulse_count = 0;
  busy = true;
  stepperStopMotion(true);
  const int N = constrain((int)f3_stepper_test_pulses, 10, 1000);
  const int PERIOD_MS = constrain((int)f3_stepper_test_period_ms, 10, 1000);
  const uint32_t pulse_slot_ms = (uint32_t)PERIOD_MS + 1U;
  f3_stepper_test_deadline_ms =
      millis() + (uint32_t)(2 * N) * pulse_slot_ms + 1000U;
  const bool right_high = ((1 * STP.system_base_dir) == 1);
  int net = 0;  // + راست از نقطهٔ شروع
  digitalWrite(IOS.enb_pin, LOW);
  f3_stepper_test_wait_ms(1, true);

  for (int i = 0; i < N; i++) {
    if (f3_stepper_test_abort || f3_stepper_test_safety_stop_now()) break;
    digitalWrite(IOS.dir_pin, right_high ? HIGH : LOW);
    if (!f3_stepper_test_wait_ms(1, true)) break;
    stepperPulseHigh();
    esp_rom_delay_us(STEPPER_PULSE_HIGH_US);
    stepperPulseLow();
    net++;
    f3_stepper_test_pulse_count++;
    if (!f3_stepper_test_wait_ms(PERIOD_MS, true)) break;
  }
  if (!(f3_stepper_test_abort || f3_stepper_test_safety_stop_now())) {
    for (int t = 0; t < 100; t++) {
      if (f3_stepper_test_abort || !f3_stepper_test_wait_ms(10, true)) break;
    }
  }
  if (!(f3_stepper_test_abort || f3_stepper_test_safety_stop_now())) {
    for (int i = 0; i < N; i++) {
      if (f3_stepper_test_abort || f3_stepper_test_safety_stop_now()) break;
      digitalWrite(IOS.dir_pin, right_high ? LOW : HIGH);
      if (!f3_stepper_test_wait_ms(1, true)) break;
      stepperPulseHigh();
      esp_rom_delay_us(STEPPER_PULSE_HIGH_US);
      stepperPulseLow();
      net--;
      if (f3_stepper_test_pulse_count > 0) f3_stepper_test_pulse_count--;
      if (!f3_stepper_test_wait_ms(PERIOD_MS, true)) break;
    }
  }

  // برگشت به نقطهٔ شروع
  if (f3_stepper_test_abort && !f3_stepper_test_safety_stop_now()) {
    f3_stepper_test_abort = false;
    f3_stepper_test_deadline_ms = millis() + (uint32_t)abs(net) * pulse_slot_ms;
  }
  f3_stepper_test_pulse_count = (uint32_t)abs(net);
  while (net > 0 && !f3_stepper_test_safety_stop_now()) {
    digitalWrite(IOS.dir_pin, right_high ? LOW : HIGH);
    if (!f3_stepper_test_wait_ms(1, false)) break;
    stepperPulseHigh();
    esp_rom_delay_us(STEPPER_PULSE_HIGH_US);
    stepperPulseLow();
    net--;
    if (f3_stepper_test_pulse_count > 0) f3_stepper_test_pulse_count--;
    if (!f3_stepper_test_wait_ms(PERIOD_MS, false)) break;
  }
  while (net < 0 && !f3_stepper_test_safety_stop_now()) {
    digitalWrite(IOS.dir_pin, right_high ? HIGH : LOW);
    if (!f3_stepper_test_wait_ms(1, false)) break;
    stepperPulseHigh();
    esp_rom_delay_us(STEPPER_PULSE_HIGH_US);
    stepperPulseLow();
    net++;
    if (f3_stepper_test_pulse_count > 0) f3_stepper_test_pulse_count--;
    if (!f3_stepper_test_wait_ms(PERIOD_MS, false)) break;
  }

  digitalWrite(IOS.enb_pin, HIGH);
  stepperPulseLow();
  const bool was_abort = f3_stepper_test_abort || f3_stepper_test_stop_fault;
  if (f3_stepper_test_stop_fault) {
    f3_stepper_test_recovery_net = net;
    f3_stepper_test_recovery_needed = true;
  }
  f3_stepper_test_deadline_ms = 0;
  f3_stepper_test_abort = false;
  busy = false;
  f3_stepper_test_busy = false;
  if (was_abort) GUI.buzzer_cmd_long = true;
  else GUI.buzzer_cmd_short = true;
}

volatile uint8_t f2_limit_seek_req = 0;
volatile int8_t f2_limit_seek_status = 0;

void f2_do_limit_seek(uint8_t target) {
  const int FAST_US = stepperIntervalUsForSpeed(STP.global_speed) * 2;
  const int CREEP_US = stepperIntervalUsForSpeed(STP.global_speed) * 12;
  const unsigned long TO = 70000UL;
  f2_limit_seek_status = 1;
  STP.mvm_interupt = false;
  STP.motorIs_moving = true;
  STP.jogEnabled = false;
  STP.moveStepperCMD = false;
  bool timed_out = false;

  if (target == 1) {
    movement_mode = 3;
    ref_stage = 1;
    if (!IOS.ref_sensor_state) {
      if (!stepperArmMotion(3, -STP.system_base_dir, STP.system_base_dir == -1, FAST_US)) {
        stepperStopMotion();
        f2_limit_seek_status = -2;
        STP.motorIs_moving = false;
        return;
      }
      isr_clutch = true;
      pitchCalWaitForStop(TO, &timed_out);
    }
    stepperStopMotion();
    bool abort = STP.mvm_interupt || IOS.stop_button_state || timed_out;

    if (!abort) {
      ref_stage = 2;
      vTaskDelay(100 / portTICK_PERIOD_MS);
      if (IOS.stop_button_state || STP.mvm_interupt) abort = true;
    }
    if (!abort) {
      if (!stepperArmMotion(3, STP.system_base_dir, STP.system_base_dir == 1, CREEP_US)) {
        stepperStopMotion();
        f2_limit_seek_status = -2;
        STP.motorIs_moving = false;
        return;
      }
      isr_clutch = true;
      pitchCalWaitForStop(TO, &timed_out);
      abort = STP.mvm_interupt || IOS.stop_button_state || timed_out;
    }
    stepperStopMotion();
    if (abort) {
      f2_limit_seek_status = timed_out ? -1 : -2;
      STP.motorIs_moving = false;
      return;
    }
    STP.currentPosition_p = STP.initial_Position_p;
    STP.referenced = true;
    STP.in_position_flag = true;
    GUI.flag_update_lable1_2 = true;
    save_last_pos_p();
    STP.motorIs_moving = false;
    STP.mvm_interupt = false;
    f2_limit_seek_status = 2;
    GUI.buzzer_cmd_short = true;
  } else if (target == 2) {
    movement_mode = 4;
    sensor_trip_which = 0;
    if (!IOS.up_lim_sensor_state) {
      if (!stepperArmMotion(4, STP.system_base_dir, STP.system_base_dir == 1, FAST_US)) {
        stepperStopMotion();
        f2_limit_seek_status = -2;
        STP.motorIs_moving = false;
        return;
      }
      isr_clutch = true;
      pitchCalWaitForStop(TO, &timed_out);
    }
    stepperStopMotion();
    bool abort = STP.mvm_interupt || IOS.stop_button_state || timed_out;

    if (!abort) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      if (IOS.stop_button_state || STP.mvm_interupt) abort = true;
    }
    if (!abort) {
      movement_mode = 3;
      ref_stage = 2;
      if (!stepperArmMotion(3, -STP.system_base_dir, STP.system_base_dir == -1, CREEP_US)) {
        stepperStopMotion();
        f2_limit_seek_status = -2;
        STP.motorIs_moving = false;
        return;
      }
      isr_clutch = true;
      pitchCalWaitForStop(TO, &timed_out);
      abort = STP.mvm_interupt || IOS.stop_button_state || timed_out;
    }
    stepperStopMotion();
    if (abort) {
      f2_limit_seek_status = timed_out ? -1 : -2;
      STP.motorIs_moving = false;
      return;
    }
    STP.currentPosition_p = STP.final_position_p;
    STP.referenced = true;
    STP.in_position_flag = true;
    GUI.flag_update_lable1_2 = true;
    save_last_pos_p();
    STP.motorIs_moving = false;
    STP.mvm_interupt = false;
    f2_limit_seek_status = 3;
    GUI.buzzer_cmd_short = true;
  }
}

static void f3_run_stepper_pin_test_recovery() {
  static bool busy = false;
  if (busy) return;
  if (g_update_mode || device_locked || sensor_alert_motion_lock || IOS.stop_button_state ||
      !pinmap_out_stepper_ready() || STP.flag_is_cutting || STP.motorIs_moving ||
      STP.jogEnabled || STP.moveStepperCMD || STP.begin_calibration || pitch_cal_busy()) {
    f3_stepper_test_stop_fault = true;
    GUI.buzzer_cmd_long = true;
    return;
  }

  const int PERIOD_MS = constrain((int)f3_stepper_test_period_ms, 10, 1000);
  const bool right_high = ((1 * STP.system_base_dir) == 1);
  int net = f3_stepper_test_recovery_net;
  f3_stepper_test_busy = true;
  f3_stepper_test_abort = false;
  f3_stepper_test_pulse_count = (uint32_t)abs(net);
  f3_stepper_test_deadline_ms = millis() + (uint32_t)abs(net) * ((uint32_t)PERIOD_MS + 1U);
  busy = true;
  stepperStopMotion(true);
  digitalWrite(IOS.enb_pin, LOW);

  while (net != 0 && !f3_stepper_test_safety_stop_now()) {
    const bool return_right_high = (net > 0) ? !right_high : right_high;
    digitalWrite(IOS.dir_pin, return_right_high ? HIGH : LOW);
    if (!f3_stepper_test_wait_ms(1, false)) break;
    stepperPulseHigh();
    esp_rom_delay_us(STEPPER_PULSE_HIGH_US);
    stepperPulseLow();
    net += (net > 0) ? -1 : 1;
    f3_stepper_test_recovery_net = net;
    if (f3_stepper_test_pulse_count > 0) f3_stepper_test_pulse_count--;
    if (!f3_stepper_test_wait_ms(PERIOD_MS, false)) break;
  }

  digitalWrite(IOS.enb_pin, HIGH);
  stepperPulseLow();
  if (f3_stepper_test_stop_fault) {
    f3_stepper_test_recovery_net = net;
    f3_stepper_test_recovery_needed = true;
    GUI.buzzer_cmd_long = true;
  } else {
    f3_stepper_test_recovery_net = 0;
    f3_stepper_test_recovery_needed = false;
    GUI.buzzer_cmd_short = true;
  }
  f3_stepper_test_deadline_ms = 0;
  f3_stepper_test_abort = false;
  busy = false;
  f3_stepper_test_busy = false;
}

void Task3(void* pvParameters) {
  vTaskDelay(250 / portTICK_PERIOD_MS);
  Serial.println("task3 start");
  {  //pins
    stepper_bind_runtime_pins();
    pinmap_out_mode(IOS.u_clip_pin, OUTPUT);
    pinmap_out_mode(IOS.f_clip_pin, OUTPUT);
    pinmap_out_mode(IOS.spray_pin, OUTPUT);
    pinmap_out_mode(IOS.saw_pin, OUTPUT);
    pinmap_out_mode(IOS.lamp_pin, OUTPUT);
    pinmap_out_mode(IOS.sawl_ift_pin, OUTPUT);
    pinmap_out_mode(IOS.q9_pin, OUTPUT);
    pinmap_out_write(IOS.u_clip_pin, HIGH);
    pinmap_out_write(IOS.f_clip_pin, HIGH);
    pinmap_out_write(IOS.spray_pin, HIGH);
    pinmap_out_write(IOS.saw_pin, HIGH);
    pinmap_out_write(IOS.lamp_pin, HIGH);
    pinmap_out_write(IOS.sawl_ift_pin, HIGH);
    pinmap_out_write(IOS.q9_pin, HIGH);
  }
  {                               //timer
    timer = timerBegin(1000000);  // 1us
    timerAttachInterrupt(timer, &onTimerFast);
    timerAlarm(timer, 100, true, 0);
    timerStop(timer);
  }
  while (1) {
    if (pinmap_out_take_stepper_rebind_request()) {
      isr_clutch = false;
      pinState = false;
      timerStop(timer);
      stepper_bind_runtime_pins();
    }
    // ایمنی: در جریانِ بروزرسانیِ F5-بوت هیچ حرکتِ موتوری آرم نمی‌شود
    // (پین‌ها در init بالا OFF شده‌اند؛ تایمرِ استپ متوقف است).
    if (g_update_mode) { vTaskDelay(50 / portTICK_PERIOD_MS); continue; }
    if (f3_stepper_test_recover_cmd) {
      f3_stepper_test_recover_cmd = false;
      f3_run_stepper_pin_test_recovery();
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    if (f3_stepper_test_cmd) {
      f3_stepper_test_cmd = false;
      f3_run_stepper_pin_test();
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    if (f2_limit_seek_req != 0 && !pitch_cal_busy()) {
      uint8_t req = f2_limit_seek_req;
      f2_limit_seek_req = 0;
      f2_do_limit_seek(req);
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    if (sensor_alert_motion_lock) {
      // فقط فرمان‌های جدید را بخور — mvm_interupt را اینجا ست نکن
      // (نژاد با بستن مودال: بعد از OK ممکن است mvm_interupt برای همیشه true بماند)
      STP.jogEnabled = false;
      STP.moveStepperCMD = false;
      isr_clutch = false;
      vTaskDelay(5 / portTICK_PERIOD_MS);
      continue;
    }
    if (STP.flag_is_cutting) {
      STP.jogEnabled = false;
      STP.moveStepperCMD = false;
      STP.mvm_interupt = false;
    }
    pitch_cal_task3_poll();
    if (!STP.referenced && STP.goref_cmd && !pitch_cal_busy()) Reference_Pos();
    else if (STP.go_sampling_CMD && STP.begin_calibration && STP.calib_stage == 1) go_sampling(STP.global_init_speed, STP.drv_ppr * 3, STP.system_set_dir);
    else if (!device_locked && STP.referenced && !STP.flag_is_cutting && !STP.begin_calibration && !pitch_cal_busy() && STP.jogEnabled && (GUI.NumLock || IOS.joystick_state_L || IOS.joystick_state_R)) {
      // بعد از رها→دوباره: همیشه از خزیدن شیب (نه ادامهٔ سرعت seek)
      int jog_init = (jog_ramp_ms > 0) ? jog_init_speed : 0;
      jogMotor(STP.global_speed, STP.system_set_dir, jog_ramp_ms, jog_init);
    }
    else if (STP.referenced && !STP.flag_is_cutting && STP.moveStepperCMD && !STP.begin_calibration && !pitch_cal_busy()) {
      const bool soft_seek = jog_seek_soft;
      jog_seek_soft = false;
      const int seek_ramp = soft_seek ? ((jog_ramp_ms > 0) ? jog_ramp_ms : auto_ramp_ms) : auto_ramp_ms;
      const int seek_init = (seek_ramp > 0) ? jog_init_speed : STP.global_speed;
      // soft seek: شیب توقف کامل زمان‌محور (۱۵mm کوتاه = توقف ناگهانی بود)
      const float soft_approach = soft_seek ? 40.0f : 0.0f;
      // لقی ۵mm فقط چپ (جاگ چپ / Enter به عدد کوچکتر). جاگ راست: allow=false
      bool allow_bl = true;
      if (soft_seek) {
        allow_bl = jog_seek_allow_backlash;
        jog_seek_allow_backlash = true;
      }
      manualMovement(STP.global_speed, STP.Man_Sp_mm, seek_init, seek_ramp,
                     soft_approach, allow_bl, soft_seek);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
