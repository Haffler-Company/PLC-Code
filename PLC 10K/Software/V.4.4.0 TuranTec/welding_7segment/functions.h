#include "esp32-hal-adc.h"
#include "esp32-hal-gpio.h"
bool saveParams(int e, int c, int s, int pid, bool J, int _mot, int _abil, int _blade, int lang) {
  e_up[program_no] = e;
  c_up[program_no] = c;
  s_up[program_no] = s;
  PID = pid;
  isJ = J;
  motor_type = _mot;
  ability = _abil;
  blade_mode = _blade;
  buzzer_en = lang;
  update_data = true;
  return (true);
}
bool saveDeviceMode(Mode mode) {
  device_mode = mode;
  update_data = true;
  return (true);
}
void setupPin(void) {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(ADC_KEY_1, INPUT);
  pinMode(ADC_KEY_2, INPUT);

  pinMode(Start, INPUT_PULLUP);
  pinMode(Stop, INPUT_PULLUP);
  pinMode(back_ward_2, INPUT_PULLUP);
  pinMode(mic_sw_blade, INPUT_PULLUP);
  pinMode(mic_sw_motor, INPUT_PULLUP);

  pinMode(move_jack, OUTPUT);
  pinMode(jack_left, OUTPUT);
  pinMode(jack_righ, OUTPUT);

  pinMode(jack_heat, OUTPUT);
  pinMode(shablon, OUTPUT);
  pinMode(heater, OUTPUT);
  pinMode(tap_move_1, OUTPUT);
  pinMode(tap_move_2, OUTPUT);

  rest_pin_out();

  tmiInterface_tm_1.begin();
  tmiInterface_tm_2.begin();
  ledModule_1.begin();
  ledModule_2.begin();

  pixels.begin();
  pixels.setBrightness(100);  // Set BRIGHTNESS to about 1/5 (max = 255)
}
void rest_pin_out(void) {
  digitalWrite(move_jack, Disabled);
  digitalWrite(jack_left, Disabled);
  digitalWrite(jack_righ, Disabled);

  digitalWrite(jack_heat, Disabled);
  digitalWrite(shablon, Disabled);

  digitalWrite(heater, Disabled);
  digitalWrite(tap_move_1, Disabled);
  digitalWrite(tap_move_2, Disabled);
}
void heater_contorl(void) {
  int set_time = 0, ti = 0;
  int c_up_adj = (c_up[program_no] * 4);
  temp_adj = readTemp();
  if (isJ) {
    ///////////////////////////////////////////////TEMP K //////////////////////////
    c_up_adj = (c_up[program_no] * 4);
    temp = (temp_adj / 4);
  } else {
    ///////////////////////////////////////////////TEMP J //////////////////////////
    c_up_adj = (c_up[program_no] * 4) * 1.38;
    temp = (temp_adj / 4) / 1.38;
  }

  if ((c_up[program_no] < temp_dis_2 + 5 && c_up[program_no] + 5 > temp_dis_2) || val_ok == 0) {
    led_stat = led_stat | ok_signal;
  } else led_stat = led_stat & ~ok_signal;

  if (c_up[program_no] < temp + 5 && c_up[program_no] + 5 > temp) {
    falt_ssr = 0;
  } else if (temp < 120) {
    if ((millis() > 1000 * 60 * 1) && falt_ssr == 0 && (millis() < 1000 * 60 * 2)) {
      if (temp > temp_last - 5 && temp < temp_last + 5) {
        falt_ssr = 1;
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////
  if (((27 + temp) > c_up[program_no]) && (temp < c_up[program_no] + 9)) {
    ti = (temp_adj + welding_temp - (temp_adj / 2.3)) * PID;
    ti = (ti / 10);
    if (ti > 0) {
      if (temp + 20 < c_up[program_no] && stat_down == 0) {
        stat_down = 1;
        copy_time = millis();
      }
      if ((copy_time + 20000) < millis()) digitalWrite(heater, Enable);
      int result_val = c_up_adj - temp_adj;
      if (result_val >= 0) {
        set_time = ti + ti * result_val * 0.015;
      } else if (result_val + 10 >= 0) {
        set_time = ti + ti * result_val * 0.030;
      } else {
        set_time = ti + ti * result_val * 0.050;
      }
      if (set_time > 0) delay(set_time);
      digitalWrite(heater, Disabled);
    }
  } else if (temp < c_up[program_no]) {
    if (temp + 40 < c_up[program_no]) stat_down = 0;
    digitalWrite(heater, Enable);  // ON HEATER
  } else if (temp > c_up[program_no]) {
    digitalWrite(heater, Disabled);  // OFF HEATER
  }
  //  Serial.print("ti = ");
  //  Serial.println(ti);
  // Serial.print("   temp = ");
  // Serial.println(temp);
  //  Serial.print("   temp_adj = ");
  //  Serial.println(temp_adj);
  //  Serial.print("   set_time = ");
  //  Serial.println(set_time);
  //  Serial.print("   welding_temp = ");
  //  Serial.println(welding_temp);
  //  Serial.print("   c_up_adj = ");
  //  Serial.print(c_up_adj);
  //  Serial.print("   PID = ");
  //  Serial.print(PID);
  //  delay(50);
}
void upvc_welder(void) {
  if (ch_mode != device_mode) {
    ch_mode = device_mode;
    rest_pin_out();
  }
  if (falt_ssr_copy == 0 && falt_ssr == 1) {
    if (map2 == 0) {
      map2 = 10;
      lcd_disabled = true;
      falt_ssr_copy = 1;
      Serial.println("Error Heater OR SSR");
      down_seg(OFF_, OFF_, OFF_, OFF_, 7);
      up_seg_step(OFF_, E_, R_, R_, o_, OFF_, OFF_, 7);
      error_display(3);
      up_seg_step(OFF_, H_, E_, A_, T_, OFF_, OFF_, 7);
      error_display(3);
      up_seg_step(OFF_, OFF_, S_, S_, R_, OFF_, OFF_, 7);
      error_display(3);
      lcd_disabled = false;
      map2 = 0;
    }
  }
  if (val_ds3231 == true) {
    if (falt_over_temp == 1) {
      if (temperature >= 65) {
        if (map2 == 0) {
          map2 = 10;
          lcd_disabled = true;
          Serial.print("Error internal Over Temp :");
          Serial.println(temperature);
          down_seg(OFF_, OFF_, OFF_, OFF_, 7);
          ledModule_2.setPatternAt(4, alphabetPattern[rate]);
          up_seg_step(OFF_, OFF_, temperature / 10, temperature % 10, ON_, OFF_, OFF_, 7);
          error_display(3);
          lcd_disabled = false;
          map2 = 0;
          temperature = 0;
        }
      }
    }
  }

  if (map1 == 0) {
    if (enter_function == 1 || read_key(Start) || keyNum == F9 || (read_key(Start_2) && device_mode == MODE_WELDER_DOUBLE)) {
      if ((led_stat & ok_signal) == 2 || enter_time + 10000 > millis() || val_ok == 0) {
        if (falt_over_temp == 1) {
          color_rgb_sin = 2;       // warning
        } else color_rgb_sin = 5;  // okey
        step_stat_rgb = 12;
        map1 = 1;
        map2 = 1;
        p_stat = 1;
        if (jackSensore_enabled == 1 && check_jack(jackSensore_enabled) == 0) {
          lcd_disabled = true;
          up_seg_step(OFF_, J_, A_, C_, K_, OFF_, OFF_, 7);
          down_seg(B_, A_, C_, OFF_, 7);
          error_display(2);
          if (e_s_heater == 1) {
            up_seg_step(OFF_, H_, E_, A_, T_, OFF_, OFF_, 7);
            down_seg(B_, A_, C_, OFF_, 7);
            error_display(2);
          }
          if (e_s_tabel == 1) {
            up_seg_step(OFF_, T_, A_, B_, E_, OFF_, OFF_, 7);
            down_seg(B_, A_, C_, OFF_, 7);
            error_display(2);
          }
          if (e_s_shablon == 1) {
            up_seg_step(OFF_, S_, H_, O_, B_, OFF_, OFF_, 7);
            down_seg(B_, A_, C_, OFF_, 7);
            error_display(2);
          }
          lcd_disabled = false;
          color_rgb_sin = 2;
        }
        if ((led_stat & ok_signal) != 2) {
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, COLOR_RGB[7]);
          }
          pixels.show();
          up_seg(E_, R_, R_, OFF_, OFF_, OFF_, OFF_, 7);
          buzzer_warning(2);
          Delay(100);
        }
        on_wi_fi = false;                    ////////////
        led_stat = led_stat & ~wifi_signal;  //wifi off  ////////////
        rest_pin_out();
        if ((5 + temp_dis_2 >= c_up[program_no]) && (temp_dis_2 <= c_up[program_no] + 5)) {
          //color_rgb_sin = 5;  // okey
        } else {
          Serial.println("Error Temp");
          color_rgb_sin = 2;  // waning
          if (temp < 1000) {
            seg_1(temp_dis_2, 7);
          } else up_seg(E_, R_, R_, OFF_, OFF_, OFF_, OFF_, 7);
          seg_4(c_up[program_no], 7);

          up_seg(ON_, ON_, ON_, OFF_, OFF_, OFF_, OFF_, 7);
          down_seg(ON_, ON_, ON_, OFF_, 7);
          for (int i = 0; i < 3; i++) {
            up_seg(ON_, ON_, ON_, OFF_, OFF_, OFF_, OFF_, 0);
            down_seg(ON_, ON_, ON_, OFF_, 0);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[color_rgb_sin]);
            }
            pixels.show();
            delay(200);
            up_seg(ON_, ON_, ON_, OFF_, OFF_, OFF_, OFF_, 7);
            down_seg(ON_, ON_, ON_, OFF_, 7);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, 0x141400);
            }
            pixels.show();
            delay(200);
          }
        }
        start_av = 0;
        seg_5(L_L, 7);

        digitalWrite(shablon, Enable);
        Serial.println("shblon ON");

        for (int i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, COLOR_RGB[0]);
        }
        pixels.show();
        for (int i = NUMPIXELS; i >= 10; i--) {
          pixels.setPixelColor(i, COLOR_RGB[color_rgb_sin]);
        }
        pixels.show();
        step_stat_rgb = 10;

        if (on_wi_fi == true) webApp.wStop();  /////////////

        if (jackSensore_enabled) {
          sen_read(s_jack_shablon, 1);
        } else Delay(ax_ready - 100);

        digitalWrite(move_jack, Enable);
        if (jackSensore_enabled) {
          sen_read(s_jack_table, 1);
        } else Serial.println("MoveJack ON");
        start_av = 1;
        seg_5(program_no, 7);
      } else {
        start_av = 0;
        seg_5(L_L, 7);
        Serial.println("Error Temp");
        lcd_disabled = true;
        map2 = 1;
        for (int i = 0; i < 2; i++) {
          up_seg(OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, 7);
          down_seg(OFF_, OFF_, OFF_, OFF_, 7);
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, COLOR_RGB[7]);
          }
          pixels.show();
          buzzer_warning(1);
          Delay(10);
          seven_seg_1_copy = 10000;
          seven_seg_4_copy = 10000;
          if (temp < 1000) {
            seg_1(temp, 7);
          } else up_seg(E_, R_, R_, OFF_, OFF_, OFF_, OFF_, 7);
          seg_4(c_up[program_no], 7);
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, COLOR_RGB[10]);
          }
          pixels.show();
          buzzer_warning(1);
          Delay(10);
        }
        enter_time = millis();
        lcd_disabled = false;
        map2 = 0;
        start_av = 1;
        seg_5(program_no, 7);
      }
    }
  }
  if (map1 > 0) {
    bool read_key_start = read_key(Start);
    if ((read_key_start || keyNum == F9)) {
      if (p_stat == 0) {
        start_av = 0;
        seg_5(L_L, 7);
        p_stat = 1;
        if (map1 == 1) {
          if (work_time == 0) work_time = millis();
          digitalWrite(jack_righ, Enable);
          Serial.println("rightJack ON");
          step_stat_rgb = 9;
          pixels.setPixelColor(9, COLOR_RGB[color_rgb_sin]);
          pixels.show();
          for (int i = 0; i < (press_ready / 50); i++) {
            if ((read_key(back_ward) || keyNum == F5)) break;
            Delay(50);
          }
          map1 = 2;
        } else if (map1 == 2) {
          if (read_key_start == 1 && pedal_Count == 3) en_pedal = 1;
          digitalWrite(jack_left, Enable);
          Serial.println("leftJack ON");
          step_stat_rgb = 8;
          pixels.setPixelColor(8, COLOR_RGB[color_rgb_sin]);
          pixels.show();  //                       1.25(S) DELAY
          for (int i = 0; i < ((press_ready / 50) + 25); i++) {
            if ((read_key(back_ward) || keyNum == F5)) break;
            if (pedal_Count == 3) {
              Delay(50);
            } else Delay(30);
          }
          map1 = 3;
          if (device_mode == MODE_WELDER_SINGLE) map2 = 3;
        }
        start_av = 1;
        seg_5(program_no, 7);
      }
    } else p_stat = 0;
    if ((read_key(back_ward) || keyNum == F5)) {
      if (p_stat_1 == 0) {
        start_av = 0;
        seg_5(L_L, 7);
        p_stat_1 = 1;
        if (map1 == 2) {
          digitalWrite(jack_righ, Disabled);
          Serial.println("rightJack OFF");
          map1 = 1;
          step_stat_rgb = 9;
          pixels.setPixelColor(9, COLOR_RGB[0]);
          pixels.show();
          Delay(press_back);
        }
        if (map1 == 3) {
          digitalWrite(jack_left, Disabled);
          Serial.println("leftJack OFF");
          map1 = 2;
          step_stat_rgb = 8;
          pixels.setPixelColor(8, COLOR_RGB[0]);
          pixels.show();
          Delay(press_back);
        }
        start_av = 1;
        seg_5(program_no, 7);
      }
    } else p_stat_1 = 0;
    if (device_mode == MODE_WELDER_DOUBLE && read_key(back_ward_2)) {
      if (p_stat_3 == 0) {
        start_av = 0;
        seg_5(L_L, 7);
        p_stat_3 = 1;
        if (map2 == 2) {
          digitalWrite(jack_left_2, Disabled);
          Serial.println("Jack Left 2 OFF");
          map2 = 1;
          step_stat_rgb = 9;
          pixels.setPixelColor(9, COLOR_RGB[0]);
          pixels.show();
          Delay(press_back);
        } else if (map2 == 3) {
          digitalWrite(jack_righ_2, Disabled);
          Serial.println("jack Righ 2 OFF");
          map2 = 2;
          step_stat_rgb = 8;
          pixels.setPixelColor(8, COLOR_RGB[0]);
          pixels.show();
          Delay(press_back);
        }
        start_av = 1;
        seg_5(program_no, 7);
      }
    } else p_stat_3 = 0;
  }
  if (device_mode == MODE_WELDER_DOUBLE && read_key(Start_2)) {
    if (p_stat_2 == 0) {
      start_av = 0;
      seg_5(L_L, 7);
      p_stat_2 = 1;
      if (map2 == 1) {
        digitalWrite(jack_left_2, Enable);
        Serial.println("Jack Left 2 ON");
        step_stat_rgb = 9;
        pixels.setPixelColor(9, COLOR_RGB[color_rgb_sin]);
        pixels.show();
        Delay(press_ready);
        map2 = 2;
      } else if (map2 == 2) {
        digitalWrite(jack_righ_2, Enable);
        Serial.println("Jack Righ 2 ON");
        step_stat_rgb = 8;
        pixels.setPixelColor(8, COLOR_RGB[color_rgb_sin]);
        pixels.show();
        Delay(press_ready);
        map2 = 3;
      }
      start_av = 1;
      seg_5(program_no, 7);
    }
  } else p_stat_2 = 0;

  if (map1 == 3 && map2 == 3) {  //if want mor perss key, must delete (device_mode == MODE_WELDER_SINGLE)
    if ((device_mode == MODE_WELDER_SINGLE && en_pedal == 1) || read_key(Start) || keyNum == F9 || ((device_mode == MODE_WELDER_DOUBLE) && read_key(Start_2))) {
      start_av = 0;
      seg_5(L_L, 7);
      en_pedal = 0;
      welding_temp = 150;
      digitalWrite(move_jack, Disabled);
      Serial.println("MoveJack OFF");
      if (jackSensore_enabled) {
        sen_read(s_jack_table, 0);
      } else Delay(table_back);  ////////////////////
      digitalWrite(shablon, Disabled);
      Serial.println("ShabplonJack OFF");
      if (jackSensore_enabled) {
        sen_read(s_jack_shablon, 0);
      } else Delay(ax_back);  ////////////////////
      digitalWrite(jack_heat, Enable);
      Serial.println("HeaterJack ON");
      step_stat_rgb = 7;
      pixels.setPixelColor(7, COLOR_RGB[color_rgb_sin]);
      pixels.show();
      if (jackSensore_enabled) {
        sen_read(s_jack_heater, 1);
      } else Delay(heater_ready);  ////////////////////
      digitalWrite(move_jack, Enable);
      Serial.println("MoveJack ON");
      if (jackSensore_enabled) {
        sen_read(s_jack_table, 1);
      } else Delay(table_ready);  ////////////////////

      map1 = 4;
      int val_delay, val2_delay = 1000;
      Serial.println("start");
      for (eTime[program_no] = e_up[program_no]; eTime[program_no] > 0; eTime[program_no]--) {
        val_delay = millis();
        Serial.print("Time = ");
        Serial.println(val2_delay);
        Delay(val2_delay);
        Serial.print("eTime = ");
        Serial.println(eTime[program_no]);
        pixels.setPixelColor(7 - int((float(e_up[program_no] - eTime[program_no]) / e_up[program_no]) * 3), COLOR_RGB[color_rgb_sin]);
        pixels.show();
        val2_delay = 2000 - (millis() - val_delay);
      }
      Serial.println("finsh");
      //Delay(190);
      for (int i = NUMPIXELS; i >= 4; i--) {
        pixels.setPixelColor(i, COLOR_RGB[color_rgb_sin]);
      }
      pixels.show();
      map1 = 5;
      welding_temp = 0;
      digitalWrite(move_jack, Disabled);
      Serial.println("MoveJack OFF");
      step_stat_rgb = 4;
      if (jackSensore_enabled) {
        sen_read(s_jack_table, 0);
      } else Delay(table_back);  ////////////////////
      digitalWrite(jack_heat, Disabled);
      Serial.println("HeaterJack OFF");
      if (jackSensore_enabled) {
        sen_read(s_jack_heater, 0);
      } else Delay(heater_back);  ////////////////////
      digitalWrite(move_jack, Enable);
      Serial.println("MoveJack ON");
      if (jackSensore_enabled) {
        sen_read(s_jack_table, 1);
      } else Delay(table_ready);  ////////////////////
      Serial.println("start");
      val2_delay = 1000;
      for (sTime[program_no] = s_up[program_no]; sTime[program_no] > 0; sTime[program_no]--) {
        val_delay = millis();
        int unit_rgb = 4 - int((float(s_up[program_no] - sTime[program_no]) / s_up[program_no]) * 4);
        if (sTime[program_no] <= (s_up[program_no] - tap_jack_1) && device_mode == MODE_WELDER_SINGLE) {
          digitalWrite(tap_move_1, Enable);
        }
        if (sTime[program_no] <= (s_up[program_no] - tap_jack_2) && device_mode == MODE_WELDER_SINGLE) {
          digitalWrite(tap_move_2, Enable);
        }
        if (sTime[program_no] < 4) {
          up_seg(ON_, ON_, ON_, ON_, ON_, ON_, ON_, 0);
          down_seg(ON_, ON_, ON_, ON_, 0);
          buzzer_warning(1);
          Delay(300);
          up_seg(ON_, ON_, ON_, ON_, ON_, ON_, ON_, 7);
          down_seg(ON_, ON_, ON_, ON_, 7);
          for (int i = NUMPIXELS; i >= unit_rgb; i--) {
            if (color_rgb_sin == 5) pixels.setPixelColor(i, 0x001400);
            if (color_rgb_sin == 2) pixels.setPixelColor(i, 0x141400);  //WARNING
          }
          pixels.show();
        } else Delay(val2_delay / 2);

        if (sTime[program_no] < 4) {
          buzzer_warning(1);
          Delay(300);
        } else Delay(val2_delay / 2);

        for (int i = NUMPIXELS; i >= unit_rgb; i--) {
          pixels.setPixelColor(i, COLOR_RGB[color_rgb_sin]);
        }
        pixels.show();
        Serial.print("sTime = ");
        Serial.println(sTime[program_no]);
        val2_delay = 2000 - (millis() - val_delay);
      }
      Serial.println("finsh");
      for (int i = NUMPIXELS; i >= 0; i--) {
        pixels.setPixelColor(i, COLOR_RGB[color_rgb_sin]);
      }
      pixels.show();
      Delay(190);
      map1 = 6;
      digitalWrite(tap_move_1, Disabled);
      Serial.println("tap_move_1 OFF");
      digitalWrite(tap_move_2, Disabled);
      Serial.println("tap_move_2 OFF");

      digitalWrite(jack_left, Disabled);
      Serial.println("leftJack OFF");

      digitalWrite(jack_left_2, Disabled);
      Serial.println("leftJack_2 OFF");


      ////////////////////////////////////////////
      step_stat_rgb = 0;
      if (finish_time) {
        Delay(press_back / 2.5);  //200
        digitalWrite(move_jack, Disabled);
        Serial.println("MoveJack OFF");
        if (jackSensore_enabled) {
          sen_read(s_jack_table, 0);
        } else Delay(table_back);
      }
      digitalWrite(jack_righ, Disabled);
      Serial.println("righJack OFF");
      digitalWrite(jack_righ_2, Disabled);
      Serial.println("righJack_2 OFF");

      if (finish_time == 0) {
        Delay(press_back / 1.5);  //350
        digitalWrite(move_jack, Disabled);
        if (jackSensore_enabled) {
          sen_read(s_jack_table, 0);
        } else Serial.println("MoveJack OFF");
      }

      unit_welding = unit_welding + 0.25;
      unit_welding_p = unit_welding_p + 0.25;
      lcd_disabled = true;
      sement_off();
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, COLOR_RGB[8]);  //off rgb
      }
      pixels.show();
      in_seg(unit_welding, 7);
      int unit_welding_c = (unit_welding - int(unit_welding)) * 4;
      Serial.print("Unit Welding Windos: ");
      Serial.println(unit_welding);

      Serial.print("Work Time : ");
      Serial.println((millis() - work_time) / 1000);
      seven_seg_5_copy = 10000;
      if (unit_welding_c == 0) unit_welding_c = 4;

      Serial.print("Unit Welding Profill: ");
      Serial.println(unit_welding_c);

      seg_5(unit_welding_c, 7);
      int device_operation_time = (millis() - work_time) / 1000;
      if (device_operation_time < 1000) {
        seg_4(device_operation_time, 7);
      } else down_seg(O_, V_, E_, F_, 7);
      keyNum = 0;
      for (int i = 0; i < 100; i++) {
        if (keyNum == F5) {
          Serial.println("Clear unit welding");
          unit_welding = 0;
          unit_welding_p = int(unit_welding_p);
          up_seg(R_, S_, T_, C_, U_, N_, T_, 7);
          Delay(1500);
          in_seg(unit_welding, 7);
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, COLOR_RGB[2]);  //YELLOW
          }
          pixels.show();
          buzzer_warning(2);
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, 0x141400);  //WARNIG
          }
          pixels.show();
          buzzer_warning(2);
          break;
        }
        Delay(15);
      }
      if (unit_welding_p == int(unit_welding_p)) {
        work_time = 0;
        update_data = true;
        save_var();
      }
      ///////////////////////////////////////////////////

      ////////////////////////////////////////////
      //      digitalWrite(jack_righ, Disabled);
      //      Serial.println("righJack OFF");
      //      digitalWrite(jack_righ_2, Disabled);
      //      Serial.println("righJack_2 OFF");
      ///////////////////////////////////////////////////

      eTime[program_no] = e_up[program_no];
      cTemp[program_no] = c_up[program_no];
      sTime[program_no] = s_up[program_no];

      Delay(table_back);

      if (auto_start) {
        for (int i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, COLOR_RGB[2]);
        }
        pixels.show();

        up_seg_step(OFF_, S_, T_, A_, R_, T_, OFF_, 7);
        down_seg(OFF_, OFF_, OFF_, OFF_, 7);
        Delay(1000);
        for (int i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, COLOR_RGB[2]);
        }
        pixels.show();
        buzzer_warning(2);

        for (int i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, 0x141400);  //WARNING
        }
        pixels.show();
        Delay(500);

        for (int i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, COLOR_RGB[2]);
        }
        pixels.show();
        buzzer_warning(2);
        int time_left_read_key = millis();
        while (1) {
          Delay(1);
          if (keyNum < F9) break;
          if (millis() > time_left_read_key + 1200 || keyNum == F9 || read_key(Start)) {
            enter_function = 1;
            break;
          }
        }
      }
      lcd_disabled = false;
      map1 = 0;
      map2 = 0;
      start_av = 1;
      seg_5(program_no, 7);
      time_u_lcd = millis();
    }
    if (on_wi_fi == true) webApp.reqHandShake();
  }

  if (keyNum != 100 && map1 == 0) {
    lcd_disabled = true;
    dontcare_f10 = true;
    if (keyNum == F1) {
      if (p_function == 0) {
        p_function = 1;
        program_no = 1;
        if (on_wi_fi) webApp.reqHandShake();
        eTime[program_no] = e_up[program_no];
        cTemp[program_no] = c_up[program_no];
        sTime[program_no] = s_up[program_no];
        c_up_copy = c_up[program_no] + 10;
        c_up_copy_n = c_up[program_no] - 10;
        show_temp = millis();
        update_data = true;
      }
    }
    if (keyNum == F2) {
      if (p_function == 0) {
        p_function = 1;
        program_no = 2;
        if (on_wi_fi) webApp.reqHandShake();
        eTime[program_no] = e_up[program_no];
        cTemp[program_no] = c_up[program_no];
        sTime[program_no] = s_up[program_no];
        c_up_copy = c_up[program_no] + 10;
        c_up_copy_n = c_up[program_no] - 10;
        show_temp = millis();
        update_data = true;
      }
    }
    if (keyNum == F3) {
      if (p_function == 0) {
        p_function = 1;
        program_no = 3;
        if (on_wi_fi) webApp.reqHandShake();
        eTime[program_no] = e_up[program_no];
        cTemp[program_no] = c_up[program_no];
        sTime[program_no] = s_up[program_no];
        c_up_copy = c_up[program_no] + 10;
        c_up_copy_n = c_up[program_no] - 10;
        show_temp = millis();
        update_data = true;
      }
    }
    if (keyNum == F4) {
      if (p_function == 0) {
        p_function = 1;
        program_no = 4;
        if (on_wi_fi) webApp.reqHandShake();
        eTime[program_no] = e_up[program_no];
        cTemp[program_no] = c_up[program_no];
        sTime[program_no] = s_up[program_no];
        c_up_copy = c_up[program_no] + 10;
        c_up_copy_n = c_up[program_no] - 10;
        show_temp = millis();
        update_data = true;
      }
    }
    if (keyNum == F5) {
      if (p_function == 0) {
        p_function = 1;
        program_no = 5;
        if (on_wi_fi) webApp.reqHandShake();
        eTime[program_no] = e_up[program_no];
        cTemp[program_no] = c_up[program_no];
        sTime[program_no] = s_up[program_no];
        c_up_copy = c_up[program_no] + 10;
        c_up_copy_n = c_up[program_no] - 10;
        show_temp = millis();
        update_data = true;
      }
    }
    int step_mode = 0;
    if (keyNum == F6)  ///////////////////////////////f6
    {
      ledModule_2.setPatternAt(4, 0);
      ledModule_2.flush();
      buzzer(1);

      ledModule_2.setPatternAt(3, 0);
      ledModule_2.flush();
      buzzer(1);

      ledModule_2.setPatternAt(0, 0);
      ledModule_2.flush();
      buzzer(1);

      ledModule_2.setPatternAt(2, 0);
      ledModule_2.flush();
      buzzer(1);

      //down_seg(ON_, ON_, ON_, OFF_, 7);
      //sement_off();
      map2 = true;
      led_stat = led_stat | set_signal;  // set on
      seven_seg_4_copy = 10000;
      seven_seg_1_copy = 10000;
      seg_4(cTemp[program_no], 7);
      seg_1(c_up[program_no], 7);
      buzzer_warning(1);
      for (int i = NUMPIXELS; i >= 0; i--) {
        pixels.setPixelColor(i, COLOR_RGB[0]);
      }
      step_mode = (NUMPIXELS - ((c_up[program_no] - 170) / 8));
      if (c_up[program_no] < 190) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
        }
      } else if (c_up[program_no] < 246 && c_up[program_no] >= 190) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[5]);  //green
        }
      } else if (c_up[program_no] >= 246) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[7]);  //red
        }
      }
      pixels.show();
      while (1) {
        Delay(50);
        if (keyNum != F6) break;
      }
      Delay(50);
      while (1) {
        Delay(320);
        if (seven_seg_1_copy != c_up[program_no]) {
          seg_1(c_up[program_no], 7);
          step_mode = (NUMPIXELS - ((c_up[program_no] - 170) / 8));
          if (c_up[program_no] < 190) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (c_up[program_no] < 246 && c_up[program_no] >= 190) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (c_up[program_no] >= 246) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
        }
        if (keyNum == F9) {
          if (c_up[program_no] < 999) {
            c_up[program_no]++;
          } else c_up[program_no] = 1;
          seg_1(c_up[program_no], 7);
          step_mode = (NUMPIXELS - ((c_up[program_no] - 170) / 8));
          if (c_up[program_no] < 190) {
            for (int i = NUMPIXELS; i >= c_up[program_no]; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (c_up[program_no] < 246 && c_up[program_no] >= 190) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (c_up[program_no] >= 246) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
          if (on_wi_fi) webApp.reqHandShake();
        }
        if (keyNum == F10) {
          if (c_up[program_no] > 1) {
            c_up[program_no]--;
          } else c_up[program_no] = 999;
          seg_1(c_up[program_no], 7);
          step_mode = (NUMPIXELS - ((c_up[program_no] - 170) / 8));
          if (c_up[program_no] < 190) {
            for (int i = NUMPIXELS; i >= c_up[program_no]; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (c_up[program_no] < 246 && c_up[program_no] >= 190) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (c_up[program_no] >= 246) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
          if (on_wi_fi) webApp.reqHandShake();
        }
        if (keyNum == F6) {
          while (1) {
            Delay(50);
            if (keyNum != F6) break;
          }
          break;
        }
      }
      led_stat = led_stat & ~set_signal;  //set off
      seg_1(temp_dis, 7);
      for (int i = NUMPIXELS; i >= 0; i--) {
        pixels.setPixelColor(i, COLOR_RGB[7]);
      }
      pixels.show();
      map2 = false;
      update_data = true;
      seven_seg_2_copy = 1000;
      seven_seg_3_copy = 1000;
      seven_seg_4_copy = 1000;
      seven_seg_5_copy = 1000;
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      temp_dis = temp_dis_2;
      con_temp = true;
      show_temp = millis();
    } else if (keyNum == F7)  ///////////////////////////////////f7
    {
      ledModule_1.setPatternAt(2, 0);
      ledModule_1.flush();
      buzzer(1);
      ledModule_1.setPatternAt(1, 0);
      ledModule_1.flush();
      buzzer(1);
      ledModule_1.setPatternAt(0, 0);
      ledModule_1.flush();
      buzzer(1);

      ledModule_2.setPatternAt(0, 0);
      ledModule_2.flush();
      buzzer(1);
      ledModule_2.setPatternAt(2, 0);
      ledModule_2.flush();
      buzzer(1);

      down_seg(OFF_, OFF_, OFF_, ON_, 7);

      //sement_off();
      map2 = true;
      show_temp = 0;
      led_stat = led_stat | set_signal;  //set on
      seven_seg_2_copy = 1000;
      seg_2(e_up[program_no], 7);
      buzzer_warning(1);
      for (int i = NUMPIXELS; i >= 0; i--) {
        pixels.setPixelColor(i, COLOR_RGB[0]);
      }
      step_mode = (NUMPIXELS - ((e_up[program_no] + 1) / 3));
      if (e_up[program_no] < 12) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
        }
      } else if (e_up[program_no] < 26 && e_up[program_no] >= 12) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[5]);  //green
        }
      } else if (e_up[program_no] >= 26) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[7]);  //red
        }
      }
      pixels.show();
      while (1) {
        Delay(50);
        if (keyNum != F7) break;
      }
      Delay(50);
      while (1) {
        Delay(320);
        if (seven_seg_2_copy != e_up[program_no]) {
          seg_2(e_up[program_no], 7);
          step_mode = (NUMPIXELS - ((e_up[program_no] + 1) / 3));
          if (e_up[program_no] < 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (e_up[program_no] < 26 && e_up[program_no] >= 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (e_up[program_no] >= 26) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
        }
        if (keyNum == F9) {
          if (e_up[program_no] < 99) {
            e_up[program_no]++;
          } else e_up[program_no] = 1;
          seg_2(e_up[program_no], 7);
          step_mode = (NUMPIXELS - ((e_up[program_no] + 1) / 3));
          if (e_up[program_no] < 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (e_up[program_no] < 26 && e_up[program_no] >= 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (e_up[program_no] >= 26) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
          if (on_wi_fi) webApp.reqHandShake();
        }
        if (keyNum == F10) {
          if (e_up[program_no] > 1) {
            e_up[program_no]--;
          } else e_up[program_no] = 99;
          seg_2(e_up[program_no], 7);
          step_mode = (NUMPIXELS - ((e_up[program_no] + 1) / 3));
          if (e_up[program_no] < 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (e_up[program_no] < 26 && e_up[program_no] >= 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (e_up[program_no] >= 26) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
          if (on_wi_fi) webApp.reqHandShake();
        }
        if (keyNum == F7) {
          while (1) {
            Delay(50);
            if (keyNum != F7) break;
          }
          break;
        }
      }
      led_stat = led_stat & ~set_signal;  //set off
      seg_2(e_up[program_no], 7);
      for (int i = NUMPIXELS; i >= 0; i--) {
        pixels.setPixelColor(i, COLOR_RGB[7]);
      }
      pixels.show();
      map2 = false;
      update_data = true;
      seven_seg_1_copy = 10000;
      seven_seg_2_copy = 10000;
      seven_seg_3_copy = 10000;
      seven_seg_4_copy = 10000;
      seven_seg_5_copy = 10000;
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      temp_dis = temp_dis_2;
      con_temp = true;
      show_temp = millis();
    } else if (keyNum == F8)  //////////////////////////////////////////////f8
    {
      ledModule_1.setPatternAt(2, 0);
      ledModule_1.flush();
      buzzer(1);
      ledModule_1.setPatternAt(1, 0);
      ledModule_1.flush();
      buzzer(1);
      ledModule_1.setPatternAt(0, 0);
      ledModule_1.flush();
      buzzer(1);

      ledModule_2.setPatternAt(4, 0);
      ledModule_2.flush();
      buzzer(1);
      ledModule_2.setPatternAt(3, 0);
      ledModule_2.flush();
      buzzer(1);

      down_seg(OFF_, OFF_, OFF_, ON_, 7);

      //sement_off();
      map2 = true;
      show_temp = 0;
      led_stat = led_stat | set_signal;  //set on
      seven_seg_3_copy = 1000;
      seg_3(s_up[program_no], 7);
      buzzer_warning(1);
      for (int i = NUMPIXELS; i >= 0; i--) {
        pixels.setPixelColor(i, COLOR_RGB[0]);
      }
      step_mode = (NUMPIXELS - ((s_up[program_no] + 1) / 3));
      if (s_up[program_no] < 12) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
        }
      } else if (s_up[program_no] < 26 && s_up[program_no] >= 12) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[5]);  //green
        }
      } else if (s_up[program_no] >= 26) {
        for (int i = NUMPIXELS; i >= step_mode; i--) {
          pixels.setPixelColor(i, COLOR_RGB[7]);  //red
        }
      }
      pixels.show();
      while (1) {
        Delay(50);
        if (keyNum != F8) break;
      }
      Delay(50);
      while (1) {
        Delay(320);
        if (seven_seg_3_copy != s_up[program_no]) {
          seg_3(s_up[program_no], 7);
          step_mode = (NUMPIXELS - ((s_up[program_no] + 1) / 3));
          if (s_up[program_no] < 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (s_up[program_no] < 26 && s_up[program_no] >= 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (s_up[program_no] >= 26) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
        }
        if (keyNum == F9) {
          if (s_up[program_no] < 99) {
            s_up[program_no]++;
          } else s_up[program_no] = 1;
          seg_3(s_up[program_no], 7);
          step_mode = (NUMPIXELS - ((s_up[program_no] + 1) / 3));
          if (s_up[program_no] < 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (s_up[program_no] < 26 && s_up[program_no] >= 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (s_up[program_no] >= 26) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
          if (on_wi_fi) webApp.reqHandShake();
        }
        if (keyNum == F10) {
          if (s_up[program_no] > 1) {
            s_up[program_no]--;
          } else s_up[program_no] = 99;
          seg_3(s_up[program_no], 7);
          step_mode = (NUMPIXELS - ((s_up[program_no] + 1) / 3));
          if (s_up[program_no] < 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);  //yellow
            }
          } else if (s_up[program_no] < 26 && s_up[program_no] >= 12) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[5]);  //green
            }
          } else if (s_up[program_no] >= 26) {
            for (int i = NUMPIXELS; i >= step_mode; i--) {
              pixels.setPixelColor(i, COLOR_RGB[7]);  //red
            }
          }
          for (int i = 0; i < step_mode; i++) {
            pixels.setPixelColor(i, COLOR_RGB[0]);
          }
          pixels.show();
          if (on_wi_fi) webApp.reqHandShake();
        }
        if (keyNum == F8) {
          while (1) {
            Delay(50);
            if (keyNum != F8) break;
          }
          break;
        }
      }
      led_stat = led_stat & ~set_signal;  //set off
      seg_3(s_up[program_no], 7);
      for (int i = NUMPIXELS; i >= 0; i--) {
        pixels.setPixelColor(i, COLOR_RGB[10]);
      }
      pixels.show();
      map2 = false;
      update_data = true;
      seven_seg_1_copy = 10000;
      seven_seg_2_copy = 10000;
      seven_seg_3_copy = 10000;
      seven_seg_4_copy = 10000;
      seven_seg_5_copy = 10000;
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      temp_dis = temp_dis_2;
      con_temp = true;
      show_temp = millis();
    }
    lcd_disabled = false;
    dontcare_f10 = false;
    update_lec();
  } else p_function = 0;
}
void save_var(void) {
  if (update_data == true) {
    int check_var[50], unit_space_copy;
    float unit_welding_copy = 0, unit_welding_p_copy = 0;

    update_data = false;
    ///////////////////////////////////////////////////////////
    address = 0;
    check_var[0] = EEPROM.readInt(address);
    address = 4;
    check_var[1] = EEPROM.readInt(address);
    address = 8;
    check_var[2] = EEPROM.readInt(address);
    address = 12;

    check_var[3] = EEPROM.readInt(address);
    address = 16;
    check_var[4] = EEPROM.readInt(address);
    address = 20;
    check_var[5] = EEPROM.readInt(address);
    address = 24;

    check_var[6] = EEPROM.readInt(address);
    address = 28;
    check_var[7] = EEPROM.readInt(address);
    address = 32;
    check_var[8] = EEPROM.readInt(address);
    address = 36;

    check_var[9] = EEPROM.readInt(address);
    address = 40;
    check_var[10] = EEPROM.readInt(address);
    address = 44;
    check_var[11] = EEPROM.readInt(address);
    address = 48;

    check_var[12] = EEPROM.readInt(address);
    address = 52;
    check_var[13] = EEPROM.readInt(address);
    address = 56;
    check_var[14] = EEPROM.readInt(address);
    address = 60;

    check_var[15] = EEPROM.readInt(address);
    address = 64;

    check_var[16] = EEPROM.readInt(address);
    address = 68;
    check_var[17] = EEPROM.readInt(address);
    //////////////////////////cleaner //////////
    address = 72;
    check_var[18] = EEPROM.readInt(address);
    address = 76;
    check_var[19] = EEPROM.readInt(address);
    address = 80;
    check_var[20] = EEPROM.readInt(address);
    address = 84;
    check_var[21] = ((EEPROM.readInt(address)) > MODES_SIZE) ? MODE_WELDER_SINGLE : Mode(EEPROM.readInt(address));
    if (device_mode > MODES_SIZE || device_mode == 0)
      Mode(1);
    address = 88;
    check_var[22] = EEPROM.readInt(address);


    address = 92;
    check_var[23] = EEPROM.readInt(address);

    address = 96;
    check_var[24] = EEPROM.readInt(address);

    address = 100;
    check_var[25] = EEPROM.readInt(address);

    address = 104;
    check_var[26] = EEPROM.readInt(address);

    address = 108;
    check_var[27] = EEPROM.readInt(address);

    address = 112;
    check_var[28] = EEPROM.readInt(address);

    address = 116;
    check_var[29] = EEPROM.readInt(address);

    address = 120;
    check_var[30] = EEPROM.readInt(address);

    address = 124;
    check_var[31] = EEPROM.readInt(address);

    address = 128;
    check_var[32] = EEPROM.readInt(address);

    address = 132;
    check_var[33] = EEPROM.readInt(address);

    address = 136;
    check_var[34] = EEPROM.readInt(address);

    address = 144;
    check_var[35] = EEPROM.readInt(address);

    address = 148;
    check_var[36] = EEPROM.readInt(address);

    address = 152;
    check_var[37] = EEPROM.readInt(address);

    address = 156;
    check_var[38] = EEPROM.readInt(address);

    address = 160;
    check_var[39] = EEPROM.readInt(address);

    address = 164;
    check_var[40] = EEPROM.readInt(address);

    address = 168;
    check_var[41] = EEPROM.readInt(address);

    address = 172;
    check_var[42] = EEPROM.readInt(address);

    address = 176;
    float TempOffset_copy = EEPROM.readFloat(address);

    address = 380;
    check_var[43] = EEPROM.readInt(address);

    address = 344;
    check_var[44] = EEPROM.readInt(address);

    address = 348;
    check_var[45] = EEPROM.readInt(address);

    unit_space_copy = EEPROM.readInt(400);

    unit_welding_copy = EEPROM.readFloat((120 + unit_space) * 4);

    unit_welding_p_copy = EEPROM.readFloat((140 + unit_space) * 4);

    //////////////////////////////////////////////////////////////////////
    Serial.println("══════════ Enter Save Data ════════════");
    Serial.print("Number Of Saves : ");
    Serial.println(number_of_saves++);

    address = 0;
    bool etntr_eeprom = 0;
    if (check_var[0] != e_up[1]) {
      EEPROM.writeInt(address, e_up[1]);
      Serial.println("EEPROM write");
      Serial.print("e_up[1] = ");
      Serial.println(e_up[1]);
      etntr_eeprom = 1;
    }
    address = 4;
    if (check_var[1] != c_up[1]) {
      EEPROM.writeInt(address, c_up[1]);
      Serial.println("EEPROM write");
      Serial.print("c_up[1] = ");
      Serial.println(c_up[1]);
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      show_temp = millis();
      etntr_eeprom = 1;
    }
    address = 8;
    if (check_var[2] != s_up[1]) {
      EEPROM.writeInt(address, s_up[1]);
      Serial.println("EEPROM write");
      Serial.print("s_up[1] = ");
      Serial.println(s_up[1]);
      etntr_eeprom = 1;
    }
    address = 12;

    if (check_var[3] != e_up[2]) {
      EEPROM.writeInt(address, e_up[2]);
      Serial.println("EEPROM write");
      Serial.print("e_up[2] = ");
      Serial.println(e_up[2]);
      etntr_eeprom = 1;
    }
    address = 16;

    if (check_var[4] != c_up[2]) {
      EEPROM.writeInt(address, c_up[2]);
      Serial.println("EEPROM write");
      Serial.print("c_up[2] = ");
      Serial.println(c_up[2]);
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      show_temp = millis();
      etntr_eeprom = 1;
    }
    address = 20;
    if (check_var[5] != s_up[2]) {
      EEPROM.writeInt(address, s_up[2]);
      Serial.println("EEPROM write");
      Serial.print("s_up[2] = ");
      Serial.println(s_up[2]);
      etntr_eeprom = 1;
    }
    address = 24;

    if (check_var[6] != e_up[3]) {
      EEPROM.writeInt(address, e_up[3]);
      Serial.println("EEPROM write");
      Serial.print("e_up[3] = ");
      Serial.println(e_up[3]);
      etntr_eeprom = 1;
    }
    address = 28;

    if (check_var[7] != c_up[3]) {
      EEPROM.writeInt(address, c_up[3]);
      Serial.println("EEPROM write");
      Serial.print("c_up[3] = ");
      Serial.println(c_up[3]);
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      show_temp = millis();
      etntr_eeprom = 1;
    }
    address = 32;
    if (check_var[8] != s_up[3]) {
      EEPROM.writeInt(address, s_up[3]);
      Serial.println("EEPROM write");
      Serial.print("s_up[3] = ");
      Serial.println(s_up[3]);
      etntr_eeprom = 1;
    }
    address = 36;

    if (check_var[9] != e_up[4]) {
      EEPROM.writeInt(address, e_up[4]);
      Serial.println("EEPROM write");
      Serial.print("e_up[4] = ");
      Serial.println(e_up[4]);
      etntr_eeprom = 1;
    }
    address = 40;

    if (check_var[10] != c_up[4]) {
      EEPROM.writeInt(address, c_up[4]);
      Serial.println("EEPROM write");
      Serial.print("c_up[4] = ");
      Serial.println(c_up[4]);
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      show_temp = millis();
      etntr_eeprom = 1;
    }
    address = 44;

    if (check_var[11] != s_up[4]) {
      EEPROM.writeInt(address, s_up[4]);
      Serial.println("EEPROM write");
      Serial.print("s_up[4] = ");
      Serial.println(s_up[4]);
      etntr_eeprom = 1;
    }
    address = 48;

    if (check_var[12] != e_up[5]) {
      EEPROM.writeInt(address, e_up[5]);
      Serial.println("EEPROM write");
      Serial.print("e_up[5] = ");
      Serial.println(e_up[5]);
      etntr_eeprom = 1;
    }
    address = 52;
    if (check_var[13] != c_up[5]) {
      EEPROM.writeInt(address, c_up[5]);
      Serial.println("EEPROM write");
      Serial.print("c_up[5] = ");
      Serial.println(c_up[5]);
      c_up_copy = c_up[program_no] + 10;
      c_up_copy_n = c_up[program_no] - 10;
      show_temp = millis();
      etntr_eeprom = 1;
    }
    address = 56;
    if (check_var[14] != s_up[5]) {
      EEPROM.writeInt(address, s_up[5]);
      Serial.println("EEPROM write");
      Serial.print("s_up[5] = ");
      Serial.println(s_up[5]);
      etntr_eeprom = 1;
    }
    address = 60;

    if (check_var[15] != program_no) {
      EEPROM.writeInt(address, program_no);
      Serial.println("EEPROM write");
      Serial.print("program_no = ");
      Serial.println(program_no);
      etntr_eeprom = 1;
    }
    address = 64;

    if (check_var[16] != PID) {
      EEPROM.writeInt(address, PID);
      Serial.println("EEPROM write");
      Serial.print("PID = ");
      Serial.println(PID);
      etntr_eeprom = 1;
    }
    address = 68;
    if (check_var[17] != isJ) {
      EEPROM.writeInt(address, isJ);
      Serial.println("EEPROM write");
      Serial.print("isJ = ");
      if (isJ) {
        Serial.println("K");
      } else Serial.println("J");
      etntr_eeprom = 1;
    }
    ////////////////////////cleaner /////////////////
    address = 72;
    if (check_var[18] != blade_mode) {
      EEPROM.writeInt(address, blade_mode);
      Serial.println("EEPROM write");
      Serial.print("blade_mode = ");
      etntr_eeprom = 1;
      switch (blade_mode) {
        case 1:
          Serial.println("WING");
          break;

        case 2:
          Serial.println("FRAME");
          break;

        case 3:
          Serial.println("DOOR");
          break;

        case 4:
          Serial.println("B.DOOR");
          break;

        default:
          Serial.println("NO");
          break;
      }
    }
    address = 76;
    if (check_var[19] != ability) {
      EEPROM.writeInt(address, ability);
      Serial.println("EEPROM write");
      Serial.print("ability = ");
      etntr_eeprom = 1;
      switch (ability) {
        case 1:
          Serial.println("FULL");
          break;

        case 2:
          Serial.println("SETTING");
          break;

        case 3:
          Serial.println("SCRAPE");
          break;

        case 4:
          Serial.println("MILLING");
          break;

        default:
          Serial.println("NO");
          break;
      }
    }
    address = 80;
    if (check_var[20] != motor_type) {
      EEPROM.writeInt(address, motor_type);
      Serial.println("EEPROM write");
      Serial.print("motor_type = ");
      etntr_eeprom = 1;
      switch (motor_type) {
        case 1:
          Serial.println("Motor 1");
          break;

        case 2:
          Serial.println("Motor 2");
          break;

        default:
          Serial.println("No Motor");
          break;
      }
    }
    address = 84;
    if (check_var[21] != device_mode) {
      EEPROM.writeInt(address, device_mode);
      Serial.println("EEPROM write");
      Serial.print("device_mode = ");
      etntr_eeprom = 1;
      switch (device_mode) {
        case 1:
          Serial.println("MODE WELDER SINGLE");
          break;

        case 2:
          Serial.println("MODE WELDER DOU");
          break;

        case 3:
          Serial.println("MODE_CLEANER");
          break;

        case 4:
          Serial.println("DIGITAL_METER_MODE");
          break;

        default:
          Serial.println("No Mode");
          break;
      }
    }
    address = 88;
    if (check_var[22] != buzzer_en) {
      EEPROM.writeInt(address, buzzer_en);
      Serial.println("EEPROM write");
      Serial.print("Buzzer = ");
      etntr_eeprom = 1;
      if (buzzer_en == 1) {
        Serial.println("Enable");
      } else Serial.println("Disabled");
    }

    ////////////////////////time jack /////////////////
    address = 92;
    if (check_var[23] != ax_ready) {
      EEPROM.writeInt(address, ax_ready);
      Serial.println("EEPROM write");
      Serial.print("The Time Of Arrival Of The Ax(mS) = ");
      Serial.println(ax_ready);
      etntr_eeprom = 1;
    }

    address = 96;
    if (check_var[24] != ax_back) {
      EEPROM.writeInt(address, ax_back);
      Serial.println("EEPROM write");
      Serial.print("Time To Return The Ax(mS) = ");
      Serial.println(ax_back);
      etntr_eeprom = 1;
    }

    address = 100;
    if (check_var[25] != table_ready) {
      EEPROM.writeInt(address, table_ready);
      Serial.println("EEPROM write");
      Serial.print("The Time Of Arrival Of The Table(mS) = ");
      Serial.println(table_ready);
      etntr_eeprom = 1;
    }

    address = 104;
    if (check_var[26] != table_back) {
      EEPROM.writeInt(address, table_back);
      Serial.println("EEPROM write");
      Serial.print("Time To Return The Table(mS) = ");
      Serial.println(table_back);
      etntr_eeprom = 1;
    }

    address = 108;
    if (check_var[27] != heater_ready) {
      EEPROM.writeInt(address, heater_ready);
      Serial.println("EEPROM write");
      Serial.print("The Time Of Arrival Of The Heater(mS) = ");
      Serial.println(heater_ready);
      etntr_eeprom = 1;
    }

    address = 112;
    if (check_var[28] != heater_back) {
      EEPROM.writeInt(address, heater_back);
      Serial.println("EEPROM write");
      Serial.print("Time To Return The Heater(mS) = ");
      Serial.println(heater_back);
      etntr_eeprom = 1;
    }


    address = 160;
    if (check_var[39] != press_ready) {
      EEPROM.writeInt(address, press_ready);
      Serial.println("EEPROM write");
      Serial.print("Press Ready (ms) = ");
      Serial.println(press_ready);
      etntr_eeprom = 1;
    }

    address = 164;
    if (check_var[40] != press_back) {
      EEPROM.writeInt(address, press_back);
      Serial.println("EEPROM write");
      Serial.print("Press Back (ms) = ");
      Serial.println(press_back);
      etntr_eeprom = 1;
    }

    address = 144;
    if (check_var[35] != tap_jack_1) {
      EEPROM.writeInt(address, tap_jack_1);
      Serial.println("EEPROM write");
      Serial.print("Tap Jack 1 (S)= ");
      Serial.println(tap_jack_1);
      etntr_eeprom = 1;
    }

    address = 148;
    if (check_var[36] != tap_jack_2) {
      EEPROM.writeInt(address, tap_jack_2);
      Serial.println("EEPROM write");
      Serial.print("Tap Jack 2 (S)= ");
      Serial.println(tap_jack_2);
      etntr_eeprom = 1;
    }

    address = 152;
    if (check_var[37] != stat_stop) {
      EEPROM.writeInt(address, stat_stop);
      Serial.println("EEPROM write");
      Serial.print("Stop Stat = ");
      if (stat_stop) {
        Serial.println("NO");
      } else Serial.println("NC");
      etntr_eeprom = 1;
    }
    //////////////////////// lock /////////////////
    address = 116;
    if (check_var[29] != lock_time) {
      EEPROM.writeInt(address, lock_time);
      Serial.println("EEPROM write");
      Serial.print("System Lock Time(DAY) = ");
      Serial.println(lock_time);
      etntr_eeprom = 1;
    }
    address = 120;
    if (check_var[30] != lock_timeing) {
      EEPROM.writeInt(address, lock_timeing);
      Serial.println("EEPROM write");
      Serial.print("Elapsed Time Of System Lock(DAY) = ");
      Serial.println(lock_timeing);
      etntr_eeprom = 1;
    }
    address = 124;
    if (check_var[31] != password_p) {
      EEPROM.writeInt(address, password_p);
      Serial.println("EEPROM write");
      Serial.print("Password = ");
      Serial.println(password_p);
      etntr_eeprom = 1;
    }
    address = 128;
    if (check_var[32] != lock_en) {
      EEPROM.writeInt(address, lock_en);
      Serial.println("EEPROM write");
      Serial.print("System lock = ");
      etntr_eeprom = 1;
      if (lock_en) {
        Serial.println("Enable");
      } else Serial.println("Disabled");
    }

    address = 132;
    if (check_var[33] != adding_time) {
      EEPROM.writeInt(address, adding_time);
      Serial.println("EEPROM write");
      Serial.print("Adding Time (D)= ");
      Serial.println(adding_time);
      etntr_eeprom = 1;
    }

    address = 136;
    if (check_var[34] != hardware_test) {
      EEPROM.writeInt(address, hardware_test);
      Serial.println("EEPROM write");
      Serial.print("Hardware Test = ");
      if (hardware_test) {
        Serial.println("Enable");
      } else Serial.println("Disabled");
      etntr_eeprom = 1;
    }

    address = 156;
    if (check_var[38] != val_ok) {
      EEPROM.writeInt(address, val_ok);
      Serial.println("EEPROM write");
      Serial.print("OK Temp Stat = ");
      if (val_ok) {
        Serial.println("Active");
      } else Serial.println("Disabel");
      etntr_eeprom = 1;
    }

    address = 168;
    if (check_var[41] != finish_time) {
      EEPROM.writeInt(address, finish_time);
      Serial.println("EEPROM write");
      Serial.print("Finish Time = ");
      if (finish_time) {
        Serial.println("Active");
      } else Serial.println("Disabel");
      etntr_eeprom = 1;
    }

    address = 172;
    if (check_var[42] != auto_start) {
      EEPROM.writeInt(address, auto_start);
      Serial.println("EEPROM write");
      Serial.print("Auto Start = ");
      if (auto_start) {
        Serial.println("Active");
      } else Serial.println("Disabel");
      etntr_eeprom = 1;
    }

    address = 176;
    if (TempOffset_copy != temp_offset) {
      EEPROM.writeFloat(address, temp_offset);
      Serial.println("EEPROM write");
      Serial.print("Temp Offset = ");
      Serial.println(temp_offset);
      etntr_eeprom = 1;
    }

    address = 380;
    if (check_var[43] != lan_) {
      EEPROM.writeInt(address, lan_);
      Serial.println("EEPROM write");
      Serial.print("Lang = ");
      if (lan_ == 1) {
        Serial.println("FA");
      } else if (lan_ == 2) {
        Serial.println("EN");
      } else if (lan_ == 3) {
        Serial.println("TU");
      } else if (lan_ == 4) {
        Serial.println("RO");
      }
      etntr_eeprom = 1;
    }

    address = 344;
    if (check_var[44] != jackSensore_enabled) {
      EEPROM.writeInt(address, jackSensore_enabled);
      Serial.println("EEPROM write");
      Serial.print("jackSensore= ");
      if (jackSensore_enabled == 0) {
        Serial.println("Disabel Sensor");
      } else Serial.println("Active Sensor");
      etntr_eeprom = 1;
    }

    address = 348;
    if (check_var[45] != pedal_Count) {
      EEPROM.writeInt(address, pedal_Count);
      Serial.println("EEPROM write");
      Serial.print("Pedal Count = ");
      Serial.println(pedal_Count);
      etntr_eeprom = 1;
    }

    if (unit_welding_p > (50000 * (unit_space + 1))) {  //90000
      if (unit_space < 18) unit_space++;
      unit_space_copy = unit_space;
      EEPROM.write(400, unit_space);
      Serial.print("Save Unit Space = ");
      Serial.println(unit_space);
      etntr_eeprom = 1;
    }

    if (unit_space_copy != unit_space) {
      EEPROM.writeInt(400, unit_space);
      Serial.println("EEPROM write");
      Serial.print("Unit Space = ");
      Serial.println(unit_space);
      etntr_eeprom = 1;
    }

    if (unit_welding_copy != unit_welding) {
      EEPROM.writeFloat((120 + unit_space) * 4, unit_welding);  // 480
      Serial.println("EEPROM write");
      Serial.print("Unit Welding = ");
      Serial.println(unit_welding);
      etntr_eeprom = 1;
    }

    if (unit_welding_p_copy != unit_welding_p) {
      EEPROM.writeFloat((140 + unit_space) * 4, unit_welding_p);  // 480
      Serial.println("EEPROM write");
      Serial.print("Unit Welding P = ");
      Serial.println(unit_welding_p);
      etntr_eeprom = 1;
    }

    if (etntr_eeprom == 1) {
      buzzer(2);
      EEPROM.commit();
    } else buzzer(1);

    address = 0;
    eTime[program_no] = e_up[program_no];
    cTemp[program_no] = c_up[program_no];
    sTime[program_no] = s_up[program_no];
    Serial.println("save eeprom");
    Serial.println("════════════════ END ══════════════════");
  }
}
void select_function(void) {
  average();
  ////////////////////////////// F1 & F6 ///////////////////////////
  bool en_save = 1;
  if ((ADC_1 > 380) && (ADC_1 < 430)) {
    delay(3);
    average();

    if ((ADC_2 > 380) && (ADC_2 < 430)) {
      delay(3);
      average();
      if ((ADC_1 > 380) && (ADC_1 < 430)) {
        break_rgb_fun = 1;
        led_stat = led_stat | set_signal;
        Delay(1);
        for (int i = NUMPIXELS; i >= 0; i--) {
          pixels.setPixelColor(i, COLOR_RGB[0]);
        }
        pixels.show();
        led_stat_copy = led_stat;
        up_seg_step(1, E_E, S_, OFF_, OFF_, OFF_, OFF_, 7);
        down_seg(2, E_E, D_, OFF_, 7);

        while (1) {
          Delay(1);
          if (keyNum == 100) break;
        }
        delay(50);
        while (1) {
          Delay(20);
          if (keyNum == F1) {
            device_mode = MODE_WELDER_SINGLE;
            up_seg_step(S_, I_, N_, G_, L_, E_, OFF_, 7);
            Delay(1500);
            break;
          }
          if (keyNum == F2) {
            device_mode = MODE_WELDER_DOUBLE;
            up_seg_step(D_, O_, U_, B_, L_, E_, OFF_, 7);
            Delay(1500);
            jackSensore_enabled = 0;
            break;
          }
        }
        up_seg_step(P_, I_, D_, E_E, E_E, ON_, ON_, 7);  // PID==
        seg_3(PID, 7);
        seg_5(0, 7);
        for (int i = NUMPIXELS; i >= 0; i--) {
          pixels.setPixelColor(i, COLOR_RGB[0]);
        }
        for (int i = NUMPIXELS; i > NUMPIXELS - 3; i--) {
          pixels.setPixelColor(i, COLOR_RGB[2]);
        }
        pixels.show();
        while (1) {
          Delay(1);
          if (keyNum == 100) break;
        }
        delay(100);

        int isJ_copy = isJ;
        int step_mode = 0;
        bool stat_f8 = 0, stat_stop_copy = stat_stop;

        while (1) {
          Delay(20);
          /////////////////////////////// F7 //////////////////////
          if (keyNum == F7) {
            up_seg_step(C_, A_, N_, C_, E_, L_, E_, 7);
            en_save = 0;
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[7]);
            }
            pixels.show();
            read_write_eeprom(0);
            Delay(1000);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[10]);
            }
            pixels.show();
            break;
          }
          /////////////////////////////// F8 //////////////////////
          if (keyNum == F8) {
            seven_seg_3_copy = 10000;
            if (stat_f8 == 0) {
              stat_f8 = 1;
              if (step_mode < 5) {
                step_mode++;
                if (device_mode == MODE_WELDER_DOUBLE && step_mode == 5) step_mode = 0;
              } else step_mode = 0;

              seg_5(step_mode, 7);
              for (int i = NUMPIXELS; i >= 0; i--) {
                pixels.setPixelColor(i, COLOR_RGB[0]);
              }
              for (int i = NUMPIXELS; i >= (NUMPIXELS - (step_mode * 2.2)); i--) {
                pixels.setPixelColor(i, COLOR_RGB[2]);
              }
              pixels.show();

              if (step_mode == 0) {
                up_seg(P_, I_, D_, E_E, E_E, ON_, ON_, 7);  // PID==
                seg_3(PID, 7);
              } else if (step_mode == 1) {
                if (isJ) {
                  up_seg(T_, E_, M_, P_, E_E, E_E, K_, 7);       // TEMP==K
                } else up_seg(T_, E_, M_, P_, E_E, E_E, J_, 7);  // TEMP==J
              } else if (step_mode == 2) {
                if (stat_stop_copy) {
                  up_seg(S_, T_, O_, P_, E_E, N_, o_, 7);       // stop == no
                } else up_seg(S_, T_, O_, P_, E_E, N_, c_, 7);  // stop == nc
              } else if (step_mode == 3) {
                if (val_ok) {
                  up_seg(o_, K_, E_E, E_E, A_, c_, T_, 7);       // ok temp == Active
                } else up_seg(o_, K_, E_E, E_E, D_, I_, S_, 7);  // ok temp == Disabel
              } else if (step_mode == 4) {
                if (pedal_Count == 3) {
                  up_seg(P_, E_, D_, A_, L_, E_E, 3, 7);       // Pedal == 3
                } else up_seg(P_, E_, D_, A_, L_, E_E, 4, 7);  // Pedal == 4
              } else if (step_mode == 5) {
                if (jackSensore_enabled) {
                  up_seg(S_, E_, N_, E_E, A_, c_, T_, 7);       // Sensor == Active
                } else up_seg(S_, E_, N_, E_E, D_, I_, S_, 7);  // Sensor == Disabel
              }
            }
          } else stat_f8 = 0;

          /////////////////////////////// F9 //////////////////////
          if (keyNum == F9) {
            if (step_mode == 0) {
              if (PID < 100)
                PID++;
              if (on_wi_fi) webApp.reqHandShake();
              seg_3(PID, 7);
            } else if (step_mode == 1) {
              isJ = !isJ;
              if (on_wi_fi) webApp.reqHandShake();
              if (isJ) {
                up_seg_step(T_, E_, M_, P_, E_E, E_E, K_, 7);       // TEMP==K
              } else up_seg_step(T_, E_, M_, P_, E_E, E_E, J_, 7);  // TEMP==J
            } else if (step_mode == 2) {
              stat_stop_copy = !stat_stop_copy;
              if (on_wi_fi) webApp.reqHandShake();
              if (stat_stop_copy) {
                up_seg_step(S_, T_, O_, P_, E_E, N_, o_, 7);       // stop == no
              } else up_seg_step(S_, T_, O_, P_, E_E, N_, c_, 7);  // stop == nc
            } else if (step_mode == 3) {
              val_ok = !val_ok;
              if (on_wi_fi) webApp.reqHandShake();
              if (val_ok) {
                up_seg_step(o_, K_, E_E, E_E, A_, c_, T_, 7);       // ok temp == Active
              } else up_seg_step(o_, K_, E_E, E_E, D_, I_, S_, 7);  // ok temp == Disabel
            } else if (step_mode == 4) {
              if (pedal_Count == 3) {
                pedal_Count = 4;
              } else pedal_Count = 3;
              if (on_wi_fi) webApp.reqHandShake();
              if (pedal_Count == 3) {
                up_seg(P_, E_, D_, A_, L_, E_E, 3, 7);       // Pedal == 3
              } else up_seg(P_, E_, D_, A_, L_, E_E, 4, 7);  // Pedal == 4
            } else if (step_mode == 5) {
              jackSensore_enabled = !jackSensore_enabled;
              if (on_wi_fi) webApp.reqHandShake();
              if (jackSensore_enabled) {
                up_seg(S_, E_, N_, E_E, A_, c_, T_, 7);       // Sensor == Active
              } else up_seg(S_, E_, N_, E_E, D_, I_, S_, 7);  // Sensor == Disabel
            }
            Delay(200);
          }

          /////////////////////////////// F10 //////////////////////
          if (keyNum == F10) {
            if (step_mode == 0) {
              if (PID > 0)
                PID--;
              if (on_wi_fi) webApp.reqHandShake();
              seg_3(PID, 7);
            } else if (step_mode == 1) {
              isJ = !isJ;
              if (on_wi_fi) webApp.reqHandShake();
              if (isJ) {
                up_seg_step(T_, E_, M_, P_, E_E, E_E, K_, 7);       // TEMP==K
              } else up_seg_step(T_, E_, M_, P_, E_E, E_E, J_, 7);  // TEMP==J
            } else if (step_mode == 2) {
              stat_stop_copy = !stat_stop_copy;
              if (on_wi_fi) webApp.reqHandShake();
              if (stat_stop_copy) {
                up_seg_step(S_, T_, O_, P_, E_E, N_, o_, 7);       // stop == no
              } else up_seg_step(S_, T_, O_, P_, E_E, N_, c_, 7);  // stop == nc
            } else if (step_mode == 3) {
              val_ok = !val_ok;
              if (on_wi_fi) webApp.reqHandShake();
              if (val_ok) {
                up_seg_step(o_, K_, E_E, E_E, A_, c_, T_, 7);       // ok temp == Active
              } else up_seg_step(o_, K_, E_E, E_E, D_, I_, S_, 7);  // ok temp == Disabel
            } else if (step_mode == 4) {
              if (pedal_Count == 3) {
                pedal_Count = 4;
              } else pedal_Count = 3;
              if (on_wi_fi) webApp.reqHandShake();
              if (pedal_Count == 3) {
                up_seg(P_, E_, D_, A_, L_, E_E, 3, 7);       // Pedal == 3
              } else up_seg(P_, E_, D_, A_, L_, E_E, 4, 7);  // Pedal == 4
            } else if (step_mode == 5) {
              jackSensore_enabled = !jackSensore_enabled;
              if (on_wi_fi) webApp.reqHandShake();
              if (jackSensore_enabled) {
                up_seg(S_, E_, N_, E_E, A_, c_, T_, 7);       // Sensor == Active
              } else up_seg(S_, E_, N_, E_E, D_, I_, S_, 7);  // Sensor == Disabel
            }
            Delay(200);
          }

          /////////////////////////////// F6 //////////////////////
          if (keyNum == F6) {
            up_seg_step(OFF_, OFF_, S_, A_, V_, E_, OFF_, 7);
            down_seg(OFF_, OFF_, OFF_, OFF_, 7);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[5]);
            }
            pixels.show();
            Delay(1000);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[10]);
            }
            pixels.show();
            dontcare_stop = true;
            while (1) {
              if (read_key(Stop) != stat_stop_copy) break;
              up_seg_step(OFF_, S_, T_, o_, P_, OFF_, OFF_, 7);
              error_display(2);
            }
            stat_stop = stat_stop_copy;
            break;
          }
        }
        Delay(1);
      }
      if (en_save == 1) update_data = true;
      update_f();
      dontcare_stop = false;
    }
  }
  //////////////////////////// F2 & F6 ///////////////////////////
  if ((ADC_1 > 300) && (ADC_1 < 380)) {
    delay(3);
    average();

    if ((ADC_2 > 300) && (ADC_2 < 380)) {
      delay(3);
      average();
      if ((ADC_1 > 300) && (ADC_1 < 380)) {
        break_rgb_fun = 1;
        led_stat = led_stat | set_signal;
        Delay(1);
        for (int i = NUMPIXELS; i >= 0; i--) {
          pixels.setPixelColor(i, COLOR_RGB[0]);
        }
        pixels.show();
        led_stat_copy = led_stat;
        up_seg_step(P_, E_, R_, I_, O_, D_, OFF_, 7);
        down_seg(J_, A_, C_, OFF_, 7);

        Delay(3000);

        down_seg(OFF_, OFF_, OFF_, OFF_, 7);
        while (1) {
          Delay(1);
          if (keyNum == 100) break;
        }
        delay(100);
        int step_mode = 0;
        bool p_stat_f9 = 0, p_stat_f10 = 0, p_stat_f8 = 0;
        bool refresh_lcd = 1;
        seg_5(step_mode, 7);
        for (int i = NUMPIXELS; i >= (NUMPIXELS - (step_mode + 1)); i--) {
          pixels.setPixelColor(i, COLOR_RGB[2]);
        }
        pixels.show();
        digitalWrite(shablon, Enable);
        while (1) {
          Delay(20);
          /////////////////////////////// F8 //////////////////////
          if (keyNum == F8) {
            if (p_stat_f8 == 0) {
              p_stat_f8 = 1;
              refresh_lcd = 1;
              if (step_mode < 11) {
                step_mode++;
              } else step_mode = 0;

              if (step_mode < 10) {
                seg_5(step_mode, 7);
              } else seg_5(step_mode + 11, 7);

              for (int i = NUMPIXELS; i >= 0; i--) {
                pixels.setPixelColor(i, COLOR_RGB[0]);
              }
              for (int i = NUMPIXELS; i >= (NUMPIXELS - step_mode); i--) {
                pixels.setPixelColor(i, COLOR_RGB[2]);
              }
              pixels.show();

              if (step_mode == 0) {
                digitalWrite(shablon, Enable);
              }
              if (step_mode == 2) {
                digitalWrite(shablon, Disabled);
                digitalWrite(jack_left, Enable);
                digitalWrite(jack_righ, Enable);
              }
              if (step_mode == 4) {
                digitalWrite(jack_left, Disabled);
                digitalWrite(jack_righ, Disabled);
                digitalWrite(move_jack, Enable);
              }
              if (step_mode == 6) {
                digitalWrite(move_jack, Disabled);
                delay(300);
                digitalWrite(jack_heat, Enable);
              }
              if (step_mode == 8) {
                digitalWrite(jack_heat, Disabled);
                delay(300);
                digitalWrite(tap_move_1, Enable);
              }
              if (step_mode == 9) {
                digitalWrite(tap_move_1, Disabled);
                digitalWrite(tap_move_2, Enable);
              }
              if (step_mode == 10) {
                digitalWrite(tap_move_2, Disabled);
              }
            }
          } else p_stat_f8 = 0;

          /////////////////////////////// F9 //////////////////////
          if (keyNum == F9) {
            if (p_stat_f9 == 0) {
              p_stat_f9 = 1;
              refresh_lcd = 1;
              if (step_mode == 0 && ax_ready < 9900) ax_ready = ax_ready + 100;
              if (step_mode == 1 && ax_back < 9900) ax_back = ax_back + 100;

              if (step_mode == 2 && press_ready < 9900) press_ready = press_ready + 100;
              if (step_mode == 3 && press_back < 9900) press_back = press_back + 100;

              if (step_mode == 4 && table_ready < 9900) table_ready = table_ready + 100;
              if (step_mode == 5 && table_back < 9900) table_back = table_back + 100;

              if (step_mode == 6 && heater_ready < 9900) heater_ready = heater_ready + 100;
              if (step_mode == 7 && heater_back < 9900) heater_back = heater_back + 100;

              if (step_mode == 8 && tap_jack_1 < s_up[program_no]) tap_jack_1++;
              if (step_mode == 9 && tap_jack_2 < s_up[program_no]) tap_jack_2++;

              if (step_mode == 10) finish_time = !finish_time;

              if (step_mode == 11) auto_start = !auto_start;
            }
          } else p_stat_f9 = 0;

          /////////////////////////////// F10 //////////////////////
          if (keyNum == F10) {
            if (p_stat_f10 == 0) {
              p_stat_f10 = 1;
              refresh_lcd = 1;
              if (step_mode == 0 && ax_ready > 200) ax_ready = ax_ready - 100;
              if (step_mode == 1 && ax_back > 200) ax_back = ax_back - 100;

              if (step_mode == 2 && press_ready > 200) press_ready = press_ready - 100;
              if (step_mode == 3 && press_back > 200) press_back = press_back - 100;

              if (step_mode == 4 && table_ready > 200) table_ready = table_ready - 100;
              if (step_mode == 5 && table_back > 200) table_back = table_back - 100;

              if (step_mode == 6 && heater_ready > 200) heater_ready = heater_ready - 100;
              if (step_mode == 7 && heater_back > 200) heater_back = heater_back - 100;

              if (step_mode == 8 && tap_jack_1 >= 1) tap_jack_1--;
              if (step_mode == 9 && tap_jack_2 >= 1) tap_jack_2--;

              if (step_mode == 10) finish_time = !finish_time;

              if (step_mode == 11) auto_start = !auto_start;
            }
          } else p_stat_f10 = 0;

          if (refresh_lcd == 1) {
            refresh_lcd = 0;
            if (step_mode == 0) up_seg(A_, OFF_, F_, O_, E_E, (ax_ready / 1000) + 10, (ax_ready / 100) - ((ax_ready / 1000) * 10), 7);

            if (step_mode == 1) up_seg(A_, OFF_, B_, A_, E_E, (ax_back / 1000) + 10, (ax_back / 100) - ((ax_back / 1000) * 10), 7);

            if (step_mode == 2) up_seg(P_, OFF_, F_, O_, E_E, (press_ready / 1000) + 10, (press_ready / 100) - ((press_ready / 1000) * 10), 7);

            if (step_mode == 3) up_seg(P_, OFF_, B_, A_, E_E, (press_back / 1000) + 10, (press_back / 100) - ((press_back / 1000) * 10), 7);

            if (step_mode == 4) up_seg(T_, OFF_, F_, O_, E_E, (table_ready / 1000) + 10, (table_ready / 100) - ((table_ready / 1000) * 10), 7);

            if (step_mode == 5) up_seg(T_, OFF_, B_, A_, E_E, (table_back / 1000) + 10, (table_back / 100) - ((table_back / 1000) * 10), 7);

            if (step_mode == 6) up_seg(H_, OFF_, F_, O_, E_E, (heater_ready / 1000) + 10, (heater_ready / 100) - ((heater_ready / 1000) * 10), 7);

            if (step_mode == 7) up_seg(H_, OFF_, B_, A_, E_E, (heater_back / 1000) + 10, (heater_back / 100) - ((heater_back / 1000) * 10), 7);

            if (step_mode == 8) up_seg(J_, OFF_, T_, 1, E_E, (tap_jack_1 / 10), tap_jack_1 - ((tap_jack_1 / 10) * 10), 7);

            if (step_mode == 9) up_seg(J_, OFF_, T_, 2, E_E, (tap_jack_2 / 10), tap_jack_2 - ((tap_jack_2 / 10) * 10), 7);

            if (step_mode == 10) {
              if (finish_time) {
                up_seg(E_, N_, D_, E_E, A_, C_, T_, 7);
              } else up_seg(E_, N_, D_, E_E, D_, I_, S_, 7);
            }

            if (step_mode == 11) {
              if (auto_start) {
                up_seg(S_, T_, A_, E_E, A_, C_, T_, 7);
              } else up_seg(S_, T_, A_, E_E, D_, I_, S_, 7);
            }
          }

          if (keyNum == F7) {
            rest_pin_out();
            seg_5(0, 7);
            for (int i = NUMPIXELS; i >= 0; i--) {
              pixels.setPixelColor(i, COLOR_RGB[0]);
            }
            pixels.setPixelColor(11, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(shablon, Enable);
            Serial.print("shablon ON (mS) = ");
            Serial.println(ax_ready);
            for (int i = ax_ready; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(A_, OFF_, D_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(0, 7);
            pixels.setPixelColor(10, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(move_jack, Enable);
            Serial.print("move_jack ON (mS) = ");
            Serial.println(table_ready);
            for (int i = table_ready; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(T_, OFF_, D_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(1, 7);
            pixels.setPixelColor(9, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(jack_left, Enable);
            digitalWrite(jack_righ, Enable);
            if (device_mode == MODE_WELDER_DOUBLE) {
              digitalWrite(jack_left_2, Enable);
              digitalWrite(jack_righ_2, Enable);
            }
            Serial.print("Press ON (mS) = ");
            Serial.println(press_ready);
            for (int i = press_ready; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(P_, OFF_, D_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(2, 7);
            pixels.setPixelColor(8, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(move_jack, Disabled);
            Serial.print("move_jack OFF (mS) = ");
            Serial.println(table_back);
            for (int i = table_back; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(T_, OFF_, B_, A_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(3, 7);
            pixels.setPixelColor(7, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(shablon, Disabled);
            Serial.print("shablon OFF (S) = ");
            Serial.println(ax_back);
            for (int i = ax_back; i >= 0; i = i - 100) {
              Delay(33);
              up_seg(A_, OFF_, B_, A_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(4, 7);
            pixels.setPixelColor(6, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(jack_heat, Enable);
            Serial.print("move_jack (mS) = ");
            Serial.println(heater_ready);
            for (int i = heater_ready; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(H_, OFF_, D_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(5, 7);
            pixels.setPixelColor(5, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(move_jack, Enable);
            Serial.print("move_jack ON (mS) = ");
            Serial.println(table_ready);
            for (int i = table_ready; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(T_, OFF_, D_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(6, 7);
            pixels.setPixelColor(4, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(move_jack, Disabled);
            Serial.print("move_jack OFF (mS) = ");
            Serial.println(table_back);
            for (int i = table_back; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(T_, OFF_, B_, A_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(7, 7);
            pixels.setPixelColor(3, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(jack_heat, Disabled);
            Serial.print("heater_back (mS) = ");
            Serial.println(heater_back);
            for (int i = heater_back; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(T_, OFF_, B_, A_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(8, 7);
            pixels.setPixelColor(2, COLOR_RGB[2]);
            pixels.show();
            digitalWrite(move_jack, Enable);
            Serial.print("move_jack ON (mS) = ");
            Serial.println(table_ready);
            for (int i = table_ready; i >= 0; i = i - 100) {
              Delay(32);
              up_seg(T_, OFF_, D_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
            }

            seg_5(9, 7);
            pixels.setPixelColor(1, COLOR_RGB[2]);
            pixels.show();
            if (finish_time) {
              digitalWrite(jack_left, Disabled);
              if (device_mode == MODE_WELDER_DOUBLE) {
                digitalWrite(jack_left_2, Disabled);
              }
              Serial.print("Press OFF (mS) = ");
              Serial.println(press_back);
              for (int i = press_back; i >= 0; i = i - 100) {
                Delay(32);
                up_seg(P_, OFF_, B_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
              }

              seg_5(A_, 7);
              pixels.setPixelColor(0, COLOR_RGB[2]);
              pixels.show();
              digitalWrite(move_jack, Disabled);
              Serial.print("move_jack OFF (mS) = ");
              Serial.println(table_back);
              for (int i = table_back; i >= 0; i = i - 100) {
                Delay(32);
                up_seg(T_, OFF_, B_, A_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
              }
              digitalWrite(jack_righ, Disabled);
              if (device_mode == MODE_WELDER_DOUBLE) {
                digitalWrite(jack_righ_2, Disabled);
              }
            } else {
              digitalWrite(jack_left, Disabled);
              digitalWrite(jack_righ, Disabled);
              if (device_mode == MODE_WELDER_DOUBLE) {
                digitalWrite(jack_left_2, Disabled);
                digitalWrite(jack_righ_2, Disabled);
              }
              Serial.print("Press OFF (S) = ");
              Serial.println(press_back);
              for (int i = press_back; i >= 0; i = i - 100) {
                Delay(32);
                up_seg(P_, OFF_, B_, I_, E_E, (i / 1000) + 10, (i / 100) - ((i / 1000) * 10), 7);
              }
              pixels.setPixelColor(0, COLOR_RGB[2]);
              pixels.show();
            }
            Serial.print("tap_jack_1 (S) = ");
            Serial.println(tap_jack_1);

            Serial.print("tap_jack_2 (S) = ");
            Serial.println(tap_jack_2);
            Serial.println("finsh");
            refresh_lcd = 1;
            step_mode = 0;
            buzzer(2);
            Delay(1000);
            digitalWrite(shablon, Enable);
            seg_5(step_mode, 7);
            for (int i = NUMPIXELS; i >= 0; i--) {
              pixels.setPixelColor(i, COLOR_RGB[0]);
            }
            for (int i = NUMPIXELS; i >= (NUMPIXELS - (step_mode + 1)); i--) {
              pixels.setPixelColor(i, COLOR_RGB[2]);
            }
            pixels.show();
          }

          if (keyNum == F6) {
            up_seg_step(OFF_, OFF_, S_, A_, V_, E_, OFF_, 7);
            down_seg(OFF_, OFF_, OFF_, OFF_, 7);
            break;
          }
        }
        rest_pin_out();
        update_data = true;
        Delay(1000);
      }
    }
  }
  //////////////////////////// F3 & F7 ///////////////////////////
  if ((ADC_1 > 230) && (ADC_1 < 300)) {
    delay(3);
    average();

    if ((ADC_2 > 230) && (ADC_2 < 300)) {
      delay(3);
      average();
      if ((ADC_1 > 230) && (ADC_1 < 300)) {
        break_rgb_fun = 1;
        led_stat = led_stat | set_signal;
        Delay(1);
        led_stat_copy = led_stat;
        for (int i = NUMPIXELS; i >= 0; i--) {
          pixels.setPixelColor(i, COLOR_RGB[0]);
        }
        pixels.show();
        up_seg_step(R_, S_, T_, F_, A_, C_, T_, 7);
        down_seg(OFF_, OFF_, OFF_, OFF_, 7);
        Delay(1000);
        bool set_time = 0;
        while (1) {
          Delay(20);
          /////////////////////////////// F6 //////////////////////
          if (keyNum == F6) {
            Serial.println("Rest Factory");
            read_write_eeprom(1);
            up_seg_step(S_, U_, C_, C_, E_, S_, S_, 7);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[5]);
            }
            pixels.show();
            Delay(1500);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[0]);
            }
            pixels.show();
            break;
          }
          /////////////////////////////// F7 //////////////////////
          if (keyNum == F7) {
            if (val_ds3231) {
              up_seg_step(E_, N_, T_, R_, P_, A_, S_, 7);
              Delay(2000);
              set_time = 1;
            } else {
              up_seg_step(N_, O_, E_E, R_, T_, c_, OFF_, 7);
              for (int i = 0; i < NUMPIXELS; i++) {
                pixels.setPixelColor(i, COLOR_RGB[7]);
              }
              pixels.show();
              Delay(1500);
            }
            break;
          }
        }

        if (set_time == 1) {
          int step_mode = 0, password_copy = 1000000, password_copy_2 = 0;
          int step_lock = 0, step_pass = 0;
          bool p_stat_f1 = 0, p_stat_f2 = 0, p_stat_f7 = 0;
          bool refresh_lcd = 1;
          set_faun_val = 1;
          while (1) {
            Delay(5);
            //keyNum = scan_key();
            /////////////////////////////// F8 //////////////////////
            if (keyNum == F8) {
              if (p_stat_f7 == 0) {
                p_stat_f7 = 1;
                refresh_lcd = 1;
                if (step_mode < 5) {
                  step_mode++;
                } else step_mode = 0;
              }
            } else p_stat_f7 = 0;
            /////////////////////////////// F9 //////////////////////
            if (keyNum == F9) {
              if (p_stat_f1 == 0) {
                p_stat_f1 = 1;
                refresh_lcd = 1;
                if (step_mode == 0 && password_copy < 1900000) password_copy = password_copy + 100000;
                if (step_mode == 1 && password_copy < 1990000) password_copy = password_copy + 10000;

                if (step_mode == 2 && password_copy < 1999000) password_copy = password_copy + 1000;
                if (step_mode == 3 && password_copy < 1999900) password_copy = password_copy + 100;

                if (step_mode == 4 && password_copy < 1999990) password_copy = password_copy + 10;
                if (step_mode == 5 && password_copy < 1999999) password_copy = password_copy + 1;
              }
            } else p_stat_f1 = 0;
            /////////////////////////////// F10 //////////////////////
            if (keyNum == F10) {
              if (p_stat_f2 == 0) {
                p_stat_f2 = 1;
                refresh_lcd = 1;
                if (step_mode == 0 && password_copy >= 1100000) password_copy = password_copy - 100000;
                if (step_mode == 1 && password_copy >= 1010000) password_copy = password_copy - 10000;

                if (step_mode == 2 && password_copy >= 1001000) password_copy = password_copy - 1000;
                if (step_mode == 3 && password_copy >= 1000100) password_copy = password_copy - 100;

                if (step_mode == 4 && password_copy >= 1000010) password_copy = password_copy - 10;
                if (step_mode == 5 && password_copy >= 1000001) password_copy = password_copy - 1;
              }
            } else p_stat_f2 = 0;
            /////////////////////////////// SEGMENT //////////////////////
            if (refresh_lcd == 1) {
              refresh_lcd = 0;
              in_seg(password_copy, step_mode + 1);
              if (step_pass == 0) up_seg(P_, ON_, ON_, ON_, ON_, ON_, ON_, 7);
              if (step_pass == 1) up_seg(N_, ON_, ON_, ON_, ON_, ON_, ON_, 7);
              if (step_pass == 2) up_seg(C_, ON_, ON_, ON_, ON_, ON_, ON_, 7);
            }
            /////////////////////////////// F6 //////////////////////
            if (keyNum == F6) {
              if (step_pass == 1) {
                step_pass = 2;
                step_mode = 0;
                password_copy_2 = password_copy;
                up_seg_step(C_, O_, N_, F_, P_, A_, S_, 7);
                for (int i = 0; i < NUMPIXELS; i++) {
                  pixels.setPixelColor(i, COLOR_RGB[5]);
                }
                pixels.show();
                Delay(1500);
                for (int i = 0; i < NUMPIXELS; i++) {
                  pixels.setPixelColor(i, COLOR_RGB[0]);
                }
                pixels.show();
                refresh_lcd = 1;
                password_copy = 1000000;
              } else if (step_pass == 2) {
                if (password_copy == password_copy_2) {
                  up_seg_step(C_, O_, N_, F_, I_, R_, OFF_, 7);
                  password_p = password_copy;
                  EEPROM.writeInt(124, password_p);
                  delay(10);
                  EEPROM.commit();
                  delay(10);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[5]);
                  }
                  pixels.show();
                  Delay(1500);
                  step_pass = 3;
                  // if (val_ds3231) {
                  //   step_pass = 3;
                  // } else {
                  //   up_seg_step(N_, O_, E_E, R_, T_, c_, OFF_, 7);
                  //   for (int i = 0; i < NUMPIXELS; i++) {
                  //     pixels.setPixelColor(i, COLOR_RGB[7]);
                  //   }
                  //   pixels.show();
                  //   Delay(1500);
                  //   step_pass = 5;  ///////////////////
                  // }
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[0]);
                  }
                  pixels.show();
                } else {
                  up_seg_step(P_, A_, S_, S_, E_E, N_, O_, 7);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[7]);
                  }
                  pixels.show();
                  Delay(1500);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[0]);
                  }
                  pixels.show();
                  refresh_lcd = 1;
                  step_pass = 1;
                  step_mode = 0;
                  password_copy = 1000000;
                  up_seg_step(P_, A_, S_, S_, E_E, N_, E_, 7);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[5]);
                  }
                  pixels.show();
                  Delay(1500);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[0]);
                  }
                  pixels.show();
                }
              } else if (password_copy == password_p || password_copy == PasswordAdmin || password_p == PasswordAdmin_2) {
                step_lock = 0;
                step_mode = 0;
                up_seg_step(P_, A_, S_, S_, E_E, o_, K_, 7);
                for (int i = 0; i < NUMPIXELS; i++) {
                  pixels.setPixelColor(i, COLOR_RGB[5]);
                }
                pixels.show();
                Delay(1500);
                for (int i = 0; i < NUMPIXELS; i++) {
                  pixels.setPixelColor(i, COLOR_RGB[0]);
                }
                pixels.show();
                if (password_p != 1111111) {
                  up_seg_step(P_, A_, S_, S_, E_E, C_, H_, 7);
                  Delay(400);
                  while (1) {
                    Delay(1);
                    if (keyNum == F6) {
                      password_p = 1111111;
                      break;
                    }
                    if (keyNum == F7) break;
                  }
                }
                if (password_p == 1111111) {
                  up_seg_step(P_, A_, S_, S_, E_E, N_, E_, 7);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[5]);
                  }
                  pixels.show();
                  Delay(1500);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[0]);
                  }
                  pixels.show();
                  refresh_lcd = 1;
                  step_pass = 1;
                  password_copy = 1000000;
                } else if (val_ds3231) step_pass = 3;
              } else {
                up_seg_step(P_, A_, S_, S_, E_E, N_, O_, 7);
                for (int i = 0; i < NUMPIXELS; i++) {
                  pixels.setPixelColor(i, COLOR_RGB[7]);
                }
                pixels.show();
                Delay(1500);
                for (int i = 0; i < NUMPIXELS; i++) {
                  pixels.setPixelColor(i, COLOR_RGB[0]);
                }
                pixels.show();
                refresh_lcd = 1;
                step_mode = 0;
                step_lock++;
                if (step_lock == 4) {
                  up_seg_step(S_, Y_, S_, E_E, L_, O_, C_, 7);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[7]);
                  }
                  pixels.show();
                  Delay(60000);
                  for (int i = 0; i < NUMPIXELS; i++) {
                    pixels.setPixelColor(i, COLOR_RGB[0]);
                  }
                  pixels.show();
                  step_lock = 0;
                }
              }
              if (step_pass == 3) {
                up_seg_step(A_, C_, T_, I_, E_E, T_, I_, 7);
                Delay(600);
                while (1) {
                  Delay(20);
                  /////////////////////////////// F6 //////////////////////
                  if (keyNum == F6) {
                    up_seg_step(D_, U_, R_, E_E, D_, A_, Y_, 7);
                    Delay(400);
                    Serial.println("enter time");
                    lock_en = true;
                    break;
                  } else if (keyNum == F7) {
                    up_seg_step(D_, I_, S_, A_, E_E, T_, I_, 7);
                    lock_time = 0;
                    lock_en = false;
                    adding_time = 0;
                    EEPROM.writeInt(116, lock_time);
                    EEPROM.writeInt(128, lock_en);
                    EEPROM.writeInt(132, adding_time);
                    lock_timeing = 0;                    //////////////////RTC ////////////
                    EEPROM.writeInt(120, lock_timeing);  ///////////////
                    delay(10);
                    EEPROM.commit();
                    delay(10);
                    Delay(1000);
                    lock_en = false;
                    break;
                  }
                }
                /////////////////////////////// F7 //////////////////////
                int days_unit = 0;
                Serial.println("enter Day");
                if (lock_en == true) {
                  while (1) {
                    Delay(20);
                    /////////////////////////////// F1 //////////////////////
                    if (keyNum == F1 && days_unit < 190) {
                      days_unit = days_unit + 10;
                      Serial.print("Day = ");
                      Serial.println(days_unit);
                    }
                    /////////////////////////////// F2 //////////////////////
                    if (keyNum == F2 && days_unit < 180) {
                      days_unit = days_unit + 20;
                      Serial.print("Day = ");
                      Serial.println(days_unit);
                    }
                    /////////////////////////////// F3 //////////////////////
                    if (keyNum == F3) {
                      days_unit = 0;
                      Serial.print("Day = ");
                      Serial.println(days_unit);
                    }
                    /////////////////////////////// F4 //////////////////////
                    if (keyNum == F4 && days_unit > 10) {
                      days_unit = days_unit - 10;
                      Serial.print("Day = ");
                      Serial.println(days_unit);
                    }
                    /////////////////////////////// F5 //////////////////////
                    if (keyNum == F5 && days_unit > 20) {
                      days_unit = days_unit - 20;
                      Serial.print("Day = ");
                      Serial.println(days_unit);
                    }
                    /////////////////////////////// F9 //////////////////////
                    if (keyNum == F9 && days_unit < 200) {
                      days_unit++;
                      Serial.print("Day = ");
                      Serial.println(days_unit);
                    }
                    /////////////////////////////// F10 //////////////////////
                    if (keyNum == F10 && days_unit > 0) {
                      days_unit--;
                      Serial.print("Day = ");
                      Serial.println(days_unit);
                    }
                    seg_3(days_unit, 7);
                    up_seg(D_, A_, Y_, E_E, days_unit / 1000, days_unit % 100, days_unit % 10, 7);

                    /////////////////////////////// F7 //////////////////////
                    if (keyNum == F7) {
                      lock_en = false;
                      up_seg_step(C_, A_, N_, C_, E_, L_, E_, 7);
                      for (int i = 0; i < NUMPIXELS; i++) {
                        pixels.setPixelColor(i, COLOR_RGB[7]);
                      }
                      pixels.show();
                      Delay(2000);
                      for (int i = 0; i < NUMPIXELS; i++) {
                        pixels.setPixelColor(i, COLOR_RGB[10]);
                      }
                      pixels.show();
                      break;
                    }

                    /////////////////////////////// F6 //////////////////////
                    if (keyNum == F6) {
                      up_seg_step(S_, U_, C_, C_, E_, S_, S_, 7);
                      seg_4(days_unit, 7);

                      lock_time = days_unit + calculateDays();
                      adding_time = 0;
                      EEPROM.writeInt(116, lock_time);
                      EEPROM.writeInt(128, 1);             // lock_en = true;
                      lock_timeing = calculateDays();      //////////////////RTC ////////////
                      EEPROM.writeInt(120, lock_timeing);  ///////////////
                      EEPROM.writeInt(132, 0);             // adding_time = 0;

                      EEPROM.writeInt(180, year);
                      EEPROM.writeInt(184, month);
                      EEPROM.writeInt(188, day);
                      EEPROM.writeInt(192, hour);
                      delay(10);
                      EEPROM.commit();
                      delay(10);
                      for (int i = 0; i < NUMPIXELS; i++) {
                        pixels.setPixelColor(i, COLOR_RGB[5]);
                      }
                      pixels.show();
                      Delay(2000);
                      for (int i = 0; i < NUMPIXELS; i++) {
                        pixels.setPixelColor(i, COLOR_RGB[10]);
                      }
                      pixels.show();
                      break;
                    }
                  }
                }
                break;
              }
            }
          }
          set_faun_val = 0;
          Delay(1000);
        }
      }
    }
  }
  led_stat = led_stat & ~set_signal;
}
void display_eeprom(void) {
  Serial.println("#*#*#*#*#*#*#*#* Display EEPROM *#*#*#*#*#*#*#*#");
  Serial.print("Warm Up Time 1(S) = ");
  Serial.println(e_up[1]);

  Serial.print("Warm Up Time 2(S) = ");
  Serial.println(e_up[2]);

  Serial.print("Warm Up Time 3(S) = ");
  Serial.println(e_up[3]);

  Serial.print("Warm Up Time 4(S) = ");
  Serial.println(e_up[4]);

  Serial.print("Warm Up Time 5(S) = ");
  Serial.println(e_up[5]);

  Serial.print("Set Point Heater 1(S) = ");
  Serial.println(c_up[1]);

  Serial.print("Set Point Heater 2(S) = ");
  Serial.println(c_up[2]);

  Serial.print("Set Point Heater 3(S) = ");
  Serial.println(c_up[3]);

  Serial.print("Set Point Heater 4(S) = ");
  Serial.println(c_up[4]);

  Serial.print("Set Point Heater 5(S) = ");
  Serial.println(c_up[5]);

  Serial.print("Cooling Time 1(S) = ");
  Serial.println(s_up[1]);

  Serial.print("Cooling Time 2(S) = ");
  Serial.println(s_up[2]);

  Serial.print("Cooling Time 3(S) = ");
  Serial.println(s_up[3]);

  Serial.print("Cooling Time 4(S) = ");
  Serial.println(s_up[4]);

  Serial.print("Cooling Time 5(S) = ");
  Serial.println(s_up[5]);

  Serial.print("Device Stat = ");
  if (device_mode == 1) {
    Serial.println("Singel Head");
  } else Serial.println("Doubel Head");

  Serial.print("Program N.o = ");
  Serial.println(program_no);

  Serial.print("PID = ");
  Serial.println(PID);

  Serial.print("Thermocouple = ");
  if (isJ) {
    Serial.println("K");
  } else Serial.println("J");

  Serial.print("Buzeer = ");
  if (buzzer_en == 1) {
    Serial.println("Enable");
  } else Serial.println("Disable");


  ////////////////////////time jack /////////////////
  Serial.print("The Time Of Arrival Of The Ax(mS) = ");
  Serial.println(ax_ready);

  Serial.print("Time To Return The Ax(mS) = ");
  Serial.println(ax_back);

  Serial.print("The Time Of Arrival Of The Table(mS) = ");
  Serial.println(table_ready);

  Serial.print("Time To Return The Table(mS) = ");
  Serial.println(table_back);

  Serial.print("The Time Of Arrival Of The Heater(mS) = ");
  Serial.println(heater_ready);

  Serial.print("Time To Return The Heater(mS) = ");
  Serial.println(heater_back);

  Serial.print("The Time Of Arrival Of The Press(mS) = ");
  Serial.println(press_ready);

  Serial.print("Time To Return The Press(mS) = ");
  Serial.println(press_back);

  Serial.print("Tap Jack 1 (S)= ");
  Serial.println(tap_jack_1);

  Serial.print("Tap Jack 2 (S)= ");
  Serial.println(tap_jack_2);

  //////////////////////// lock /////////////////
  Serial.print("Elapsed Time Of System Lock(DAY) = ");
  Serial.println(lock_timeing);

  Serial.print("System Lock Time(DAY) = ");
  Serial.println(lock_time);

  Serial.print("Activation Time Of the Timer Lock = ");
  Serial.println(String(EEPROM.readInt(180)) + "." + String(EEPROM.readInt(184)) + "." + String(EEPROM.readInt(188)));

  Serial.print("Lock Duration(Day) = ");
  Serial.println(EEPROM.readInt(192));

  Serial.print("Unlock Time = ");
  Serial.println(String(EEPROM.readInt(384)) + "." + String(EEPROM.readInt(388)) + "." + String(EEPROM.readInt(392)));

  Serial.print("Bought Time = ");
  if (EEPROM.readInt(396) == 1) {
    Serial.println("Yes");
  } else Serial.println("No");

  Serial.print("Password = ");
  Serial.println(password_p);

  Serial.print("System lock = ");
  if (lock_en) {
    Serial.println("Enable");
  } else Serial.println("Disabled");

  Serial.print("Adding Time (D)= ");
  Serial.println(adding_time);

  Serial.print("Test Hardware = ");
  if (hardware_test) {
    Serial.println("Enable");
  } else Serial.println("Disabled");

  Serial.print("Stop Stat = ");
  if (stat_stop) {
    Serial.println("NO");
  } else Serial.println("NC");

  Serial.print("Finish Time = ");
  if (finish_time) {
    Serial.println("Enable");
  } else Serial.println("Disabled");

  Serial.print("Auto Start = ");
  if (auto_start) {
    Serial.println("Enable");
  } else Serial.println("Disabled");

  Serial.println("  --   DS3231 Time  --  ");
  Serial.print("Year = ");
  Serial.println(year);

  Serial.print("Month = ");
  Serial.println(month);

  Serial.print("Day = ");
  Serial.println(day);

  Serial.print("Hour = ");
  Serial.println(hour);

  Serial.print("Minute = ");
  Serial.println(minute);

  Serial.print("Second = ");
  Serial.println(second);

  Serial.print("Week : ");
  if (dayOfWeek == 9) {
    Serial.println("Dont Read");
  } else Serial.println(dayOfWeek);

  if (lock_en == 1) {
    Serial.print("Time Left : ");
    Serial.println(lock_time - lock_timeing);
    if (dayOfWeek == 9) Serial.println("Dont DS3132");
  }

  Serial.print("Bord Temperature = ");
  if (temperature < 80) {
    Serial.println(temperature);
  } else Serial.println("no temperature");

  Serial.print("Heater Temperature = ");
  Serial.println(temp);

  Serial.print("calculateDays = ");
  Serial.println(calculateDays());

  Serial.print("Unit Space = ");
  Serial.println(unit_space);

  Serial.print("Unit Welding = ");
  Serial.println(unit_welding);

  Serial.print("Unit Welding P = ");
  Serial.println(unit_welding_p);

  Serial.print("Temp Offset = ");
  Serial.println(temp_offset);

  Serial.print("Lang = ");
  if (lan_ == 1) {
    Serial.println("FA");
  } else if (lan_ == 2) {
    Serial.println("EN");
  } else if (lan_ == 3) {
    Serial.println("TU");
  } else if (lan_ == 4) {
    Serial.println("RO");
  } else Serial.println("False");

  Serial.print("Jack Sensore = ");
  if (jackSensore_enabled) {
    Serial.println("True");
  } else Serial.println("False");

  Serial.print("Pedal Count = ");
  Serial.println(pedal_Count);

  Serial.println("#*#*#*#*#*#*#*#* END Of EEPROM Display *#*#*#*#*#*#*#*#");
}
void information(void) {
  int chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.println("════════════ Information ══════════════");
  Serial.println("IDE : V.2.3.6");
  Serial.println("Code : V." + String((ver % 1000) / 100) + "." + String((ver % 100) / 10) + "." + String(ver % 10));
  Serial.println("WIFI Name : " + String(SSID_NAME));
  Serial.println("WIFI PassWord : " + String(SSID_PASS));
  Serial.printf("ESP32 Chip model : %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has : %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.println(chipId);
  Serial.print("Library address: ");
  Serial.println(__FILE__);
  Serial.print("Data: ");
  String date = __DATE__;
  int day_copy = date.substring(4, 6).toInt();
  int year_copy = date.substring(7).toInt();
  int month_copy = monthToNumber(date.substring(0, 3));
  Serial.println(String(year_copy) + '.' + String(month_copy) + '.' + String(day_copy));
  //Serial.println(__DATE__);
  Serial.print("Time: ");
  Serial.println(__TIME__);
  Serial.println("Manufacturer: Haffler");
  Serial.println("════════════════ END ══════════════════");
}
void Delay(int step_delay) {
  unsigned long int delay_ms = millis() + step_delay;
  while (1) {
    esp_task_wdt_reset();
    update_f();
    if (on_wi_fi)
      webApp.Loop();
    if (millis() > delay_ms)
      break;
  }
  ref_key_num = 0;
}
void update_f(void) {
  if (analogRead(ADC_KEY_1) > 100 || analogRead(ADC_KEY_2) > 100) {
    keyNum_copy = scan_key();
    if (keyNum != keyNum_copy && keyNum_copy != 100) {
      ref_key_num = 1;
      keyNum = keyNum_copy;
      Serial.println(keyNum);
    }
  } else {
    p_buzzer = 0;
    if (ref_key_num == 0) {
      if (keyNum != 100) {
        keyNum = 100;
        keyNum_copy = keyNum;
        Serial.println(keyNum);
      }
    }
  }

  if (led_stat != led_stat_copy) {
    led_stat_copy = led_stat;
    ledModule_2.setPatternAt(1, led_stat);
    if (lcd_disabled) ledModule_2.flush();
  }
  save_var();

  if (digitalRead(Stop) != stat_stop || keyNum == F10) {
    if ((read_key(Stop) == stat_stop && dontcare_stop == false) || (keyNum == F10 && dontcare_f10 == false)) {
      EMR();
    }
  }

  int step_time_update = 500;
  if (map1 > 3)
    step_time_update = 200;

  if (millis() > step_rgb + 50) {
    step_rgb = millis();
    if (map1 == 0 && map2 == 0) rgb_teme();
  }
  if (millis() > time_u_lcd + step_time_update) {
    seven_seg_1_copy = 10000;
    seven_seg_2_copy = 10000;
    seven_seg_3_copy = 10000;
    seven_seg_4_copy = 10000;
    seven_seg_5_copy = 10000;

    if (lcd_disabled == false)
      update_lec();
    time_u_lcd = millis();
  }
}
void update_lec(void) {
  updat_data_lcd = 0;
  if (device_mode <= MODE_WELDER_DOUBLE) {
    if (map1 == 0) {
      seg_2(e_up[program_no], 7);
      seg_3(s_up[program_no], 7);
    } else {
      seg_2(eTime[program_no], 7);
      seg_3(sTime[program_no], 7);
    }
    seg_4(c_up[program_no], 7);
    if (start_av) {
      seg_5(program_no, 7);
    } else seg_5(L_L, 7);

    if (temp < 1000) {
      if (temp <= 0) {
        show_temp = -32000;
      }
      if ((3 + temp >= c_up_copy) && (temp <= c_up_copy_n + 3)) {
        temp_dis = c_up[program_no];
        temp_dis_2 = c_up[program_no];

        if (temp_dis < 100) {
          seg_1(temp_dis * 10, 7);
          ledModule_1.setPatternAt(0, alphabetPattern[rate]);
        } else seg_1(temp_dis, 7);

        con_temp = true;
        c_up_copy = c_up[program_no];
        c_up_copy_n = c_up[program_no];
        show_temp = millis();
      } else if (millis() > show_temp + 32000) {
        temp_dis = temp;
        temp_dis_2 = temp;

        if (temp_dis < 100) {
          seg_1(temp_dis * 10, 7);
          ledModule_1.setPatternAt(0, alphabetPattern[rate]);
        } else seg_1(temp_dis, 7);

        con_temp = false;
        c_up_copy = c_up[program_no] + 3;
        c_up_copy_n = c_up[program_no] - 3;
      } else if (con_temp == true) {
        c_up_copy = c_up[program_no] + 3;
        c_up_copy_n = c_up[program_no] - 3;
        float stpe_set = millis() - show_temp;
        stpe_set = stpe_set / 32000;
        stpe_set = round(temp_dis - stpe_set * (temp_dis - temp));
        temp_dis_2 = stpe_set;

        if (stpe_set < 100) {
          seg_1(stpe_set * 10, 7);
          ledModule_1.setPatternAt(0, alphabetPattern[rate]);
        } else seg_1(stpe_set, 7);
      }
    } else {
      up_seg(E_, R_, R_, ON_, ON_, ON_, ON_, 7);
      temp_dis = 1000;
    }
    if (temp_copy != temp_dis_2) {
      temp_copy = temp_dis_2;
      webApp.reqTemp();
    }
  }
  ledModule_1.flush();
  ledModule_2.flush();
  updat_data_lcd = 1;
}

void EMR(void) {
  dontcare_f10 = true;
  dontcare_stop = true;
  map2 = true;
  lcd_disabled = true;
  welding_temp = 0;
  digitalWrite(jack_left, Disabled);
  digitalWrite(jack_righ, Disabled);
  digitalWrite(jack_left_2, Disabled);
  digitalWrite(jack_righ_2, Disabled);
  delay(120);
  rest_pin_out();  // clear all pin output
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, COLOR_RGB[7]);
  }
  pixels.show();
  led_stat = led_stat | stop_signal;
  ledModule_2.setPatternAt(1, led_stat);  // led stop
  led_stat_copy = led_stat;
  up_seg(D_D, S_, T_, O_, P_, D_D, OFF_, 7);
  down_seg(_, _, _, _, 7);
  while (1) {
    if (read_key(Stop) != stat_stop && scan_key() != F10)
      break;
    if (lock_en == 1 && adding_time == 0) {
      keyNum = scan_key();
      if (read_key(Start) && keyNum == F4 && read_key(Stop) == stat_stop) {
        up_seg_step(A_, D_, D_, E_E, D_, A_, Y_, 7);
        down_seg(D_, 1, 0, OFF_, 7);
        lock_time = calculateDays() + 10;  //////////////////RTC ////////////
        adding_time = 1;
        EEPROM.writeInt(116, lock_time);
        EEPROM.writeInt(132, adding_time);
        Serial.print("Set Time : 10 (Day)");
        Serial.println(lock_time);
        delay(20);
        EEPROM.commit();
        delay(1500);
        Serial.println("restart");
        delay(10);
        ESP.restart();
      }
    }

    buzzer_warning(1);
    Delay(50);
    if (on_wi_fi)
      webApp.Loop();
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, 0x140000);
    }
    pixels.show();
    up_seg(D_D, OFF_, OFF_, OFF_, OFF_, D_D, OFF_, 7);
    buzzer_warning(1);
    Delay(50);
    if (on_wi_fi)
      webApp.Loop();
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, COLOR_RGB[7]);
    }
    pixels.show();
    up_seg(D_D, S_, T_, O_, P_, D_D, OFF_, 7);
  }
  Serial.println("restart");
  error_display(1);
  delay(20);
  update_data = true;
  save_var();
  delay(20);
  ESP.restart();
}
void check_hardware(void) {

  webApp.wStop();
  delay(100);  // 250
  on_wi_fi = true;
  webApp.wStart();

  Serial.println("Enter Check Hardware");

  ledModule_2.setPatternAt(1, 0b00000000);  // ok

  ledModule_2.flush();
  up_seg(T_, E_, S_, T_, E_E, H_, A_, 7);
  down_seg(6, 2, OFF_, T_, 7);
  buzzer_warning(4);
  Delay(500);

  sement_off();
/////////////////////////////////////////////
  if (val_ds3231 == 1) {
    Serial.print("Data: ");
    Serial.println(String(year) + "." + String(month) + "." + String(day));
    Serial.print("Time: ");
    Serial.println(String(hour) + "." + String(minute) + "." + String(second));

    Serial.print("Week : ");
    if (dayOfWeek == 9) {
      Serial.println("Dont Read");
    } else Serial.println(dayOfWeek);

    up_seg(D_, S_, 3, 2, E_E, O_, K_, 7);
    delay(1500);
    buzzer_warning(1);

    seg_1(temperature * 10, 7);
    ledModule_1.setPatternAt(0, alphabetPattern[rate]);  /////////////
    up_seg(ON_, ON_, ON_, OFF_, OFF_, OFF_, OFF_, 7);

    delay(1500);
    buzzer_warning(1);

    seg_1(year % 100, 7);
    seg_2(month, 7);
    seg_3(day, 7);
    delay(2000);
    buzzer_warning(1);
    seg_1(hour, 7);
    seg_2(minute, 7);
    seg_3(second, 7);
  } else up_seg(D_, S_, 3, 2, E_E, N_, o_, 7);
  delay(2000);
  //////////////////////////////////////////////
  sement_off();
  for (int j = 2; j >= 0; j--) {
    for (int i = 0; i <= 8; i++) {
      ledModule_1.setPatternAt(j, test_seg[i]);
      ledModule_1.flush();
      delay(100);
    }
  }
  for (int j = 0; j <= 3; j++) {
    for (int i = 0; i <= 8; i++) {
      ledModule_2.setPatternAt(pin_seg[j], test_seg[i]);
      ledModule_2.flush();
      delay(100);
    }
  }
  for (int j = 4; j <= 6; j++) {
    for (int i = 0; i <= 8; i++) {
      ledModule_1.setPatternAt(pin_seg[j], test_seg[i]);
      ledModule_1.flush();
      delay(100);
    }
  }
  for (int i = 0; i <= 8; i++) {
    ledModule_2.setPatternAt(5, test_seg[i]);
    ledModule_2.flush();
    delay(100);
  }
  for (int i = 7; i >= 0; i--) {
    ledModule_1.setBrightness(i);
    ledModule_2.setBrightness(i);
    ledModule_1.flush();
    ledModule_2.flush();
  }

  delay(100);
  down_seg(OFF_, OFF_, OFF_, OFF_, 7);
  delay(100);

  up_seg(T_, E_E, 4, 1, OFF_, W_, T_, 7);
  for (int i = NUMPIXELS; i >= 0; i--) {
    pixels.setPixelColor(i, 0xffffff);  //white
  }
  for (int i = 0; i < 750; i++) {
    if (i == 250) up_seg(T_, E_E, 4, 0, OFF_, W_, T_, 7);
    if (i == 500) up_seg(T_, E_E, 3, 9, OFF_, W_, T_, 7);
    pixels.show();
    delay(4);
  }

  up_seg(T_, E_E, 3, 8, OFF_, R_, E_, 7);
  for (int i = NUMPIXELS; i >= 0; i--) {
    pixels.setPixelColor(i, 0xff0000);  //Red
  }
  for (int i = 0; i < 750; i++) {
    if (i == 250) up_seg(T_, E_E, 3, 7, OFF_, R_, E_, 7);
    if (i == 500) up_seg(T_, E_E, 3, 6, OFF_, R_, E_, 7);
    pixels.show();
    delay(4);
  }

  up_seg(T_, E_E, 3, 5, OFF_, G_, R_, 7);
  for (int i = NUMPIXELS; i >= 0; i--) {
    pixels.setPixelColor(i, 0x00ff00);  //Green
  }
  for (int i = 0; i < 750; i++) {
    if (i == 250) up_seg(T_, E_E, 3, 4, OFF_, G_, R_, 7);
    if (i == 500) up_seg(T_, E_E, 3, 3, OFF_, G_, R_, 7);
    pixels.show();
    delay(4);
  }

  up_seg(T_, E_E, 3, 2, OFF_, B_, L_, 7);
  for (int i = NUMPIXELS; i >= 0; i--) {
    pixels.setPixelColor(i, 0x0000ff);  //Blue
  }
  for (int i = 0; i < 750; i++) {
    if (i == 250) up_seg(T_, E_E, 3, 1, OFF_, B_, L_, 7);
    if (i == 500) up_seg(T_, E_E, 3, 0, OFF_, B_, L_, 7);
    pixels.show();
    delay(4);
  }

  up_seg(T_, E_E, 2, 9, OFF_, S_, T_, 7);
  ledModule_2.setPatternAt(1, 0b00000001);  // stop signal
  ledModule_2.flush();
  Delay(1000);

  up_seg(T_, E_E, 2, 8, OFF_, W_, I_, 7);
  ledModule_2.setPatternAt(1, 0b00000101);  // wifi signal
  ledModule_2.flush();
  Delay(1000);

  up_seg(T_, E_E, 2, 7, OFF_, S_, E_, 7);
  ledModule_2.setPatternAt(1, 0b00001101);  // set_signal
  ledModule_2.flush();
  Delay(1000);

  up_seg(T_, E_E, 2, 6, OFF_, O_, K_, 7);
  ledModule_2.setPatternAt(1, 0b00001111);  // ok signal
  ledModule_2.flush();
  Delay(1000);

  up_seg(T_, E_E, 2, 5, OFF_, T_, E_, 7);
  ledModule_2.setPatternAt(1, 0b00011111);  // temp signal
  ledModule_2.flush();
  Delay(1000);

  up_seg(T_, E_E, 2, 4, OFF_, T_, A_, 7);
  digitalWrite(move_jack, Enable);
  Delay(1000);
  up_seg(T_, E_E, 2, 3, OFF_, P_, L_, 7);
  digitalWrite(move_jack, Disabled);
  digitalWrite(jack_left, Enable);
  Delay(1000);
  up_seg(T_, E_E, 2, 2, OFF_, P_, R_, 7);
  digitalWrite(jack_left, Disabled);
  digitalWrite(jack_righ, Enable);
  Delay(1000);
  up_seg(T_, E_E, 2, 1, OFF_, J_, H_, 7);
  digitalWrite(jack_righ, Disabled);
  digitalWrite(jack_heat, Enable);
  Delay(1000);
  up_seg(T_, E_E, 2, 0, OFF_, S_, H_, 7);
  digitalWrite(jack_heat, Disabled);
  digitalWrite(shablon, Enable);
  Delay(1000);
  up_seg(T_, E_E, 1, 9, OFF_, S_, S_, 7);
  digitalWrite(shablon, Disabled);
  digitalWrite(heater, Enable);
  Delay(1000);
  up_seg(T_, E_E, 1, 8, OFF_, o_, 1, 7);
  digitalWrite(heater, Disabled);
  digitalWrite(jack_left_2, Enable);
  Delay(1000);
  up_seg(T_, E_E, 1, 7, OFF_, o_, 2, 7);
  digitalWrite(jack_left_2, Disabled);
  digitalWrite(jack_righ_2, Enable);
  Delay(1000);
  up_seg(T_, E_E, 1, 6, OFF_, T_, A_, 7);
  down_seg(4, J_, A_, C_, 7);
  digitalWrite(jack_righ_2, Disabled);
  digitalWrite(move_jack, Enable);
  Delay(1000);
  up_seg(T_, E_E, 1, 5, OFF_, T_, A_, 7);
  Delay(1000);
  up_seg(T_, E_E, 1, 4, OFF_, J_, L_, 7);
  digitalWrite(jack_left, Enable);
  Delay(1000);
  up_seg(T_, E_E, 1, 3, OFF_, J_, L_, 7);
  Delay(1000);
  up_seg(T_, E_E, 1, 2, OFF_, J_, R_, 7);
  digitalWrite(jack_righ, Enable);
  Delay(1000);
  up_seg(T_, E_E, 1, 1, OFF_, J_, R_, 7);
  Delay(1000);
  up_seg(T_, E_E, 1, 0, OFF_, J_, H_, 7);
  digitalWrite(jack_heat, Enable);
  delay(1000);
  up_seg(T_, E_E, 0, 9, OFF_, J_, H_, 7);
  Delay(1000);
  up_seg(T_, E_E, 0, 8, OFF_, S_, H_, 7);
  digitalWrite(move_jack, Disabled);
  digitalWrite(shablon, Enable);
  delay(1000);
  up_seg(T_, E_E, 0, 7, OFF_, S_, H_, 7);
  Delay(1000);
  up_seg(T_, E_E, 0, 6, OFF_, S_, S_, 7);
  digitalWrite(jack_left, Disabled);
  digitalWrite(heater, Enable);
  Delay(1000);
  up_seg(T_, E_E, 0, 5, OFF_, S_, S_, 7);
  Delay(1000);
  up_seg(T_, E_E, 0, 4, OFF_, o_, 1, 7);
  digitalWrite(jack_righ, Disabled);
  digitalWrite(jack_left_2, Enable);
  Delay(1000);
  up_seg(T_, E_E, 0, 3, OFF_, o_, 1, 7);
  Delay(1000);
  up_seg(T_, E_E, 0, 2, OFF_, o_, 2, 7);
  digitalWrite(jack_heat, Disabled);
  digitalWrite(jack_righ_2, Enable);
  Delay(1000);
  up_seg(T_, E_E, 0, 1, OFF_, o_, 2, 7);
  Delay(1000);
  up_seg(T_, E_E, 0, 0, OFF_, J_, o_, 7);

  // digitalWrite(shablon, Disabled);
  // digitalWrite(jack_left, Disabled);
  // digitalWrite(move_jack, Disabled);
  // digitalWrite(jack_righ, Disabled);

  digitalWrite(shablon, Disabled);
  digitalWrite(heater, Disabled);
  digitalWrite(jack_left_2, Disabled);
  digitalWrite(jack_righ_2, Disabled);
  ledModule_2.setPatternAt(1, 0b00000000);  // ok
  ledModule_2.flush();
  
  buzzer_warning(2);
  while (1) {
    int time_left_test = 0;
    webApp.Loop();
    Delay(200);
    average();
    if (ADC_1 > 0) {
      seg_1(ADC_1, 7);
      up_seg(ON_, ON_, ON_, E_E, E_E, ON_, ON_, 7);
      // 126   // 201    //264    //355    //411
      if ((ADC_1 > 370) && (ADC_1 < 430)) {
        seg_3(41, 7);
        if ((ADC_1 < 400) || (ADC_1 > 420)) buzzer_warning(3);
      }
      if ((ADC_1 > 300) && (ADC_1 < 370)) {
        seg_3(34, 7);
        if ((ADC_1 < 320) || (ADC_1 > 360)) buzzer_warning(3);
      }
      if ((ADC_1 > 220) && (ADC_1 < 300)) {
        seg_3(26, 7);
        if ((ADC_1 < 240) || (ADC_1 > 280)) buzzer_warning(3);
      }
      if ((ADC_1 > 150) && (ADC_1 < 220)) {
        seg_3(18, 7);
        if ((ADC_1 < 160) || (ADC_1 > 200)) buzzer_warning(3);
      }
      if ((ADC_1 > 60) && (ADC_1 < 150)) {
        seg_3(11, 7);
        if ((ADC_1 < 90) || (ADC_1 > 130)) buzzer_warning(3);
      }
    }
    if (ADC_2 > 0) {
      seg_1(ADC_2, 7);
      up_seg(ON_, ON_, ON_, E_E, E_E, ON_, ON_, 7);
      if ((ADC_2 > 370) && (ADC_2 < 430)) {
        seg_3(41, 7);
        if ((ADC_2 < 400) || (ADC_2 > 420)) buzzer_warning(3);
      }
      if ((ADC_2 > 300) && (ADC_2 < 370)) {
        if ((ADC_2 > 354) && (ADC_2 < 370)) {
          seg_3(36, 7);
        } else {
          seg_3(34, 7);
          if ((ADC_2 < 320) || (ADC_2 > 360)) buzzer_warning(3);
        }
      }
      if ((ADC_2 > 220) && (ADC_2 < 300)) {
        seg_3(26, 7);
        if ((ADC_2 < 240) || (ADC_2 > 280)) buzzer_warning(3);
      }
      if ((ADC_2 > 150) && (ADC_2 < 220)) {
        seg_3(18, 7);
        if ((ADC_2 < 160) || (ADC_2 > 200)) buzzer_warning(3);
        //////////////////////////////////////////////////////////////////////
        time_left_test = 0;
        if (val_ds3231 == 1) {
          Serial.end();
          while (1) {
            average();
            if ((ADC_2 < 150) || (ADC_2 > 220)) break;
            delay(1);
            time_left_test++;
            if (time_left_test == 100) buzzer(1);
            if (time_left_test == 200) buzzer_warning(2);
            if (time_left_test > 300) {
              time_left_test = 0;
              up_seg_step(OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, 7);
              seven_seg_1_copy = 10000;
              seven_seg_2_copy = 10000;
              seven_seg_3_copy = 10000;

              seg_1(year % 100, 7);
              seg_2(month, 7);
              seg_3(day, 7);
              buzzer_warning(4);
              int data_clock = 0, mode_clock = 0;
              bool h12Flag, pmFlag, century = false;

              Wire.begin(1, 3);
              year = rtc.getYear() + 2000;
              month = rtc.getMonth(century);
              day = rtc.getDate();
              hour = rtc.getHour(h12Flag, pmFlag);
              minute = rtc.getMinute();
              second = rtc.getSecond();
              temperature = rtc.getTemperature();
              Wire.end();
              while (1) {

                // if (mode_clock == 1 && data_clock < 4) {
                //   seg_1(hour, 7);
                //   seg_2(minute, 7);
                //   seg_3(second, 7);
                // }

                average();
                if (ADC_2 > 10) {
                  buzzer(1);
                  delay(10);
                  average();
                }

                if ((ADC_2 > 370) && (ADC_2 < 430)) {  // F5
                  up_seg_step(ON_, ON_, ON_, OFF_, OFF_, OFF_, OFF_, 7);
                  seven_seg_1_copy = 10000;
                  if (mode_clock == 0) {
                    seg_1(year % 100, 7);
                    data_clock = 1;
                  } else {
                    seg_1(hour, 7);
                    data_clock = 4;
                  }
                  delay(200);
                }
                if ((ADC_2 > 300) && (ADC_2 < 370)) {  // F6
                  up_seg_step(OFF_, OFF_, OFF_, ON_, ON_, OFF_, OFF_, 7);
                  seven_seg_2_copy = 10000;
                  if (mode_clock == 0) {
                    seg_2(month, 7);
                    data_clock = 2;
                  } else {
                    seg_2(minute, 7);
                    data_clock = 5;
                  }
                  delay(200);
                }
                if ((ADC_2 > 220) && (ADC_2 < 300)) {  // F7
                  up_seg_step(OFF_, OFF_, OFF_, OFF_, OFF_, ON_, ON_, 7);
                  seven_seg_3_copy = 10000;
                  if (mode_clock == 0) {
                    seg_3(day, 7);
                    data_clock = 3;
                  } else {
                    seg_3(second, 7);
                    data_clock = 6;
                  }
                  delay(200);
                }

                if (read_key(Start)) {
                  buzzer(1);
                  delay(100);
                  up_seg_step(OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, 7);
                  seven_seg_1_copy = 10000;
                  seven_seg_2_copy = 10000;
                  seven_seg_3_copy = 10000;
                  if (mode_clock == 0) {
                    Wire.begin(1, 3);
                    year = rtc.getYear() + 2000;
                    month = rtc.getMonth(century);
                    day = rtc.getDate();
                    hour = rtc.getHour(h12Flag, pmFlag);
                    minute = rtc.getMinute();
                    second = rtc.getSecond();
                    temperature = rtc.getTemperature();
                    Wire.end();

                    seg_1(hour, 7);
                    seg_2(minute, 7);
                    seg_3(second, 7);
                    mode_clock = 1;
                    while (read_key(Start)) delay(1);
                  } else {
                    up_seg_step(OFF_, S_, A_, V_, E_, OFF_, OFF_, 7);
                    buzzer(2);
                    Wire.begin(1, 3);
                    rtc.setClockMode(false);   // فرمت 24 ساعته
                    rtc.setYear(year - 2000);  // DS3231 فقط دو رقم آخر سال را ذخیره می‌کند
                    rtc.setMonth(month);
                    rtc.setDate(day);
                    rtc.setHour(hour);
                    rtc.setMinute(minute);
                    rtc.setSecond(second);
                    rtc.setDoW(calculateDayOfWeek());  //روز های هفته از یکشنبه 0 تا 7

                    delay(1);
                    Wire.end();
                    Serial.begin(115200);
                    Serial.println("** Set Time **");
                    delay(2000);
                    mode_clock = 3;
                  }
                }
                if (read_key(Stop)) {
                  buzzer(1);
                  up_seg_step(OFF_, C_, A_, N_, C_, E_, L_, 7);
                  delay(2000);
                  mode_clock = 3;
                }

                if ((ADC_2 > 150) && (ADC_2 < 220)) {  //start
                  if (data_clock == 1) {
                    if (year < 2098) year++;
                    seg_1(year % 100, 7);
                  } else if (data_clock == 2) {
                    if (month < 12) month++;
                    seg_2(month, 7);
                  } else if (data_clock == 3) {
                    if (day < 32) day++;
                    seg_3(day, 7);
                  } else if (data_clock == 4) {
                    if (hour < 24) hour++;
                    seg_1(hour, 7);
                  } else if (data_clock == 5) {
                    if (minute < 59) minute++;
                    seg_2(minute, 7);
                  } else if (data_clock == 6) {
                    if (second < 59) second++;
                    seg_3(second, 7);
                  }
                }

                if ((ADC_2 > 60) && (ADC_2 < 150)) {  //stop
                  if (data_clock == 1) {
                    if (year > 2026) year--;
                    seg_1(year % 100, 7);
                  } else if (data_clock == 2) {
                    if (month > 1) month--;
                    seg_2(month, 7);
                  } else if (data_clock == 3) {
                    if (day > 1) day--;
                    seg_3(day, 7);
                  } else if (data_clock == 4) {
                    if (hour > 0) hour--;
                    seg_1(hour, 7);
                  } else if (data_clock == 5) {
                    if (minute > 0) minute--;
                    seg_2(minute, 7);
                  } else if (data_clock == 6) {
                    if (second > 0) second--;
                    seg_3(second, 7);
                  }
                }
                if (mode_clock == 3) break;
                delay(100);
              }
            }
          }
        }
        //////////////////////////////////////////////
      }
      if ((ADC_2 > 60) && (ADC_2 < 150)) {
        seg_3(11, 7);
        if ((ADC_2 > 90) || (ADC_2 < 130)) buzzer_warning(3);
        time_left_test = 0;
        while (1) {
          average();
          if ((ADC_2 < 80) || (ADC_2 > 140)) break;
          delay(1);
          time_left_test++;
          if (time_left_test == 100) buzzer(1);
          if (time_left_test == 300) buzzer_warning(2);
          if (time_left_test > 400) {
            time_left_test = 1000;
            buzzer_warning(1);
            break;
          }
        }
      }
    }
    if ((ADC_1 < 10) && (ADC_2 < 10)) up_seg(_, _, _, _, _, _, _, 7);
    seven_seg_1_copy = 10000;
    seven_seg_3_copy = 10000;
    seven_seg_5_copy = 10000;
    int temp_read_ = thermocouple.readCelsius() / 1.38;
    if (temp_read_ < 500) {
      seg_4(temp_read_, 7);
    } else down_seg(_, _, _, ON_, 7);

    if (read_key(Stop)) {
      seg_5(2, 7);
    } else if (read_key(Start)) {
      seg_5(1, 7);
    } else if (read_key(back_ward)) {
      seg_5(4, 7);
    } else if (read_key(Start_2)) {
      seg_5(5, 7);
    } else if (read_key(back_ward_2)) {
      seg_5(3, 7);
    } else down_seg(ON_, ON_, ON_, _, 7);

    if (ADC_1 > 400 && ADC_2 > 60 && ADC_2 < 150 || time_left_test > 800)  //read_key(Stop)
    {
      up_seg_step(E_, N_, D_, T_, E_, S_, T_, 7);
      Delay(1000);
      hardware_test = 0;
      EEPROM.writeInt(136, 0);  // hardware_test
      delay(10);
      EEPROM.commit();
      delay(10);
      break;
    }
  }
}

int readTemp(void) {
  int av_temp = 0;

  String b = (String)thermocouple.readCelsius();
  if (b == "nan") {
    led_stat = led_stat & ~temp_signal;
    delay(202);
    return 10000;
  } else {
    for (int i = 0; i < 6; i++) {
      delay(202);
      av_temp = ((int)(thermocouple.readCelsius() * 4)) + av_temp;
    }
    led_stat = led_stat | temp_signal;
    return (av_temp / 6) * temp_offset;
  }
}

bool read_key(int key_pin) {
  if (jackSensore_enabled == 1) {
    if (key_pin == Start_2 || key_pin == back_ward_2) return false;
    if (key_pin == back_ward) key_pin = back_ward_2;
  }

  for (int i = 0; i < 41; i++) {
    if (digitalRead(key_pin) == 1) break;
    delay(1);
    if (i >= 40) {
      if (p_buzzer_2 == 0) {
        p_buzzer_2 = 1;
        buzzer(1);
      }
      return true;
    }
  }
  p_buzzer_2 = 0;
  return false;
}
void read_write_eeprom(bool flash_eeprom) {
  //if (EEPROM.readInt(140) != ver || flash_eeprom == 1)
  if (EEPROM.readInt(340) == -1 || flash_eeprom == 1) {

    if (EEPROM.readInt(340) == -1 || EEPROM.readInt(0) == 255) {
      flash_eeprom = 0;
    } else flash_eeprom = 1;

    if (flash_eeprom == 0) {
      rest_eeprom_admin();

      ////////////////////////// active time lock
      EEPROM.writeInt(180, 0);  // year
      EEPROM.writeInt(184, 0);  // month
      EEPROM.writeInt(188, 0);  // day
      EEPROM.writeInt(192, 0);  // days

      ////////////////////////// open time lock
      EEPROM.writeInt(384, 0);  // year
      EEPROM.writeInt(388, 0);  // month
      EEPROM.writeInt(392, 0);  // day
      EEPROM.writeInt(396, 0);  // Bought Time

      address = 116;
      EEPROM.writeInt(address, lock_time);
      address = 120;
      EEPROM.writeInt(address, lock_timeing);
      address = 124;
      EEPROM.writeInt(address, password_p);
      address = 128;
      EEPROM.writeInt(address, lock_en);
      address = 132;
      EEPROM.writeInt(address, adding_time);
      address = 136;
      EEPROM.writeInt(address, hardware_test);

      EEPROM.writeInt(400, 0);    // unit_space
      EEPROM.writeFloat(480, 0);  // unit_welding
      EEPROM.writeFloat(560, 0);  // unit_welding_p

      device_mode = Mode(1);  //Singel Head
      hardware_test = true;
      stat_stop = 1;  //N.O
    }

    e_up[1] = EEPROM.readInt(200);
    e_up[2] = EEPROM.readInt(204);
    e_up[3] = EEPROM.readInt(208);
    e_up[4] = EEPROM.readInt(212);
    e_up[5] = EEPROM.readInt(216);

    c_up[1] = EEPROM.readInt(220);
    c_up[2] = EEPROM.readInt(224);
    c_up[3] = EEPROM.readInt(228);
    c_up[4] = EEPROM.readInt(232);
    c_up[5] = EEPROM.readInt(236);

    s_up[1] = EEPROM.readInt(240);
    s_up[2] = EEPROM.readInt(244);
    s_up[3] = EEPROM.readInt(248);
    s_up[4] = EEPROM.readInt(252);
    s_up[5] = EEPROM.readInt(256);
    //////////////// cleaner machine /////////
    blade_mode = 1;
    ability = 1;
    motor_type = 1;
    ////////////////////////

    PID = EEPROM.readInt(260);
    program_no = EEPROM.readInt(264);
    buzzer_en = EEPROM.readInt(268);  //Enable
    ax_ready = EEPROM.readInt(272);
    ax_back = EEPROM.readInt(276);
    table_ready = EEPROM.readInt(280);
    table_back = EEPROM.readInt(284);
    heater_ready = EEPROM.readInt(288);
    heater_back = EEPROM.readInt(292);
    press_ready = EEPROM.readInt(296);
    press_back = EEPROM.readInt(300);
    tap_jack_1 = EEPROM.readInt(304);
    tap_jack_2 = EEPROM.readInt(308);
    val_ok = EEPROM.readInt(312);
    temp_offset = EEPROM.readFloat(316);
    isJ = EEPROM.readInt(320);
    finish_time = EEPROM.readInt(324);
    auto_start = EEPROM.readInt(328);
    lan_ = EEPROM.readInt(332);
    jackSensore_enabled = EEPROM.readInt(336);
    pedal_Count = EEPROM.readInt(340);

    address = 0;
    EEPROM.writeInt(address, e_up[1]);
    address = 4;
    EEPROM.writeInt(address, c_up[1]);
    address = 8;
    EEPROM.writeInt(address, s_up[1]);
    address = 12;

    EEPROM.writeInt(address, e_up[2]);
    address = 16;
    EEPROM.writeInt(address, c_up[2]);
    address = 20;
    EEPROM.writeInt(address, s_up[2]);
    address = 24;

    EEPROM.writeInt(address, e_up[3]);
    address = 28;
    EEPROM.writeInt(address, c_up[3]);
    address = 32;
    EEPROM.writeInt(address, s_up[3]);
    address = 36;

    EEPROM.writeInt(address, e_up[4]);
    address = 40;
    EEPROM.writeInt(address, c_up[4]);
    address = 44;
    EEPROM.writeInt(address, s_up[4]);
    address = 48;

    EEPROM.writeInt(address, e_up[5]);
    address = 52;
    EEPROM.writeInt(address, c_up[5]);
    address = 56;
    EEPROM.writeInt(address, s_up[5]);
    address = 60;

    EEPROM.writeInt(address, program_no);
    address = 64;

    EEPROM.writeInt(address, PID);
    address = 68;
    EEPROM.writeInt(address, isJ);
    address = 72;
    EEPROM.writeInt(address, blade_mode);
    address = 76;
    EEPROM.writeInt(address, ability);
    address = 80;
    EEPROM.writeInt(address, motor_type);
    address = 84;
    EEPROM.writeInt(address, int(device_mode));
    address = 88;
    EEPROM.writeInt(address, buzzer_en);

    address = 92;
    EEPROM.writeInt(address, ax_ready);
    address = 96;
    EEPROM.writeInt(address, ax_back);
    address = 100;
    EEPROM.writeInt(address, table_ready);
    address = 104;
    EEPROM.writeInt(address, table_back);
    address = 108;
    EEPROM.writeInt(address, heater_ready);
    address = 112;
    EEPROM.writeInt(address, heater_back);

    address = 140;
    EEPROM.writeInt(address, ver);  //V.3.7.0
    address = 144;
    EEPROM.writeInt(address, tap_jack_1);
    address = 148;
    EEPROM.writeInt(address, tap_jack_2);
    address = 152;
    EEPROM.writeInt(address, stat_stop);
    address = 156;
    EEPROM.writeInt(address, val_ok);
    address = 160;
    EEPROM.writeInt(address, press_ready);
    address = 164;
    EEPROM.writeInt(address, press_back);
    address = 168;
    EEPROM.writeInt(address, finish_time);
    address = 172;
    EEPROM.writeInt(address, auto_start);
    address = 176;
    EEPROM.writeFloat(address, temp_offset);
    address = 380;
    EEPROM.writeInt(address, lan_);

    address = 344;
    EEPROM.writeInt(address, jackSensore_enabled);
    address = 348;
    EEPROM.writeInt(address, pedal_Count);

    delay(10);
    EEPROM.commit();
    delay(10);
    Serial.println("EEPROM first");
  } else {
    address = 0;
    e_up[1] = EEPROM.readInt(address);
    address = 4;
    c_up[1] = EEPROM.readInt(address);
    address = 8;
    s_up[1] = EEPROM.readInt(address);
    address = 12;

    e_up[2] = EEPROM.readInt(address);
    address = 16;
    c_up[2] = EEPROM.readInt(address);
    address = 20;
    s_up[2] = EEPROM.readInt(address);
    address = 24;

    e_up[3] = EEPROM.readInt(address);
    address = 28;
    c_up[3] = EEPROM.readInt(address);
    address = 32;
    s_up[3] = EEPROM.readInt(address);
    address = 36;

    e_up[4] = EEPROM.readInt(address);
    address = 40;
    c_up[4] = EEPROM.readInt(address);
    address = 44;
    s_up[4] = EEPROM.readInt(address);
    address = 48;

    e_up[5] = EEPROM.readInt(address);
    address = 52;
    c_up[5] = EEPROM.readInt(address);
    address = 56;
    s_up[5] = EEPROM.readInt(address);
    address = 60;

    program_no = EEPROM.readInt(address);

    address = 64;
    PID = EEPROM.readInt(address);
    address = 68;
    isJ = EEPROM.readInt(address);
    //////////////////////////cleaner //////////
    address = 72;
    blade_mode = EEPROM.readInt(address);
    address = 76;
    ability = EEPROM.readInt(address);
    address = 80;
    motor_type = EEPROM.readInt(address);
    address = 84;
    device_mode = ((EEPROM.readInt(address)) > MODES_SIZE) ? MODE_WELDER_SINGLE : Mode(EEPROM.readInt(address));
    if (device_mode > MODES_SIZE || device_mode == 0)
      Mode(1);
    ch_mode = device_mode;
    address = 88;
    buzzer_en = EEPROM.readInt(address);

    address = 92;
    ax_ready = EEPROM.readInt(address);

    address = 96;
    ax_back = EEPROM.readInt(address);

    address = 100;
    table_ready = EEPROM.readInt(address);

    address = 104;
    table_back = EEPROM.readInt(address);

    address = 108;
    heater_ready = EEPROM.readInt(address);

    address = 112;
    heater_back = EEPROM.readInt(address);

    address = 116;
    lock_time = EEPROM.readInt(address);

    address = 120;
    lock_timeing = EEPROM.readInt(address);

    address = 124;
    password_p = EEPROM.readInt(address);

    address = 128;
    lock_en = EEPROM.readInt(address);

    address = 132;
    adding_time = EEPROM.readInt(address);

    address = 136;
    hardware_test = EEPROM.readInt(address);

    address = 140;
    //ver = EEPROM.readInt(address);

    address = 144;
    tap_jack_1 = EEPROM.readInt(address);

    address = 148;
    tap_jack_2 = EEPROM.readInt(address);

    address = 152;
    stat_stop = EEPROM.readInt(address);

    address = 156;
    val_ok = EEPROM.readInt(address);

    address = 160;
    press_ready = EEPROM.readInt(address);

    address = 164;
    press_back = EEPROM.readInt(address);

    address = 168;
    finish_time = EEPROM.readInt(address);

    address = 172;
    auto_start = EEPROM.readInt(address);

    address = 176;
    temp_offset = EEPROM.readFloat(address);

    address = 380;
    lan_ = EEPROM.readInt(address);

    address = 344;
    jackSensore_enabled = EEPROM.readInt(address);

    address = 348;
    pedal_Count = EEPROM.readInt(address);


    unit_space = EEPROM.readInt(400);

    unit_welding = EEPROM.readFloat((120 + unit_space) * 4);  // 480

    unit_welding_p = EEPROM.readFloat((140 + unit_space) * 4);  // 560

    address = 0;
  }
  if (lock_en == 1) {
    lock_timeing = calculateDays();  //////////////////RTC ////////////
    int lock_timeing_copy = EEPROM.readInt(120);
    if (lock_timeing < lock_timeing_copy) {
      lock_timeing = lock_time;
    } else {
      EEPROM.writeInt(120, lock_timeing);
      delay(10);
      EEPROM.commit();
      delay(10);
    }
  }
  for (int i = 0; i < 6; i++) {
    sTime[i] = s_up[i];
    cTemp[i] = c_up[i];
    eTime[i] = e_up[i];
  }
  Serial.println("Set Variable");
}
void error_display(int step_error) {
  //lcd_disabled = true;
  for (int i = 0; i < step_error; i++) {
    buzzer_warning(1);
    Delay(50);
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, COLOR_RGB[7]);
    }
    pixels.show();
    up_seg(OFF_, ON_, ON_, ON_, ON_, OFF_, OFF_, 7);

    buzzer_warning(1);
    Delay(50);
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, 0x140000);  //red & dark
    }
    pixels.show();
    up_seg(D_D, ON_, ON_, ON_, ON_, D_D, OFF_, 7);
  }
  //lcd_disabled = false;
  update_f();
}
void seg_1(int number, int bright) {
  if (number < 1000 && seven_seg_1_copy != number) {
    seven_seg_1_copy = number;
    int one = number - (number / 10) * 10;
    int villagers = (number - (number / 100) * 100) / 10;
    int hundreds = number / 100;

    ledModule_1.setPatternAt(0, alphabetPattern[one]);
    if (number >= 10) {
      ledModule_1.setPatternAt(1, alphabetPattern[villagers]);
    } else ledModule_1.setPatternAt(1, 0);
    if (number >= 100) {
      ledModule_1.setPatternAt(2, alphabetPattern[hundreds]);
    } else ledModule_1.setPatternAt(2, 0);

    ledModule_1.setBrightness(bright);
    if (updat_data_lcd) ledModule_1.flush();
  }
}
void seg_2(int number, int bright) {
  if (number < 100 && seven_seg_2_copy != number) {
    seven_seg_2_copy = number;
    int one = number - (number / 10) * 10;
    int villagers = number / 10;

    ledModule_2.setPatternAt(3, alphabetPattern[one]);
    if (number >= 10) {
      ledModule_2.setPatternAt(4, alphabetPattern[villagers]);
    } else ledModule_2.setPatternAt(4, 0);

    ledModule_2.setBrightness(bright);
    if (updat_data_lcd) ledModule_2.flush();
  }
}
void seg_3(int number, int bright) {
  if (number < 100 && seven_seg_3_copy != number) {
    seven_seg_3_copy = number;
    int one = number - (number / 10) * 10;
    int villagers = number / 10;

    ledModule_2.setPatternAt(2, alphabetPattern[one]);
    if (number >= 10) {
      ledModule_2.setPatternAt(0, alphabetPattern[villagers]);
    } else ledModule_2.setPatternAt(0, 0);

    ledModule_2.setBrightness(bright);
    if (updat_data_lcd) ledModule_2.flush();
  }
}
void seg_4(int number, int bright) {
  if (number < 1000 && seven_seg_4_copy != number) {
    seven_seg_4_copy = number;
    int one = number - (number / 10) * 10;
    int villagers = (number - (number / 100) * 100) / 10;
    int hundreds = number / 100;

    ledModule_1.setPatternAt(5, alphabetPattern[one]);
    if (number >= 10) {
      ledModule_1.setPatternAt(3, alphabetPattern[villagers]);
    } else ledModule_1.setPatternAt(3, 0);
    if (number >= 100) {
      ledModule_1.setPatternAt(4, alphabetPattern[hundreds]);
    } else ledModule_1.setPatternAt(4, 0);

    ledModule_1.setBrightness(bright);
    if (updat_data_lcd) ledModule_1.flush();
  }
}
void seg_5(int number, int bright) {
  if (number < 60 && seven_seg_5_copy != number) {
    seven_seg_5_copy = number;
    int one = number;

    ledModule_2.setPatternAt(5, alphabetPattern[one]);

    ledModule_2.setBrightness(bright);
    if (updat_data_lcd) ledModule_2.flush();
  }
}
void in_seg(int number, int select_seg) {
  int val_1 = (number - (number / 10) * 10);
  int val_2 = (number - (number / 100) * 100) / 10;
  int val_3 = (number - (number / 1000) * 1000) / 100;
  int val_4 = (number - (number / 10000) * 10000) / 1000;
  int val_5 = (number - (number / 100000) * 100000) / 10000;
  int val_6 = (number - (number / 1000000) * 1000000) / 100000;
  int val_7 = (number - (number / 10000000) * 10000000) / 1000000;

  if (select_seg == 6) val_1 = val_1 + 10;
  if (select_seg == 5) val_2 = val_2 + 10;
  if (select_seg == 4) val_3 = val_3 + 10;
  if (select_seg == 3) val_4 = val_4 + 10;
  if (select_seg == 2) val_5 = val_5 + 10;
  if (select_seg == 1) val_6 = val_6 + 10;
  if (select_seg == 0) val_7 = val_7 + 10;

  ledModule_2.setPatternAt(2, alphabetPattern[val_1]);
  if (number >= 10) {
    ledModule_2.setPatternAt(0, alphabetPattern[val_2]);
  } else ledModule_2.setPatternAt(0, 0);
  if (number >= 100) {
    ledModule_2.setPatternAt(3, alphabetPattern[val_3]);
  } else ledModule_2.setPatternAt(3, 0);
  if (number >= 1000) {
    ledModule_2.setPatternAt(4, alphabetPattern[val_4]);
  } else ledModule_2.setPatternAt(4, 0);
  if (number >= 10000) {
    ledModule_1.setPatternAt(0, alphabetPattern[val_5]);
  } else ledModule_1.setPatternAt(0, 0);
  if (number >= 100000) {
    ledModule_1.setPatternAt(1, alphabetPattern[val_6]);
  } else ledModule_1.setPatternAt(1, 0);
  if (set_faun_val == 0) {
    if (number >= 1000000) {
      ledModule_1.setPatternAt(2, alphabetPattern[val_7]);
    } else ledModule_1.setPatternAt(2, 0);
  }

  ledModule_2.setBrightness(7);
  if (updat_data_lcd) ledModule_2.flush();
  ledModule_1.setBrightness(7);
  if (updat_data_lcd) ledModule_1.flush();
}
void up_seg(int S_1, int S_2, int S_3, int S_4, int S_5, int S_6, int S_7, int bright) {
  if (S_1 < ON_) ledModule_1.setPatternAt(2, alphabetPattern[S_1]);
  if (S_2 < ON_) ledModule_1.setPatternAt(1, alphabetPattern[S_2]);
  if (S_3 < ON_) ledModule_1.setPatternAt(0, alphabetPattern[S_3]);
  if (S_4 < ON_) ledModule_2.setPatternAt(4, alphabetPattern[S_4]);
  if (S_5 < ON_) ledModule_2.setPatternAt(3, alphabetPattern[S_5]);
  if (S_6 < ON_) ledModule_2.setPatternAt(0, alphabetPattern[S_6]);
  if (S_7 < ON_) ledModule_2.setPatternAt(2, alphabetPattern[S_7]);

  ledModule_2.setBrightness(bright);
  if (updat_data_lcd) ledModule_2.flush();
  ledModule_1.setBrightness(bright);
  if (updat_data_lcd) ledModule_1.flush();
}
void up_seg_step(int S_1, int S_2, int S_3, int S_4, int S_5, int S_6, int S_7, int bright) {
  int delay_seg = 50;
  for (int i = 0; i <= 5; i++) {
    ledModule_1.setPatternAt(i, 0);
    if (i != 1) ledModule_2.setPatternAt(i, 0);
  }
  ledModule_2.setBrightness(bright);
  ledModule_1.setBrightness(bright);

  if (S_1 < ON_) {
    ledModule_1.setPatternAt(2, alphabetPattern[S_1]);
    ledModule_1.flush();
    ledModule_2.flush();
    delay(delay_seg);
  }
  if (S_2 < ON_) {
    ledModule_1.setPatternAt(1, alphabetPattern[S_2]);
    ledModule_1.flush();
    delay(delay_seg);
  }
  if (S_3 < ON_) {
    ledModule_1.setPatternAt(0, alphabetPattern[S_3]);
    ledModule_1.flush();
    delay(delay_seg);
  }
  if (S_4 < ON_) {
    ledModule_2.setPatternAt(4, alphabetPattern[S_4]);
    ledModule_2.flush();
    delay(delay_seg);
  }
  if (S_5 < ON_) {
    ledModule_2.setPatternAt(3, alphabetPattern[S_5]);
    ledModule_2.flush();
    delay(delay_seg);
  }
  if (S_6 < ON_) {
    ledModule_2.setPatternAt(0, alphabetPattern[S_6]);
    ledModule_2.flush();
    delay(delay_seg);
  }
  if (S_7 < ON_) {
    ledModule_2.setPatternAt(2, alphabetPattern[S_7]);
    ledModule_2.flush();
    delay(delay_seg);
  }
}
void down_seg(int S_1, int S_2, int S_3, int S_4, int bright) {
  if (S_1 < ON_) ledModule_1.setPatternAt(4, alphabetPattern[S_1]);
  if (S_2 < ON_) ledModule_1.setPatternAt(3, alphabetPattern[S_2]);
  if (S_3 < ON_) ledModule_1.setPatternAt(5, alphabetPattern[S_3]);
  if (S_4 < ON_) ledModule_2.setPatternAt(5, alphabetPattern[S_4]);

  ledModule_2.setBrightness(bright);
  if (updat_data_lcd) ledModule_2.flush();
  ledModule_1.setBrightness(bright);
  if (updat_data_lcd) ledModule_1.flush();
}
void buzzer(int setp_beeb) {
  for (int i = 0; i < setp_beeb; i++) {
    if (buzzer_en == 1) {
      digitalWrite(PIN_BUZZER, HIGH);
      delay(55);
      digitalWrite(PIN_BUZZER, LOW);
      if (setp_beeb == 1) break;
      delay(45);
    } else delay(100);
  }
}
void buzzer_warning(int setp_beeb) {
  for (int i = 0; i < setp_beeb; i++) {
    if (buzzer_en == 1) {
      digitalWrite(PIN_BUZZER, HIGH);
      delay(150);
      digitalWrite(PIN_BUZZER, LOW);
      if (setp_beeb == 1) break;
      delay(50);
    } else delay(200);
  }
}
void IRAM_ATTR rgb_teme(void) {
  if (firstPixelHue < 5 * 65536) {
    firstPixelHue += 256;
  } else firstPixelHue = 0;
  pixels.rainbow(firstPixelHue);
  pixels.show();
}
void average(void) {
  for (int i = 0; i < 50; i++) {
    ADC_1 = ADC_1 + analogRead(ADC_KEY_1);
    ADC_2 = ADC_2 + analogRead(ADC_KEY_2);
    delayMicroseconds(3);
  }
  ADC_1 = ADC_1 / 500;
  ADC_2 = ADC_2 / 500;
}
int scan_key(void) {
  average();
  // 126   // 201    //264    //355    //411
  // 126   // 201    //264    //355    //411
  // 110   // 181    //255    // 335  // 410
  /////////////////////////////////////////// stop ///////////////////////////
  if ((ADC_2 > 50) && (ADC_2 < 160)) {
    delay(70);
    average();
    if ((ADC_2 > 50) && (ADC_2 < 160)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 10;
    }
  }
  /////////////////////////////////////////// stop ///////////////////////////
  if ((ADC_1 > 380) && (ADC_1 < 415)) {
    delay(5);
    average();
    if ((ADC_1 > 380) && (ADC_1 < 415)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 1;
    }
  }

  if ((ADC_1 > 300) && (ADC_1 < 380)) {
    delay(5);
    average();
    if ((ADC_1 > 300) && (ADC_1 < 380)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 2;
    }
  }

  if ((ADC_1 > 230) && (ADC_1 < 300)) {
    delay(5);
    average();
    if ((ADC_1 > 230) && (ADC_1 < 300)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 3;
    }
  }

  if ((ADC_1 > 160) && (ADC_1 < 230)) {
    delay(5);
    average();
    if ((ADC_1 > 160) && (ADC_1 < 230)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 4;
    }
  }

  if ((ADC_1 > 50) && (ADC_1 < 160)) {
    delay(70);
    average();
    if ((ADC_1 > 50) && (ADC_1 < 160)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 5;
    }
  }
  /////////////////////////////////////////////////////
  if ((ADC_2 > 380) && (ADC_2 < 415)) {
    delay(5);
    average();
    if ((ADC_2 > 380) && (ADC_2 < 415)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 6;
    }
  }

  if ((ADC_2 > 300) && (ADC_2 < 380)) {
    delay(5);
    average();
    if ((ADC_2 > 300) && (ADC_2 < 380)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 7;
    }
  }
  if ((ADC_2 > 230) && (ADC_2 < 300)) {
    delay(5);
    average();
    if ((ADC_2 > 230) && (ADC_2 < 300)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 8;
    }
  }

  if ((ADC_2 > 160) && (ADC_2 < 230)) {
    delay(5);
    average();
    if ((ADC_2 > 160) && (ADC_2 < 230)) {
      if (p_buzzer == 0) {
        p_buzzer = 1;
        buzzer(1);
      }
      return 9;
    }
    return 100;
  }
}
void demo(void) {
  Serial.println("Demo Playe");

  //up_seg_step(R_, R_, R_, R_, R_, R_, R_, 7);
  up_seg_step(T_, U_, R_, A_, N_, T_, E_, 7);
  //up_seg_step(H_, A_, F_, F_, L_, E_, R_, 7);

  if (lock_en == 0) {
    down_seg(U_, (ver / 100) + 10, (ver - ((ver / 100) * 100)) / 10, ON_, 7);  //ver = 2.8
  } else seg_4(lock_time - lock_timeing, 7);

  if (isJ) {
    ledModule_2.setPatternAt(5, alphabetPattern[K_]);       //k   A G F E D B C H
  } else ledModule_2.setPatternAt(5, alphabetPattern[J_]);  //J   A G F E D B C H

  ledModule_1.flush();
  ledModule_2.flush();

  for (int j = 0; j < 256; j++) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, Wheel((i + j) & 255));
    }
    pixels.show();
    delay(1);
    select_function();
    clear_memory();
    if (break_rgb_fun) break;
  }
  // for (int t = 5; t < 11; t++) {
  //   for (int i = 0; i < NUMPIXELS; i++) {
  //     pixels.setPixelColor(i, COLOR_RGB[t]);
  //     pixels.show();
  //     Delay(35);
  //   }
  //   if (t < 10) {
  //     t++;
  //   } else break;
  //   for (int i = NUMPIXELS; i >= 0; i--) {
  //     pixels.setPixelColor(i, COLOR_RGB[t]);
  //     pixels.show();
  //     Delay(35);
  //   }
  //   if (t < 10) {
  //     t++;
  //   } else break;
  //   for (int i = 0; i <= (NUMPIXELS / 2); i++) {
  //     pixels.setPixelColor(i, COLOR_RGB[t]);
  //     pixels.setPixelColor(NUMPIXELS - i, COLOR_RGB[t]);
  //     pixels.show();
  //     Delay(60);
  //   }
  //   if (t < 10) {
  //     t++;
  //   } else break;
  //   for (int i = (NUMPIXELS / 2); i >= 0; i--) {
  //     pixels.setPixelColor(i, COLOR_RGB[t]);
  //     pixels.setPixelColor(NUMPIXELS - i, COLOR_RGB[t]);
  //     pixels.show();
  //     Delay(60);
  //   }
  // }
  led_stat = led_stat | wifi_signal;
  buzzer(2);
}
void sement_off(void) {
  // up_seg(OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, OFF_, 7);
  // down_seg(OFF_, OFF_, OFF_, OFF_, 7);
  for (int i = 0; i <= 5; i++) {
    ledModule_1.setPatternAt(i, 0);
    if (i != 1) ledModule_2.setPatternAt(i, 0);
  }
  ledModule_1.flush();
  ledModule_2.flush();
}
void check_lock_time(void) {
  Serial.println("Time limit");
  if (lock_timeing >= lock_time || val_ds3231 == 0 || no_battery == 1) {
    int step_mode = 0, password_copy = 1000000;
    int count_false_pass = 0;
    bool p_stat_f9 = 0, p_stat_f10 = 0, p_stat_f8 = 0;
    bool refresh_lcd = 1;
    heater_en = 0;
    digitalWrite(heater, Disabled);
    Serial.println("System Lock");
    up_seg_step(S_, Y_, S_, E_E, L_, O_, C_, 7);
    down_seg(OFF_, OFF_, OFF_, OFF_, 7);
    buzzer_warning(3);
    Delay(2000);
    if (val_ds3231 == 0) {
      up_seg_step(D_, I_, S_, E_E, R_, T_, C_, 7);
      Serial.println("Disconect Ds3231");
      Delay(2000);
    }
    if (no_battery == 1) {
      up_seg_step(N_, o_, B_, A_, T_, R_, Y_, 7);
      Serial.println("No Battery Ds3231");
      Delay(2000);
    }
    if (lock_timeing >= lock_time) {
      up_seg_step(E_, N_, D_, T_, I_, M_, E_, 7);
      Serial.println("End Time");
      Delay(2000);
    }
    set_faun_val = 1;
    while (1) {
      Delay(5);
      /////////////////////////////// F8 //////////////////////
      if (keyNum == F8) {
        if (p_stat_f8 == 0) {
          p_stat_f8 = 1;
          refresh_lcd = 1;
          if (step_mode < 5) {
            step_mode++;
          } else step_mode = 0;
        }
      } else p_stat_f8 = 0;

      /////////////////////////////// F9 //////////////////////
      if (keyNum == F9) {
        if (p_stat_f9 == 0) {
          p_stat_f9 = 1;
          refresh_lcd = 1;
          if (step_mode == 0 && password_copy < 1900000) password_copy = password_copy + 100000;
          if (step_mode == 1 && password_copy < 1990000) password_copy = password_copy + 10000;

          if (step_mode == 2 && password_copy < 1999000) password_copy = password_copy + 1000;
          if (step_mode == 3 && password_copy < 1999900) password_copy = password_copy + 100;

          if (step_mode == 4 && password_copy < 1999990) password_copy = password_copy + 10;
          if (step_mode == 5 && password_copy < 1999999) password_copy = password_copy + 1;
        }
      } else p_stat_f9 = 0;

      /////////////////////////////// F10 //////////////////////
      if (keyNum == F10) {
        if (p_stat_f10 == 0) {
          p_stat_f10 = 1;
          refresh_lcd = 1;
          if (step_mode == 0 && password_copy >= 1100000) password_copy = password_copy - 100000;
          if (step_mode == 1 && password_copy >= 1010000) password_copy = password_copy - 10000;

          if (step_mode == 2 && password_copy >= 1001000) password_copy = password_copy - 1000;
          if (step_mode == 3 && password_copy >= 1000100) password_copy = password_copy - 100;

          if (step_mode == 4 && password_copy >= 1000010) password_copy = password_copy - 10;
          if (step_mode == 5 && password_copy >= 1000001) password_copy = password_copy - 1;
        }
      } else p_stat_f10 = 0;

      if (refresh_lcd == 1) {
        refresh_lcd = 0;
        in_seg(password_copy, step_mode + 1);
        up_seg(P_, ON_, ON_, ON_, ON_, ON_, ON_, 7);
      }
      /////////////////////////////// F6 //////////////////////
      if (keyNum == F6) {
        if (password_copy == password_p || password_copy == PasswordAdmin || password_copy == PasswordAdmin_2) {
          lock_time = 0;
          lock_en = false;

          EEPROM.writeInt(116, lock_time);
          EEPROM.writeInt(128, lock_en);
          lock_timeing = calculateDays();      //////////////////RTC ////////////
          EEPROM.writeInt(120, lock_timeing);  ///////////////

          EEPROM.writeInt(384, year);
          EEPROM.writeInt(388, month);
          EEPROM.writeInt(392, day);
          EEPROM.writeInt(396, adding_time);

          adding_time = 0;
          EEPROM.writeInt(132, adding_time);

          delay(10);
          EEPROM.commit();
          delay(10);
          delay(10);
          EEPROM.commit();
          delay(10);
          while (1) {
            Delay(1);
            if (keyNum == 100) break;
          }
          Serial.println("Open System");
          up_seg_step(O_, P_, E_, N_, E_E, S_, Y_, 7);
          delay(100);
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, COLOR_RGB[5]);
          }
          pixels.show();
          Delay(1500);
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, COLOR_RGB[10]);
          }
          pixels.show();
          break;
        } else if (password_copy == PasswordAddTime && adding_time == 0) {
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, COLOR_RGB[5]);
          }
          pixels.show();
          up_seg_step(A_, D_, D_, E_E, D_, A_, Y_, 7);
          down_seg(D_, 1, 0, OFF_, 7);
          lock_timeing = calculateDays();
          lock_time = lock_timeing + 10;  //////////////////RTC ////////////
          adding_time = 1;
          EEPROM.writeInt(116, lock_time);
          EEPROM.writeInt(120, lock_timeing);
          EEPROM.writeInt(132, adding_time);
          Serial.print("Set Time : 10 (Day)");
          Serial.println(lock_time);
          delay(20);
          EEPROM.commit();
          delay(1500);
          Serial.println("restart");
          delay(10);
          ESP.restart();
        } else {
          buzzer_warning(2);
          Serial.println("Password False");
          while (1) {
            Delay(1);
            if (keyNum == 100) break;
          }
          delay(100);
          count_false_pass++;
          refresh_lcd = 1;
          if (count_false_pass > 3) {
            Serial.println("System Lock = 60(s)");
            buzzer_warning(4);
            count_false_pass = 0;
            up_seg_step(S_, Y_, S_, E_E, L_, O_, C_, 7);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[7]);
            }
            pixels.show();
            Delay(60000);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[10]);
            }
            pixels.show();
            password_copy = 1000000;
            step_mode = 0;
          } else {
            up_seg_step(P_, A_, S_, S_, E_E, N_, O_, 7);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[7]);
            }
            pixels.show();
            Delay(1500);
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, COLOR_RGB[0]);
            }
            pixels.show();
          }
        }
        //break;
      }
    }
    set_faun_val = 0;
  }
}
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
bool isLeapYear(int year) {  // بررسی سال کبیسه

  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}
