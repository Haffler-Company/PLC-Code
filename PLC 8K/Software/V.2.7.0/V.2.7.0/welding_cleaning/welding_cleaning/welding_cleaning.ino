#include "setting.h"
#include "functions.h"

TaskHandle_t Task1;

void setup()
{
  EEPROM.begin(EEPROM_SIZE);
 
  read_write_eeprom();
  setupPin();
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.createChar(0, customChar);
  lcd.begin(16, 2);
  lcd.createChar(1, customChar2);

  lcd.setCursor(0, 0);
  lcd.print("     Haffler    ");
  lcd.setCursor(0, 1);
  lcd.print("TM: ");
  lcd.setCursor(3, 1);
  if (isJ)
  {
    lcd.print("K");
  }else lcd.print("J");

  lcd.setCursor(5, 1);
  lcd.print("V2.2 ");
  
  lcd.setCursor(10, 1);
  lcd.print("PID:");

  lcd.setCursor(14, 1);
  lcd.print(PID);

  Serial.println("RUN");
  Serial.println("Code = V2.2.0");

  xTaskCreatePinnedToCore(
      loop1,   /* Task function. */
      "loop1", /* name of task. */
      1000,    /* Stack size of task */
      NULL,    /* parameter of the task */
      1,       /* priority of the task */
      &Task1,  /* Task handle to keep track of created task */
      0);  

  webApp.wStop();
  delay(200);
  on_wi_fi = true;
  webApp.wStart();
  
  delay(500);
  select_function();
}

void loop1(void *parameter)
{
  while (true)
  {
    if (!stopfource)
    {
      if (device_mode == MODE_WELDER_SINGLE || device_mode == MODE_WELDER_DOUBLE)  heater_contorl();
      // if (device_mode == DIGITAL_METER_MODE) steper_motor(profil_size , move_speed);
     }
    delay(1);
  }
}

void loop()
{

  if (on_wi_fi)
    webApp.Loop();
  if (device_mode == MODE_WELDER_SINGLE || device_mode == MODE_WELDER_DOUBLE)
  {
    upvc_welder();
    update_f();
  }else if (device_mode == MODE_CLEANER)
  {
    upvc_cleaner();
    update_f();
  }
//  if (device_mode == DIGITAL_METER_MODE)
//    upvc_digital_meter();

  // webApp.reqHandShake();
}
