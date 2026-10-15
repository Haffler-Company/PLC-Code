#include "setting.h"
#include "functions.h"

TaskHandle_t Task1;

void setup() {
  EEPROM.begin(EEPROM_SIZE);

  setupPin();
  read_time_ds3231();
  Serial.begin(115200);
  read_write_eeprom();

  lcd.begin(16, 2);
  lcd.createChar(0, customChar);
  lcd.begin(16, 2);
  lcd.createChar(1, customChar2);

  if (hardware_test == 1) check_hardware();

  xTaskCreatePinnedToCore(
    loop1,   /* Task function. */
    "loop1", /* name of task. */
    1000,    /* Stack size of task */
    NULL,    /* parameter of the task */
    1,       /* priority of the task */
    &Task1,  /* Task handle to keep track of created task */
    0);

  if ((rtc_get_reset_reason(0) == POWERON_RESET) || (rtc_get_reset_reason(1) == POWERON_RESET)) {
    if (lock_en == 1) {
      lcd.setCursor(0, 0);
      lcd.print(" Haffler");
      lcd.setCursor(11, 0);
      lcd.print("D:");
      lcd.setCursor(13, 0);
      lcd.print(lock_time - lock_timeing);
    } else {
      lcd.setCursor(0, 0);
      if (val_ds3231 && (temperature < 50) && (temperature != 0)) {
        lcd.print("Haffler T:" + String(temperature));
        lcd.write(byte(0));
        lcd.print("W:" + String(dayOfWeek));
      } else lcd.print("     Haffler    ");
    }
    lcd.setCursor(0, 1);
    lcd.print("TM: ");
    lcd.setCursor(3, 1);
    if (isJ) {
      lcd.print("K");
    } else lcd.print("J");

    lcd.setCursor(5, 1);
    lcd.print("V" + String(ver * 0.01));

    lcd.setCursor(9, 1);
    lcd.print(" PID:");

    lcd.setCursor(14, 1);
    lcd.print(PID);

    Serial.println("RUN");
    Serial.println("Code = V." + String(ver * 0.01));
    for (int i = 0; i < 20; i++) {
      delay(100);
      select_function();
    }
  } else select_function();

  if (lock_en == 1) check_lock_time();

  webApp.wStop();
  delay(100);
  on_wi_fi = true;
  webApp.wStart();
  if (on_wi_fi == true) Serial.println("WIFI IS ON");
  information();
  display_eeprom();

  if ((6 + temp >= c_up) && (temp <= c_up + 6)) {
    c_up_copy = c_up;
    c_up_copy_n = c_up;
    temp_dis = c_up;
    con_temp = true;
    show_temp = millis();
  }
  
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = WDT_TIMEOUT * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  if (esp_task_wdt_reconfigure(&twdt_config) != ESP_OK) {
    esp_task_wdt_init(&twdt_config);
  }
#else
  esp_task_wdt_init(WDT_TIMEOUT, true);  // enable panic so ESP32 restarts
#endif
  esp_task_wdt_add(NULL);                // add current thread to WDT
  Serial.println("RUN");
}

void loop1(void *parameter) {
  while (true) {
    if (device_mode <= MODE_WELDER_DOUBLE) heater_contorl();
    delay(1);
  }
}

void loop() {
  if (device_mode == MODE_WELDER_SINGLE || device_mode == MODE_WELDER_DOUBLE) {
    upvc_welder();
    update_f();
  } else if (device_mode == MODE_CLEANER) {
    upvc_cleaner();
    update_f();
  }
  esp_task_wdt_reset();


  //  if (device_mode == DIGITAL_METER_MODE)
  //    upvc_digital_meter();

  // webApp.reqHandShake();
}