long calculateDays(void) {  // محاسبه تعداد روزها از تاریخ مبنا
  // تعداد روزهای هر ماه
  int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  // محاسبه تعداد روزهای سال‌های کامل سپری‌شده
  long totalDays = 0;
  //////////// START_YEAR
  for (int y = 2020; y < year; y++) {
    totalDays += isLeapYear(y) ? 366 : 365;
  }

  // تنظیم فوریه برای سال جاری در صورت کبیسه بودن
  if (isLeapYear(year)) {
    daysInMonth[1] = 29;
  }

  // محاسبه تعداد روزهای ماه‌های کامل سپری‌شده
  for (int m = 0; m < month - 1; m++) {
    totalDays += daysInMonth[m];
  }

  // اضافه کردن روزهای سپری‌شده از ماه جاری
  totalDays += day;

  return totalDays;
}
void read_time_ds3231(void) {
  Serial.end();
  Wire.begin(1, 3);
  Wire.beginTransmission(0x68);  //address : DS3231
  byte error = Wire.endTransmission();
  if (!cap.begin(0x5A)) {  //0x5A  -- 0x5C
    val_mpr121 = 0;
  }
  if (error == 0) {
    val_ds3231 = true;
    //setRTCDateTime();              /////////////
    dayOfWeek = rtc.getDoW();  // دریافت روز هفته (0 تا 7)
    //if (dayOfWeek == 8) dayOfWeek = 0;
    uint8_t status = readRegister(0x0F);
    // بررسی بیت OSF
    if (status & 0x80) {
      no_battery = 1;
      // بازنشانی بیت OSF
      status &= ~0x80;
      writeRegister(0x0F, status);
    }

    if (rtc.getYear() + 2000 < 2024) {
      if (lock_en == 1) {
        falt_ds3231 = 1;
      } else {
        setRTCDateTime();
        set_time_ds3231 = 1;
      }
    }
    ////////////////////////////////////////////////////
    String date = __DATE__;
    String time = __TIME__;
    day = date.substring(4, 6).toInt();
    year = date.substring(7).toInt();
    month = monthToNumber(date.substring(0, 3));

    hour = time.substring(0, 2).toInt();
    minute = time.substring(3, 5).toInt();
    second = time.substring(6).toInt();

    int All_Days_program = calculateDays();
    //////////////////////////////////////////////////
    bool h12Flag, pmFlag, century = false;
    year = rtc.getYear() + 2000;
    month = rtc.getMonth(century);
    day = rtc.getDate();
    hour = rtc.getHour(h12Flag, pmFlag);
    minute = rtc.getMinute();
    second = rtc.getSecond();
    temperature = rtc.getTemperature();

    int All_Days_ds3231 = calculateDays();
    if (lock_en == 0 && All_Days_ds3231 < All_Days_program) {
      setRTCDateTime();
      set_time_ds3231 = 1;
    }
  }
  Wire.end();
  Serial.begin(115200);
  delay(1);
  Serial.println("");
  Serial.println("Read RTC");
  if (val_ds3231 == 0) Serial.println("DS3231 : False");
  if (no_battery == 1) Serial.println("Battery disconnected or oscillator stopped!");
  if (set_time_ds3231 == 1) Serial.println("Set Time");
  if (falt_ds3231 == 1) Serial.println("DS3231 : Cheng");
  if (val_mpr121 == 0) {
    Serial.println("MPR121 not found, check wiring?");
  } else Serial.println("MPR121 found!");
}
void setRTCDateTime() {
  // تجزیه تاریخ و زمان از ماکروهای __DATE__ و __TIME__
  String date = __DATE__;
  String time = __TIME__;
  day = date.substring(4, 6).toInt();
  year = date.substring(7).toInt();
  month = monthToNumber(date.substring(0, 3));

  hour = time.substring(0, 2).toInt();
  minute = time.substring(3, 5).toInt();
  second = time.substring(6).toInt();

  // تنظیم زمان روی DS3231
  rtc.setClockMode(false);   // فرمت 24 ساعته
  rtc.setYear(year - 2000);  // DS3231 فقط دو رقم آخر سال را ذخیره می‌کند
  rtc.setMonth(month);
  rtc.setDate(day);
  rtc.setHour(hour);
  rtc.setMinute(minute);
  rtc.setSecond(second);
  rtc.setDoW(calculateDayOfWeek());  //روز های هفته از یکشنبه 0 تا 7
}
int monthToNumber(String month) {
  if (month == "Jan") return 1;
  if (month == "Feb") return 2;
  if (month == "Mar") return 3;
  if (month == "Apr") return 4;
  if (month == "May") return 5;
  if (month == "Jun") return 6;
  if (month == "Jul") return 7;
  if (month == "Aug") return 8;
  if (month == "Sep") return 9;
  if (month == "Oct") return 10;
  if (month == "Nov") return 11;
  if (month == "Dec") return 12;
  return 0;  // در صورت اشتباه، عدد 0 برمی‌گرداند
}
void clear_memory(void) {
  if ((ADC_2 > 354) && (ADC_2 < 380)) {
    delay(3);
    average();
    if ((ADC_2 > 354) && (ADC_2 < 380)) {
      buzzer_warning(3);
      delay(1000);
      average();
      if ((ADC_2 > 354) && (ADC_2 < 380)) {
        read_write_eeprom(1);
        unit_welding = 0;
        up_seg_step(S_, U_, C_, C_, E_, S_, S_, 7);
        Serial.println("Rest Factory");
        Serial.println("restart");
        update_data = true;
        save_var();
        display_eeprom();
        delay(1500);
        ESP.restart();
      }
    }
  }
}
uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(0x68);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);
  return Wire.read();
}
void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(0x68);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}
uint8_t calculateDayOfWeek(void) {
  if (month < 3) {
    month += 12;
    year -= 1;
  }

  uint8_t k = year % 100;
  uint16_t j = year / 100;

  uint8_t dow = (day + (13 * (month + 1)) / 5 + k + (k / 4) + (j / 4) - (2 * j)) % 7;

  // تطبیق با فرمت استاندارد (شنبه = 0)
  return dow;
}
void fade_seven_sgment(void) {
  for (int i = 7; i > 0; i = i - 3) {
    if (i == 1) i = 0;
    ledModule_1.setBrightness(i);
    ledModule_2.setBrightness(i);
    ledModule_1.flush();
    ledModule_2.flush();
  }
  for (int i = 0; i <= 5; i++) {
    ledModule_1.setPatternAt(i, 0);
    if (i != 1) ledModule_2.setPatternAt(i, 0);
  }
  ledModule_1.flush();
  ledModule_2.flush();
}
void admin_eeprom(void) {
  EEPROM.writeInt(200, e_up[1]);  // e_up[1] = 20;
  EEPROM.writeInt(204, e_up[2]);  // e_up[2] = 19;
  EEPROM.writeInt(208, e_up[3]);  // e_up[3] = 18;
  EEPROM.writeInt(212, e_up[4]);  // e_up[4] = 17;
  EEPROM.writeInt(216, e_up[5]);  // e_up[5] = 16;

  EEPROM.writeInt(220, c_up[1]);  // c_up[1] = 205;
  EEPROM.writeInt(224, c_up[2]);  // c_up[2] = 215;
  EEPROM.writeInt(228, c_up[3]);  // c_up[3] = 230;
  EEPROM.writeInt(232, c_up[4]);  // c_up[4] = 235;
  EEPROM.writeInt(236, c_up[5]);  // c_up[5] = 240;

  EEPROM.writeInt(240, s_up[1]);  // s_up[1] = 20;
  EEPROM.writeInt(244, s_up[2]);  // s_up[2] = 19;
  EEPROM.writeInt(248, s_up[3]);  // s_up[3] = 18;
  EEPROM.writeInt(252, s_up[4]);  // s_up[4] = 17;
  EEPROM.writeInt(256, s_up[5]);  // s_up[5] = 16;

  EEPROM.writeInt(260, PID);                  // PID = 15;
  EEPROM.writeInt(264, program_no);           // program_no = 1;
  EEPROM.writeInt(268, buzzer_en);            // buzzer_en = 1;  //Enable
  EEPROM.writeInt(272, ax_ready);             // ax_ready = 1200;
  EEPROM.writeInt(276, ax_back);              // ax_back = 1200;
  EEPROM.writeInt(280, table_ready);          // table_ready = 1000;
  EEPROM.writeInt(284, table_back);           // table_back = 600;
  EEPROM.writeInt(288, heater_ready);         // heater_ready = 2100;
  EEPROM.writeInt(292, heater_back);          // heater_back = 2100;
  EEPROM.writeInt(296, press_ready);          // press_ready = 600;
  EEPROM.writeInt(300, press_back);           // press_back = 500;
  EEPROM.writeInt(304, tap_jack_1);           // tap_jack_1 = 3;
  EEPROM.writeInt(308, tap_jack_2);           // tap_jack_2 = 6;
  EEPROM.writeInt(312, val_ok);               // val_ok = 0;
  EEPROM.writeFloat(316, temp_offset);        // temp_offset = 1;
  EEPROM.writeInt(320, isJ);                  // isJ = false;
  EEPROM.writeInt(324, finish_time);          // finish_time = true;
  EEPROM.writeInt(328, auto_start);           // auto_start = 1;
  EEPROM.writeInt(332, lan_);                 // Lan_ = 1; // FA
  EEPROM.writeInt(336, jackSensore_enabled);  // jackSensore_enabled = 0;  Not Sensor
  EEPROM.writeInt(340, pedal_Count);          // pedal_Count = 3 or 4

  Serial.println("admin Write");
}
void rest_eeprom_admin(void) {
  EEPROM.writeInt(200, 20);  // e_up[1] = 20;
  EEPROM.writeInt(204, 19);  // e_up[2] = 19;
  EEPROM.writeInt(208, 18);  // e_up[3] = 18;
  EEPROM.writeInt(212, 17);  // e_up[4] = 17;
  EEPROM.writeInt(216, 16);  // e_up[5] = 16;

  EEPROM.writeInt(220, 205);  // c_up[1] = 205;
  EEPROM.writeInt(224, 215);  // c_up[2] = 215;
  EEPROM.writeInt(228, 230);  // c_up[3] = 230;
  EEPROM.writeInt(232, 235);  // c_up[4] = 235;
  EEPROM.writeInt(236, 240);  // c_up[5] = 240;

  EEPROM.writeInt(240, 20);  // s_up[1] = 20;
  EEPROM.writeInt(244, 19);  // s_up[2] = 19;
  EEPROM.writeInt(248, 18);  // s_up[3] = 18;
  EEPROM.writeInt(252, 17);  // s_up[4] = 17;
  EEPROM.writeInt(256, 16);  // s_up[5] = 16;

  EEPROM.writeInt(260, 15);    // PID = 15;
  EEPROM.writeInt(264, 1);     // program_no = 1;
  EEPROM.writeInt(268, 1);     // buzzer_en = 1;  //Enable
  EEPROM.writeInt(272, 1200);  // ax_ready = 1200;
  EEPROM.writeInt(276, 1200);  // ax_back = 1200;
  EEPROM.writeInt(280, 1000);  // table_ready = 1000;
  EEPROM.writeInt(284, 600);   // table_back = 600;
  EEPROM.writeInt(288, 2100);  // heater_ready = 2100;
  EEPROM.writeInt(292, 2100);  // heater_back = 2100;
  EEPROM.writeInt(296, 600);   // press_ready = 600;
  EEPROM.writeInt(300, 500);   // press_back = 500;
  EEPROM.writeInt(304, 3);     // tap_jack_1 = 3;
  EEPROM.writeInt(308, 6);     // tap_jack_2 = 6;
  EEPROM.writeInt(312, 0);     // val_ok = 0;
  EEPROM.writeFloat(316, 1);   // temp_offset = 1;
  EEPROM.writeInt(320, 0);     // isJ = false;
  EEPROM.writeInt(324, 1);     // finish_time = true;
  EEPROM.writeInt(328, 1);     // auto_start = 1;
  EEPROM.writeInt(332, 4);     // lan_ = 4; // TU  NOT FA ///////////////////////
  EEPROM.writeInt(336, 0);     // jackSensore_enabled = false;
  EEPROM.writeInt(340, 3);     // pedal_Count = 3;
  Serial.println("Rest EEPROM Admin");
}
bool check_jack(bool sensor_val) {
  if (sensor_val == 1) {
    int av_adc_1 = 0, av_adc_2 = 0, min_sen = 150;
    for (int i = 0; i < 90; i++) {
      av_adc_1 = av_adc_1 + analogRead(s_jack_shablon);
      av_adc_2 = av_adc_2 + analogRead(s_jack_heater);
      delayMicroseconds(11);
    }
    av_adc_1 = av_adc_1 / 900;
    av_adc_2 = av_adc_2 / 900;
    if (av_adc_1 > min_sen) {
      e_s_shablon = 1;
    } else e_s_shablon = 0;

    if (av_adc_2 > min_sen) {
      e_s_heater = 1;
    } else e_s_heater = 0;

    if (digitalRead(s_jack_table) == 1) {
      e_s_tabel = 1;
    } else e_s_tabel = 0;

    if (av_adc_1 < min_sen && av_adc_2 < min_sen && digitalRead(s_jack_table) == 0) {
      return 1;
    } else return 0;
  }
}
int sen_read(int key_pin, bool sen_stat) {
  // jack heater     = Start_2
  // jack shablon    = back_ward
  // jack back tabel = back_ward_2
  // 88   --         336
  int min_sen = 150, max_sen = 350;
  int step_error_ = 0;
  bool break_error = 0;
  while (1) {
    esp_task_wdt_reset();
    step_error_ = step_error_ + 10;
    int av_adc = 0;
    if (key_pin != s_jack_table) {
      for (int i = 0; i < 90; i++) {
        av_adc = av_adc + analogRead(key_pin);
        delayMicroseconds(11);
      }
      av_adc = av_adc / 900;
      Delay(1);
    } else {
      //av_adc = 240;
      //if (sen_stat == 0)
      if (digitalRead(s_jack_table) == 0) av_adc = 0;
      //if (sen_stat == 1)
      if (digitalRead(s_jack_table) == 1) av_adc = 410;
      Delay(10);
    }

    if (sen_stat == 0) {
      if (av_adc < min_sen) return 0;

      if (key_pin == s_jack_heater) {
        if (step_error_ > (heater_back * 1.5)) {
          lcd_disabled = true;
          up_seg_step(OFF_, H_, E_, A_, T_, OFF_, OFF_, 7);
          down_seg(B_, A_, C_, OFF_, 7);
          error_display(2);
          if (av_adc > max_sen) {
            down_seg(A_, I_, R_, OFF_, 7);
            error_display(2);
          }
          lcd_disabled = false;
          //sement_off();
          break_error = 1;
        }
      }
      if (key_pin == s_jack_shablon) {
        if (step_error_ > (ax_back * 1.5)) {
          lcd_disabled = true;
          up_seg_step(OFF_, S_, H_, A_, B_, OFF_, OFF_, 7);
          down_seg(B_, A_, C_, OFF_, 7);
          error_display(2);
          if (av_adc > max_sen) {
            down_seg(A_, I_, R_, OFF_, 7);
            error_display(2);
          }
          lcd_disabled = false;
          //sement_off();
          break_error = 1;
        }
      }
      if (key_pin == s_jack_table) {
        if (step_error_ > (table_back * 1.5)) {
          lcd_disabled = true;
          up_seg_step(OFF_, T_, A_, B_, L_, OFF_, OFF_, 7);
          down_seg(B_, A_, C_, OFF_, 7);
          error_display(2);
          lcd_disabled = false;
          //sement_off();
          break_error = 1;
        }
      }
    } else if (sen_stat == 1) {
      if (av_adc > max_sen) return 1;
      if (key_pin == s_jack_heater) {
        if (step_error_ > (heater_ready * 1.5)) {
          lcd_disabled = true;
          up_seg_step(OFF_, H_, E_, A_, T_, OFF_, OFF_, 7);
          down_seg(F_, o_, R_, OFF_, 7);
          error_display(2);
          if (av_adc > max_sen) {
            down_seg(A_, I_, R_, OFF_, 7);
            error_display(2);
          }
          lcd_disabled = false;
          //sement_off();
          break_error = 1;
        }
      }
      if (key_pin == s_jack_shablon) {
        if (step_error_ > (ax_ready * 1.5)) {
          lcd_disabled = true;
          up_seg_step(OFF_, S_, H_, A_, B_, OFF_, OFF_, 7);
          down_seg(F_, o_, R_, OFF_, 7);
          error_display(2);
          if (av_adc > max_sen) {
            down_seg(A_, I_, R_, OFF_, 7);
            error_display(2);
          }
          lcd_disabled = false;
          //sement_off();
          break_error = 1;
        }
      }
      if (key_pin == s_jack_table) {
        if (step_error_ > (table_ready * 1.5)) {
          lcd_disabled = true;
          up_seg_step(OFF_, T_, A_, B_, L_, OFF_, OFF_, 7);
          down_seg(B_, A_, C_, OFF_, 7);
          error_display(2);
          lcd_disabled = false;
          //sement_off();
          break_error = 1;
        }
      }
    }
    if (break_error == 1) {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, COLOR_RGB[0]);
      }
      pixels.show();
      for (int i = NUMPIXELS; i >= step_stat_rgb; i--) {
        pixels.setPixelColor(i, COLOR_RGB[color_rgb_sin]);
      }
      pixels.show();
      Serial.println(step_stat_rgb);
      break_error = 0;
      step_error_ = -1000;
      //return 100;
    }
  }
}
//
