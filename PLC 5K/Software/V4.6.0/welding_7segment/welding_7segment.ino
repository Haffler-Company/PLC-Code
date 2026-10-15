#include "setting.h"
#include "functions.h"
TaskHandle_t Task1;

void setup() {
  setupPin();
  EEPROM.begin(EEPROM_SIZE);
  read_time_ds3231();
  read_write_eeprom(0);

  dontcare_f10 = true;
  dontcare_stop = true;
  lcd_disabled = true;
  if (hardware_test == 1)
    check_hardware();

  dontcare_stop = false;
  map2 = true;

  xTaskCreatePinnedToCore(
    loop1,   /* Task function. */
    "loop1", /* name of task. */
    1000,    /* Stack size of task */
    NULL,    /* parameter of the task */
    1,       /* priority of the task */
    &Task1,  /* Task handle to keep track of created task */
    0);
  webApp.wStop();
  delay(100);  // 250
  on_wi_fi = true;
  webApp.wStart();
  if (on_wi_fi == true) {
    Serial.println("WIFI IS ON");
    led_stat = led_stat | wifi_signal;
  }

  if ((rtc_get_reset_reason(0) == POWERON_RESET) || (rtc_get_reset_reason(1) == POWERON_RESET)) {
    // show_temp = -32000;
    pixels.clear();
    pixels.show();
    for (int i = NUMPIXELS; i >= 0; i--) {
      pixels.setPixelColor(i, 0xffffff);  // white
    }
    pixels.show();
    ledModule_2.setPatternAt(1, 0xff);
    ledModule_2.setPatternAt(5, key_welding);
    up_seg(8, 8, 8, 8, 8, 8, 8, 7);
    delay(500);
    ledModule_2.setPatternAt(1, 0);
    ledModule_2.setPatternAt(5, 0);
    in_seg(unit_welding_p, 7);
    delay(1000);
    if (val_ds3231 && (temperature < 80) && (temperature != 0)) {
      if (temperature >= 65) {
        falt_over_temp = 1;
        led_stat = led_stat & ~error_div;
      }
      seg_1(temperature * 10, 7);
      ledModule_1.setPatternAt(0, alphabetPattern[rate]);
      seg_3(dayOfWeek, 7);
      ledModule_1.flush();
      //ledModule_2.flush();
    }
    delay(1500);
    Delay(1);
    demo();
    unit_welding = int(unit_welding);
    unit_welding_p = int(unit_welding_p);
    information();
    display_eeprom();
  } else
    select_function();
    
    programs_selection();

  if (lock_en == 1)
    check_lock_time();

  lcd_disabled = false;
  dontcare_f10 = false;
  map2 = false;
  int step_temp = 0;
  while (1) {
    if (temp < 600 || step_temp > 1000)
      break;
    delay(1);
    step_temp++;
  }
  if ((4 + temp >= c_up[program_no]) && (temp <= c_up[program_no] + 4)) {
    c_up_copy = c_up[program_no];
    c_up_copy_n = c_up[program_no];
    temp_dis = c_up[program_no];
    con_temp = true;
    show_temp = millis();
  }
  temp = temp / temp_offset;
  temp_offset_int = round((temp_offset * temp) - temp);
  temp = temp * temp_offset;
  temp_last = temp;

  esp_task_wdt_init(WDT_TIMEOUT, true);  // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                // add current thread to WDT
  Serial.print("Active Wach time(S) = ");
  Serial.println(WDT_TIMEOUT);

  led_stat = led_stat | all_key_on;
  led_stat_2 = key_welding;
  ledModule_2.setPatternAt(1, led_stat);
  ledModule_2.setPatternAt(5, led_stat_2);
  ledModule_2.flush();

  Serial.println("RUN");
  // webApp.reqHandShake();
  // clear_memory();
}

// ledModule_2.setPatternAt(1, key_stop);
// ledModule_2.setPatternAt(5, ~key_welding);
// ledModule_2.flush();

// ledModule_2.setPatternAt(1, led_stat);
// if (led_stat >= all_key_on) ledModule_2.setPatternAt(5, key_welding);
// ledModule_2.flush();

void loop1(void *parameter) {
  while (true) {
    if (heater_en)
      heater_contorl();
    delay(1);
  }
}

void loop() {
  Delay(1);
  if (on_wi_fi)
    webApp.Loop();
  upvc_welder();
  // average();
  // Serial.println(ADC_2);
  // delay(100);
  esp_task_wdt_reset();
  // clear_memory();
  // update_f();
}
