// Task7: (1) ADC GPIO4 → ترمینال I4/I5
//         (2) ورودی دیجیتال GPIO21=T1 و GPIO37=T2 → منابع فیزیکی 8 و 9 pinmap
// Active LOW؛ حرکت فقط بعد از دیدن آزاد پایدار؛ چپ+راست هم‌زمان باطل.

#define blade_motor_adc_pin 4
#define joy_right_pin 21  // T1
#define joy_left_pin 37   // T2
#define JOY_INPUT_ACTIVE_LEVEL LOW
#define JOY_DEBOUNCE_MS 30

// ---- ADC I4/I5 (سابقاً جویستیک) ----
static int analogSamples_bm = 0;
static int bm_state = 0;  // 0 idle / 1 I4 / -1 I5 / 2 هر دو ورودی
static int temp_val_bm = 0;

static void average_bm() {
  analogSamples_bm = 0;
  for (int i = 0; i < 10; i++) {
    if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(20)) == pdTRUE) {
      temp_val_bm = analogRead(blade_motor_adc_pin);
      xSemaphoreGive(adcSemaphore);
    }
    analogSamples_bm = analogSamples_bm + temp_val_bm;
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  analogSamples_bm = analogSamples_bm / 10;
}

static void read_input_blade_motor_adc() {
  average_bm();
  if (!((analogSamples_bm > 1800) && (analogSamples_bm < 2300))) {
    vTaskDelay(pdMS_TO_TICKS(40));
    average_bm();
    if (!((analogSamples_bm > 1800) && (analogSamples_bm < 2300))) {
      vTaskDelay(pdMS_TO_TICKS(1));
      if (analogSamples_bm > 1300 && analogSamples_bm < 1800) {
        bm_state = 1;   // I4
        return;
      } else if (analogSamples_bm > 3800 && analogSamples_bm < 4100) {
        bm_state = -1;  // I5
        return;
      } else if (analogSamples_bm > 2400 && analogSamples_bm < 3000) {
        bm_state = 2;   // بازهٔ مستقل سخت‌افزار برای I4+I5
        return;
      } else {
        bm_state = 0;
        return;
      }
    } else bm_state = 0;
  } else bm_state = 0;
}

static void determine_blade_motor_inputs() {
  const bool i4 = (bm_state == 1 || bm_state == 2);
  const bool i5 = (bm_state == -1 || bm_state == 2);
  pinmap_in_publish_pair(4, i4, 5, i5);
  IOS.blade_return_state = pinmap_in_role_active(4);
  IOS.motor_fault_state = pinmap_in_role_active(5);
}

// ---- جویستیک دیجیتال T1/T2 ----
static bool joy_l_raw_last = false;
static bool joy_r_raw_last = false;
static bool joy_l_physical_stable = false;
static bool joy_r_physical_stable = false;
static unsigned long joy_l_raw_changed_ms = 0;
static unsigned long joy_r_raw_changed_ms = 0;
static bool joy_motion_armed = false;
static uint32_t joy_pinmap_generation_seen = 0;

static void read_input_js() {
  const bool raw_l = (digitalRead(joy_left_pin) == JOY_INPUT_ACTIVE_LEVEL);
  const bool raw_r = (digitalRead(joy_right_pin) == JOY_INPUT_ACTIVE_LEVEL);
  const unsigned long now_ms = millis();

  if (raw_l != joy_l_raw_last) {
    joy_l_raw_last = raw_l;
    joy_l_raw_changed_ms = now_ms;
  }
  if (raw_r != joy_r_raw_last) {
    joy_r_raw_last = raw_r;
    joy_r_raw_changed_ms = now_ms;
  }
  if ((now_ms - joy_l_raw_changed_ms) >= JOY_DEBOUNCE_MS)
    joy_l_physical_stable = joy_l_raw_last;
  if ((now_ms - joy_r_raw_changed_ms) >= JOY_DEBOUNCE_MS)
    joy_r_physical_stable = joy_r_raw_last;

  // وضعیت فیزیکی Tها همیشه منتشر می‌شود؛ نقش منطقی ممکن است هرکدام از ده ورودی باشد.
  pinmap_in_publish_one(8, joy_r_physical_stable);  // T1
  pinmap_in_publish_one(9, joy_l_physical_stable);  // T2
}

static void determine_inputs_js() {
  const uint32_t map_generation = pinmap_in_get_generation();
  const bool mapped_r = pinmap_in_role_active(8);
  const bool mapped_l = pinmap_in_role_active(9);
  if (map_generation != joy_pinmap_generation_seen) {
    joy_pinmap_generation_seen = map_generation;
    joy_motion_armed = false;
  }
  // بعد از روشن‌شدن یا تغییر نگاشت، تا هر دو نقش یک بار آزاد نشوند حرکت مجاز نیست.
  if (!mapped_l && !mapped_r) joy_motion_armed = true;

  if (!GUI.joystick_enable || STP.flag_is_cutting || IOS.motor_fault_state ||
      IOS.stop_button_state) {
    IOS.joystick_state_L = false;
    IOS.joystick_state_R = false;
    return;
  }

  bool l = mapped_l && joy_motion_armed;
  bool r = mapped_r && joy_motion_armed;
  if (l && r) {
    l = false;
    r = false;
  }
  IOS.joystick_state_L = l;
  IOS.joystick_state_R = r;
}

void Task7(void *pvParameters) {
  vTaskDelay(450 / portTICK_PERIOD_MS);
  Serial.println("task7 start");
  pinMode(blade_motor_adc_pin, INPUT);
  // ورودی اکتیو-LOW در حالت قطع باید HIGH پایدار بماند؛ بدون pull-up شناور می‌شود.
  pinMode(joy_left_pin, INPUT_PULLUP);
  pinMode(joy_right_pin, INPUT_PULLUP);

  const unsigned long now_ms = millis();
  joy_l_raw_last = (digitalRead(joy_left_pin) == JOY_INPUT_ACTIVE_LEVEL);
  joy_r_raw_last = (digitalRead(joy_right_pin) == JOY_INPUT_ACTIVE_LEVEL);
  joy_l_raw_changed_ms = now_ms;
  joy_r_raw_changed_ms = now_ms;
  joy_l_physical_stable = false;
  joy_r_physical_stable = false;
  joy_motion_armed = false;
  joy_pinmap_generation_seen = pinmap_in_get_generation();
  IOS.joystick_state_L = false;
  IOS.joystick_state_R = false;
  IOS.blade_return_state = false;
  IOS.motor_fault_state = false;

  while (1) {
    // T1/T2 را پیش از تأخیر نمونه‌گیری ADC منتشر کن تا نقش ایمنی نگاشت‌شده دیر نشود.
    read_input_js();
    read_input_blade_motor_adc();
    determine_blade_motor_inputs();
    determine_inputs_js();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
