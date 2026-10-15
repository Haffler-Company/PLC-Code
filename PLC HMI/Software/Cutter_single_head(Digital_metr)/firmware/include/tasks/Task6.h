// task: task microswithces input reading
#define U_SWITCH_PIN 6  //I0(high limit) , I1(ref) pins on hafler plc state1:ref state-1_hlimit

int analogSamples_us = 0;
int us_state = 0;  // 0 idle / 1 I0 / -1 I1 / 2 هر دو ورودی
int temp_val_us = 0;
int prev_us_state = 0;

void average_us() {
  analogSamples_us = 0;
  for (int i = 0; i < 10; i++) {
    if (xSemaphoreTake(adcSemaphore, pdMS_TO_TICKS(20)) == pdTRUE) {
      temp_val_us = analogRead(U_SWITCH_PIN);
      xSemaphoreGive(adcSemaphore);
    }
    analogSamples_us = analogSamples_us + temp_val_us;
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  analogSamples_us = analogSamples_us / 10;
}

void read_input_us() {
  average_us();
  if (!((analogSamples_us > 1800) && (analogSamples_us < 2300))) {
    vTaskDelay(pdMS_TO_TICKS(40));
    average_us();
    if (!((analogSamples_us > 1800) && (analogSamples_us < 2300))) {
      vTaskDelay(pdMS_TO_TICKS(5));
      if (analogSamples_us > 1200 && analogSamples_us < 1800) {
        us_state = 1; 
        return;
      } else if (analogSamples_us > 3800 && analogSamples_us < 4100) {
        us_state = -1;
        return;
      } else if (analogSamples_us > 2400 && analogSamples_us < 2800) {
        us_state = 2;  // بازهٔ مستقل سخت‌افزار برای I0+I1
        return;
      } else {
        us_state = 0;
        return;
      }
    } else us_state = 0;
  } else us_state = 0;
}

void determine_inputs_us() {
  prev_us_state = us_state;
  const bool i0 = (us_state == 1 || us_state == 2);
  const bool i1 = (us_state == -1 || us_state == 2);
  pinmap_in_publish_pair(0, i0, 1, i1);
  IOS.ref_sensor_state = pinmap_in_role_active(0);
  IOS.up_lim_sensor_state = pinmap_in_role_active(1);
}

void Task6(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(400));
  Serial.println("task6 start");
  pinMode(U_SWITCH_PIN, INPUT);
  while (1) {
    read_input_us();
    vTaskDelay(pdMS_TO_TICKS(1));
    determine_inputs_us();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
