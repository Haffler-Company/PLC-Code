// task: کلیدهای حافظه F1..F5 روی پنل
// صفحهٔ اصلی: F1..F5 = حافظهٔ مقصد
//   - نگه‌داشتن F + Enter = ذخیره (بدون حرکت)
//   - رها کردن F بدون Enter = فراخوانی به مقصد (اگر قبلاً ذخیره شده)
// صفحهٔ تنظیمات: F1..F5 = تب‌ها

#define f_keys_pin 1
int temp_val_fk = 0;
static bool f_key_latched = false;
static int panel_f_last_which = 0;

static int decode_panel_f_key(int adc) {
  if (adc >= 3730) return 1;                              // F1 (4095)
  if (adc >= 3050 && adc < 3730) return 2;                 // F2 (3365)
  if (adc >= 2365 && adc < 3050) return 3;                 // F3 (2750)
  if (adc >= 1600 && adc < 2365) return 4;                 // F4 (1980)
  if (adc >= 800  && adc < 1600) return 5;                 // F5 (1235)
  return 0;
}

void detemine_f_keys() {
  if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(20)) == pdTRUE) {
    temp_val_fk = analogRead(f_keys_pin);
    xSemaphoreGive(adcSemaphore);
  }
  vTaskDelay(pdMS_TO_TICKS(10));
  if (temp_val_fk < 800) {
    // رها کردن: روی صفحهٔ اصلی اگر Enter ذخیره نکرد → فراخوانی
    if (panel_f_last_which >= 1 && panel_f_last_which <= 5 &&
        GUI.scr_id == 1 && !panel_f_save_consumed) {
      panel_f_press_edge = panel_f_last_which;
    }
    panel_f_held = 0;
    panel_f_last_which = 0;
    panel_f_save_consumed = false;
    f_key_latched = false;
    return;
  }

  // همیشه held را به‌روز کن تا Enter+F ذخیره ببیند (حتی بعد از latch)
  int held_now = decode_panel_f_key(temp_val_fk);
  if (held_now) {
    panel_f_held = held_now;
    panel_f_last_which = held_now;
  }

  if (f_key_latched) return;
  vTaskDelay(pdMS_TO_TICKS(40));
  if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(20)) == pdTRUE) {
    temp_val_fk = analogRead(f_keys_pin);
    xSemaphoreGive(adcSemaphore);
  }
  if (temp_val_fk < 800) {
    panel_f_held = 0;
    return;
  }

  int which = decode_panel_f_key(temp_val_fk);
  if (!which) return;
  panel_f_held = which;
  panel_f_last_which = which;
  f_key_latched = true;
  panel_f_save_consumed = false;

  // هر کلید F خارج از پنج کلید سمت راست، ابتدا فقط حالت انتخاب را می‌بندد.
  if (panel_five_nav_is_active()) {
    panel_f_save_consumed = true;
    GUI.flag_panel_nav_exit_key = true;
    return;
  }

  // صفحهٔ اصلی: فشردن فقط latch برای held — فراخوانی روی رها کردن
  if (GUI.scr_id == 1) return;

  // بقیهٔ F-keys فقط روی صفحهٔ تنظیمات = تعویض تب
  if (GUI.scr_id != 2) return;
  if (which == 1) GUI.tab_view_select = 1;
  else if (which == 2) GUI.tab_view_select = 2;
  else if (which == 3) GUI.tab_view_select = 3;
  else if (which == 4) GUI.tab_view_select = 4;
  else if (which == 5) GUI.tab_view_select = 5;
}

void Task9(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(550));
  Serial.println("task9 start");
  pinMode(f_keys_pin, INPUT);
  while (1) {
    detemine_f_keys();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
