// task: خواندن پنج کلید سمت راست پنل از ladder پین ۲
// ترتیب سخت‌افزار: SAVE / CANCEL / MODE / START / STOP
//   CANCEL کوتاه = خروج بدون ذخیره      | CANCEL نگه‌داشته 5s = دیالوگ ریست فکتوری
//   SAVE   کوتاه = ذخیره و خروج          | SAVE   نگه‌داشته 5s = دیالوگ پورتال WiFi
//   START  کوتاه روی اصلی = حرکت استپر (مثل Enter) — نه استارت۱/۲ برش

#define menuKey_pin 2  // on  PLC panel stop/start/mode
#define MENU_KEY_HOLD_MS 5000
#define MENU_KEY_DEBOUNCE_MS 45
int temp_val_mk = 0;
enum {
  MENU_KEY_NONE = 0,
  MENU_KEY_SAVE = 1,
  MENU_KEY_CANCEL = 2,
  MENU_KEY_MODE = 3,
  MENU_KEY_START = 4,
  MENU_KEY_STOP = 5
};
static int menu_held_key = MENU_KEY_NONE;
static int menu_candidate_key = MENU_KEY_NONE;
static int menu_stable_key = MENU_KEY_NONE;
static uint32_t menu_candidate_since_ms = 0;
static unsigned long menu_hold_start_ms = 0;
static uint32_t menu_repeat_last_ms = 0;
static bool menu_long_fired = false;

static int decode_menu_key(int adc) {
  if (adc < 800) return MENU_KEY_NONE;
  static const int centers[5] = {4095, 3365, 2750, 1980, 1235};
  int best = -1, best_dist = 100000, second_dist = 100000;
  for (int i = 0; i < 5; ++i) {
    const int d = abs(adc - centers[i]);
    if (d < best_dist) {
      second_dist = best_dist;
      best_dist = d;
      best = i;
    } else if (d < second_dist) {
      second_dist = d;
    }
  }
  if (best < 0 || best_dist > 400 || (second_dist - best_dist) < 150)
    return MENU_KEY_NONE;
  return best + 1;
}

static int read_menu_key_median5() {
  int samples[5] = {0, 0, 0, 0, 0};
  if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(20)) == pdTRUE) {
    for (int i = 0; i < 5; ++i) {
      samples[i] = analogRead(menuKey_pin);
      // ADC conversion already provides settling time. Keep a short gap for
      // the resistor ladder without burning ~600 us of Core0 every 10 ms.
      if (i < 4) delayMicroseconds(20);
    }
    xSemaphoreGive(adcSemaphore);
  }
  for (int i = 0; i < 4; ++i) {
    for (int j = i + 1; j < 5; ++j) {
      if (samples[j] < samples[i]) {
        const int t = samples[i];
        samples[i] = samples[j];
        samples[j] = t;
      }
    }
  }
  temp_val_mk = samples[2];
  return decode_menu_key(temp_val_mk);
}

static int read_menu_key_stable() {
  const int decoded = read_menu_key_median5();
  const uint32_t now = millis();
  if (decoded != menu_candidate_key) {
    menu_candidate_key = decoded;
    menu_candidate_since_ms = now;
    return menu_stable_key;
  }
  if ((uint32_t)(now - menu_candidate_since_ms) >= MENU_KEY_DEBOUNCE_MS)
    menu_stable_key = decoded;
  return menu_stable_key;
}

void detemine_menue_keys() {
  const int stable = read_menu_key_stable();

  if (stable == MENU_KEY_NONE) {
    if (menu_held_key != MENU_KEY_NONE && !menu_long_fired) {
      if (menu_held_key == MENU_KEY_SAVE) GUI.flag_save_key = true;
      else if (menu_held_key == MENU_KEY_CANCEL) GUI.flag_cancl_key = true;
      else if (menu_held_key == MENU_KEY_START) GUI.flag_start_key = true;
      else if (menu_held_key == MENU_KEY_STOP) GUI.flag_stop_key = true;
    }
    menu_held_key = MENU_KEY_NONE;
    menu_hold_start_ms = 0;
    menu_repeat_last_ms = 0;
    menu_long_fired = false;
    return;
  }

  // تا رهاشدن کامل، تغییر مستقیم یک باند به باند دیگر را نویز تلقی کن.
  if (menu_held_key != MENU_KEY_NONE && stable != menu_held_key) return;

  if (menu_held_key == MENU_KEY_NONE) {
    menu_held_key = stable;
    menu_hold_start_ms = millis();
    menu_repeat_last_ms = menu_hold_start_ms;
    menu_long_fired = false;
    if (stable == MENU_KEY_MODE) GUI.flag_mode_key = true;
    // STOP is safety-critical: act after debounce on press, not on release.
    if (stable == MENU_KEY_STOP) {
      // Task10 is the owner of the panel ladder. Do not wait for Task2/LVGL
      // before stopping a reference pulse running in Task3.
      panel_request_stop_motion();
      GUI.flag_stop_key = true;
      menu_long_fired = true;  // prevent a second STOP action when the key is released
    } else if (stable == MENU_KEY_START && panel_five_nav_is_active()) {
      // پاسخ آنی لمسی در مود ناوبری
      GUI.flag_start_key = true;
      menu_long_fired = true;
    }
    return;
  }

  // افت ADC در لحظهٔ رهاکردن نباید به‌خاطر پنجرهٔ debounce یک long-press کاذب بسازد.
  if (menu_candidate_key != menu_held_key) return;

  // تکرار خودکار (Auto-Repeat) برای کلیدهای START و STOP صرفاً در حالت ناوبری
  if (panel_five_nav_is_active() &&
      (menu_held_key == MENU_KEY_START || menu_held_key == MENU_KEY_STOP)) {
    const uint32_t now = millis();
    if ((uint32_t)(now - menu_hold_start_ms) >= 600U) {
      if ((uint32_t)(now - menu_repeat_last_ms) >= 150U) {
        menu_repeat_last_ms = now;
        if (menu_held_key == MENU_KEY_START) GUI.flag_start_key = true;
        else GUI.flag_stop_key = true;
      }
    }
  }

  if (GUI.scr_id == 2 && !menu_long_fired &&
      (menu_held_key == MENU_KEY_CANCEL || menu_held_key == MENU_KEY_SAVE) &&
      (millis() - menu_hold_start_ms >= MENU_KEY_HOLD_MS)) {
      menu_long_fired = true;
      GUI.buzzer_cmd_long = true;
      if (menu_held_key == MENU_KEY_CANCEL) GUI.flag_factory_reset_dialog = true;
      else GUI.flag_web_portal_dialog = true;
  }
}

void Task10(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(600));
  Serial.println("task10 start");
  pinMode(menuKey_pin, INPUT);
  uint32_t last_warr_tick_ms = millis();
  while (1) {
    detemine_menue_keys();
    const uint32_t now = millis();
    if ((uint32_t)(now - last_warr_tick_ms) >= 1000UL) {
      last_warr_tick_ms = now;
      warranty_tick_1s();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
