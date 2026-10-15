bool saveParams(int e, int c, int s, int pid, bool J, int _mot, int _abil, int _blade, int lang) {
  e_up = e;
  c_up = c;
  s_up = s;
  PID = pid;
  isJ = J;
  motor_type = _mot;
  ability = _abil;
  blade_mode = _blade;
  language = lang;
  update_data = true;
  return (true);
}
bool saveDeviceMode(Mode mode) {
  device_mode = mode;
  update_data = true;
  return (true);
}

void setupPin(void) {
  pinMode(Start, INPUT_PULLUP);
  pinMode(Stop, INPUT_PULLUP);
  pinMode(mic_sw_blade, INPUT_PULLUP);
  pinMode(mic_sw_motor, INPUT_PULLUP);

  pinMode(blade_mode_1, OUTPUT);
  pinMode(blade_mode_2, OUTPUT);
  pinMode(motor_1, OUTPUT);

  pinMode(motor_2, OUTPUT);
  pinMode(profile_holder_jack, OUTPUT);
  pinMode(motor_replace, OUTPUT);
  pinMode(motor_blade_move, OUTPUT);
  pinMode(up_down_nails, OUTPUT);

  digitalWrite(motor_1, Disabled);
  digitalWrite(motor_2, Disabled);
  digitalWrite(profile_holder_jack, Disabled);

  digitalWrite(motor_blade_move, Disabled);
  digitalWrite(up_down_nails, Disabled);

  if (device_mode == MODE_CLEANER) {
    if (blade_mode == 1) {
      digitalWrite(blade_mode_1, Disabled);
      digitalWrite(blade_mode_2, Disabled);
    } else if (blade_mode == 2) {
      digitalWrite(blade_mode_1, Enable);
      digitalWrite(blade_mode_2, Disabled);
    } else if (blade_mode == 3) {
      digitalWrite(blade_mode_1, Disabled);
      digitalWrite(blade_mode_2, Enable);
    } else if (blade_mode == 4) {
      digitalWrite(blade_mode_1, Enable);
      digitalWrite(blade_mode_2, Enable);
    }

    if (motor_type == 1)
      digitalWrite(motor_replace, Disabled);
    if (motor_type == 2)
      digitalWrite(motor_replace, Enable);
  } else {
    digitalWrite(blade_mode_1, Disabled);
    digitalWrite(blade_mode_2, Disabled);
    digitalWrite(motor_replace, Disabled);
    digitalWrite(tap_move_1, Disabled);
  }
}
////////////////////////////////////////////////////////code jamshidi //////////////////////////////////////////
void heater_contorl(void) {
  int set_time = 0, ti = 0;
  int c_up_adj = (c_up * 4);
  temp_adj = readTemp();
  if (isJ) {
    ///////////////////////////////////////////////TEMP K //////////////////////////
    c_up_adj = (c_up * 4);
    temp = (temp_adj / 4);
  } else {
    ///////////////////////////////////////////////TEMP J //////////////////////////
    c_up_adj = (c_up * 4) * 1.38;
    temp = (temp_adj / 4) / 1.38;
  }

  // if ((c_up < temp + 5 && c_up + 5 > temp) || val_ok == 0) {
  //   led_stat = led_stat | ok_signal;
  // } else led_stat = led_stat & ~ok_signal;

  // if (c_up < temp + 5 && c_up + 5 > temp) {
  //   falt_ssr = 0;
  // } else if (temp < 120) {
  //   if ((millis() > 1000 * 60 * 1) && falt_ssr == 0 && (millis() < 1000 * 60 * 2)) {
  //     if (temp > temp_last - 5 && temp < temp_last + 5) {
  //       falt_ssr = 1;
  //       Serial.println("Error Heater OR SSR");
  //     }
  //   }
  // }
  ///////////////////////////////////////////////////////////////////////
  if (((27 + temp) > c_up) && (temp < c_up + 9)) {
    ti = (temp_adj + welding_temp - (temp_adj / 2.3)) * PID;
    ti = (ti / 10);
    if (ti > 0) {
      if (temp + 20 < c_up && stat_down == 0) {
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
  } else if (temp < c_up) {
    if (temp + 40 < c_up) stat_down = 0;
    digitalWrite(heater, Enable);  // ON HEATER
  } else if (temp > c_up) {
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
    setupPin();
  }
  if (map1 == 0 && (read_key(Start) || keyNum == F5 || (device_mode == MODE_WELDER_DOUBLE && read_key(Start_2)))) {
    on_wi_fi = false;
    map1 = 1;
    map2 = 1;
    p_stat = 1;
    digitalWrite(shablon, Enable);
    Serial.println("shblon ON");
    lcd.setCursor(11, 1);
    lcd.write(byte(1));
    webApp.wStop();
    Delay(ax_ready - 100);  /////////////////////////////////////////// 1000
    digitalWrite(move_jack, Enable);
    Serial.println("MoveJack ON");
    lcd.setCursor(11, 1);  ///////////
    lcd.write(" ");
  }
  if (map1 > 0) {
    if ((read_key(Start) || keyNum == F5)) {
      if (p_stat == 0) {
        p_stat = 1;
        if (map1 == 1) {
          digitalWrite(jack_righ, Enable);
          Serial.println("rightJack ON");
          lcd.setCursor(11, 1);
          lcd.write(byte(1));
          Delay(500);
          lcd.setCursor(11, 1);
          lcd.write(" ");
          map1 = 2;
        } else if (map1 == 2) {
          digitalWrite(jack_left, Enable);
          Serial.println("leftJack ON");
          lcd.setCursor(11, 1);
          lcd.write(byte(1));
          Delay(1500);
          lcd.setCursor(11, 1);
          lcd.write(" ");
          map1 = 3;
          if (device_mode == MODE_WELDER_SINGLE)
            map2 = 3;
        }
      }
    } else
      p_stat = 0;

    if ((read_key(back_ward) || keyNum == F8)) {
      if (p_stat_1 == 0) {
        p_stat_1 = 1;
        if (map1 == 2 || map2 == 2) {
          if (map1 == 2) {
            digitalWrite(jack_righ, Disabled);
            Serial.println("rightJack OFF");
            map1 = 1;
          }
          if (device_mode == MODE_WELDER_DOUBLE) {
            if (map2 == 2) {
              digitalWrite(jack_left_2, Disabled);
              Serial.println("Jack Left 2 OFF");
              map2 = 1;
            }
          }
          lcd.setCursor(11, 1);
          lcd.write(byte(1));
          Delay(400);
          lcd.setCursor(11, 1);
          lcd.write(" ");
        }
        if (map1 == 3 || map2 == 3) {
          if (map1 == 3) {
            digitalWrite(jack_left, Disabled);
            Serial.println("rightJack OFF");
            map1 = 2;
          }
          if (device_mode == MODE_WELDER_DOUBLE) {
            if (map2 == 3) {
              digitalWrite(jack_righ_2, Disabled);
              Serial.println("Jack Righ 2 OFF");
              map2 = 2;
            }
          }
          lcd.setCursor(11, 1);
          lcd.write(byte(1));
          Delay(400);
          lcd.setCursor(11, 1);
          lcd.write(" ");
        }
      }
    } else p_stat_1 = 0;
  }
  if (device_mode == MODE_WELDER_DOUBLE && read_key(Start_2)) {
    if (p_stat_2 == 0) {
      p_stat_2 = 1;
      if (map2 == 1) {
        digitalWrite(jack_left_2, Enable);
        Serial.println("Jack Left 2 ON");
        lcd.setCursor(11, 1);
        lcd.write(byte(1));
        for (int i = 0; i < 5; i++) {
          if ((read_key(back_ward) || keyNum == F8)) break;
          Delay(100);
        }
        lcd.setCursor(11, 1);
        lcd.write(" ");
        map2 = 2;
      } else if (map2 == 2) {
        digitalWrite(jack_righ_2, Enable);
        Serial.println("Jack Righ 2 ON");
        lcd.setCursor(11, 1);
        lcd.write(byte(1));
        for (int i = 0; i < 15; i++) {
          if ((read_key(back_ward) || keyNum == F8)) break;
          Delay(100);
        }
        lcd.setCursor(11, 1);
        lcd.write(" ");
        map2 = 3;
      }
    }
  } else
    p_stat_2 = 0;

  if (map1 == 3 && map2 == 3) {
    if (read_key(Start) || keyNum == F5 || (read_key(Start_2) && device_mode == MODE_WELDER_DOUBLE) || device_mode == MODE_WELDER_SINGLE) {
      welding_temp = 150;
      digitalWrite(move_jack, Disabled);
      Serial.println("MoveJack OFF");
      Delay(table_back);  //////////////////// 600
      digitalWrite(shablon, Disabled);
      Serial.println("ShabplonJack OFF");
      Delay(ax_back);  //////////////////// 1200
      digitalWrite(jack_heat, Enable);
      Serial.println("HeaterJack ON");
      Delay(heater_ready);  //////////////////// 2100
      digitalWrite(move_jack, Enable);
      Serial.println("MoveJack ON");
      Delay(table_ready);  //////////////////// 1000
      map1 = 4;
      for (eTime = e_up; eTime > 0; eTime--) {
        Delay(1000);
      }
      Delay(190);
      map1 = 5;
      digitalWrite(move_jack, Disabled);
      Serial.println("MoveJack OFF");
      Delay(table_back);  ////////////////////600
      digitalWrite(jack_heat, Disabled);
      Serial.println("HeaterJack OFF");
      Delay(heater_back);  //////////////////// 2100
      digitalWrite(move_jack, Enable);
      Serial.println("MoveJack ON");
      Delay(table_ready);  //////////////////// 1000
      welding_temp = 0;
      for (sTime = s_up; sTime > 0; sTime--) {
        if (sTime <= (s_up - tap_jack_1) && device_mode == MODE_WELDER_SINGLE) {
          digitalWrite(tap_move_1, Enable);
        }
        if (sTime <= (s_up - tap_jack_2) && device_mode == MODE_WELDER_SINGLE) {
          digitalWrite(tap_move_2, Enable);
        }
        Delay(1000);
      }
      Delay(190);
      map1 = 6;
      digitalWrite(jack_left, Disabled);
      Serial.println("leftJack OFF");

      digitalWrite(jack_left_2, Disabled);
      Serial.println("leftJack_2 OFF");

      digitalWrite(tap_move_1, Disabled);
      Serial.println("tap_move_1 OFF");
      digitalWrite(tap_move_2, Disabled);
      Serial.println("tap_move_2 OFF");

      Delay(100);  ////////////////////
      digitalWrite(move_jack, Disabled);
      Serial.println("MoveJack OFF");
      Delay(table_back / 1.5);
      digitalWrite(jack_righ, Disabled);
      Serial.println("righJack OFF");
      digitalWrite(jack_righ_2, Disabled);
      Serial.println("righJack_2 OFF");


      eTime = e_up;
      cTemp = c_up;
      sTime = s_up;
      // end_tone();
      map1 = 0;
      map2 = 0;
    }
  }
  //////////////////////////////
  if (keyNum != 100 && map1 == 0) {
    if (keyNum == F3 && e_up < 99) {
      e_up++;
      if (on_wi_fi) webApp.reqHandShake();
    }

    else if (keyNum == F4 && e_up > 1) {
      e_up--;
      if (on_wi_fi) webApp.reqHandShake();
    }
    ////////////////////////////////
    else if (keyNum == F1 && c_up < 999) {
      c_up++;
      show_temp = millis();
      c_up_copy = c_up + 10;
      c_up_copy_n = c_up - 10;
      if (on_wi_fi) webApp.reqHandShake();
    } else if (keyNum == F2 && c_up > 1) {
      c_up--;
      show_temp = millis();
      c_up_copy = c_up + 10;
      c_up_copy_n = c_up - 10;
      if (on_wi_fi) webApp.reqHandShake();
    }
    //////////////////////////////////////////
    else if (keyNum == F7 && s_up < 99) {
      s_up++;
      if (on_wi_fi) webApp.reqHandShake();
    }

    else if (keyNum == F8 && s_up > 1) {
      s_up--;
      if (on_wi_fi) webApp.reqHandShake();
    }
  } else if (e_up != eTime || s_up != sTime || c_up != cTemp) {
    update_data = true;
    if (c_up != cTemp) {
      temp_dis = temp_dis_2;
      con_temp = true;
    }
  }
  update_lec();
  if (c_up == cTemp) Delay(150);
  Delay(50);
}
void save_var(void) {
  if (update_data == true) {
    Serial.println("%*%*%*%*%*%*%*%* Save EEPROM *%*%*%*%*%*%*%*%");
    update_data = false;
    address = 0;
    check_var[0] = EEPROM.readInt(address);  // e_up

    address = 4;
    check_var[1] = EEPROM.readInt(address);  // c_up

    address = 8;
    check_var[2] = EEPROM.readInt(address);  // s_up

    address = 12;
    check_var[3] = EEPROM.readInt(address);  // PID

    address = 16;
    check_var[4] = EEPROM.readInt(address);  // isJ

    ////////////////////////cleaner /////////////////
    address = 20;
    check_var[5] = EEPROM.readInt(address);  // blade_mode

    address = 24;
    check_var[6] = EEPROM.readInt(address);  // ability

    address = 28;
    check_var[7] = EEPROM.readInt(address);  // motor_type

    address = 32;
    check_var[8] = EEPROM.readInt(address);  // device_mode
    if (check_var[8] > MODES_SIZE || device_mode == 0)
      Mode(1);

    address = 36;
    check_var[9] = EEPROM.readInt(address);  // language

    ////////////////////////time jack /////////////////
    address = 40;
    check_var[10] = EEPROM.readInt(address);  //ax_ready

    address = 44;
    check_var[11] = EEPROM.readInt(address);  // ax_back

    address = 48;
    check_var[12] = EEPROM.readInt(address);  // table_ready

    address = 52;
    check_var[13] = EEPROM.readInt(address);  // table_back

    address = 56;
    check_var[14] = EEPROM.readInt(address);  // heater_ready

    address = 60;
    check_var[15] = EEPROM.readInt(address);  // heater_back

    address = 88;
    check_var[22] = EEPROM.readInt(address);  // press_ready

    address = 92;
    check_var[23] = EEPROM.readInt(address);  // press_back

    address = 96;
    check_var[24] = EEPROM.readInt(address);  // tap_jack_1

    address = 100;
    check_var[25] = EEPROM.readInt(address);  // tap_jack_2

    //////////////////////// lock /////////////////

    address = 64;
    check_var[16] = EEPROM.readInt(address);  // lock_time
    address = 68;
    check_var[17] = EEPROM.readInt(address);  // lock_timeing

    address = 72;
    check_var[18] = EEPROM.readInt(address);  // password
    address = 76;
    check_var[19] = EEPROM.readInt(address);  // lock_en

    address = 80;
    check_var[20] = EEPROM.readInt(address);  // adding_time


    //////////////////////////////// check var //////////////////////////////////////
    bool etntr_eeprom = 0;
    address = 0;
    if (check_var[0] != e_up) {
      EEPROM.writeInt(address, e_up);
      Serial.println("EEPROM write");
      Serial.print("Warm Up Time(S) = ");
      Serial.println(e_up);
      etntr_eeprom = 1;
    }

    address = 4;
    if (check_var[1] != c_up) {
      EEPROM.writeInt(address, c_up);
      Serial.println("EEPROM write");
      Serial.print("Set Point Heater(S) = ");
      Serial.println(c_up);
      etntr_eeprom = 1;
    }

    address = 8;
    if (check_var[2] != s_up) {
      EEPROM.writeInt(address, s_up);
      Serial.println("EEPROM write");
      Serial.print("Cooling Time(S) = ");
      Serial.println(s_up);
      etntr_eeprom = 1;
    }

    address = 12;
    if (check_var[3] != PID) {
      EEPROM.writeInt(address, PID);
      Serial.println("EEPROM write");
      Serial.print("PID = ");
      Serial.println(PID);
      etntr_eeprom = 1;
    }

    address = 16;
    if (check_var[4] != isJ) {
      EEPROM.writeInt(address, isJ);
      Serial.println("EEPROM write");
      Serial.print("Thermocouple = ");
      if (isJ) {
        Serial.println("K");
      } else Serial.println("J");
      etntr_eeprom = 1;
    }

    ////////////////////////cleaner /////////////////
    address = 20;
    if (check_var[5] != blade_mode) {
      EEPROM.writeInt(address, blade_mode);
      Serial.println("EEPROM write");
      Serial.print("blade_mode = ");
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
      etntr_eeprom = 1;
    }

    address = 24;
    if (check_var[6] != ability) {
      EEPROM.writeInt(address, ability);
      Serial.println("EEPROM write");
      Serial.print("ability = ");
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
      etntr_eeprom = 1;
    }

    address = 28;
    if (check_var[7] != motor_type) {
      EEPROM.writeInt(address, motor_type);
      Serial.println("EEPROM write");
      Serial.print("motor_type = ");
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
      etntr_eeprom = 1;
    }

    address = 32;
    if (check_var[8] != device_mode) {
      EEPROM.writeInt(address, device_mode);
      Serial.println("EEPROM write");
      Serial.print("device_mode = ");
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
      etntr_eeprom = 1;
    }

    address = 36;
    if (check_var[9] != language) {
      EEPROM.writeInt(address, language);
      Serial.println("EEPROM write");
      Serial.print("language = ");
      if (language == 1) {
        Serial.println("English");
      } else Serial.println("Turkce");
      etntr_eeprom = 1;
    }

    ////////////////////////time jack /////////////////
    address = 40;
    if (check_var[10] != ax_ready) {
      EEPROM.writeInt(address, ax_ready);
      Serial.println("EEPROM write");
      Serial.print("The Time Of Arrival Of The Ax(S) = ");
      Serial.println(ax_ready);
      etntr_eeprom = 1;
    }

    address = 44;
    if (check_var[11] != ax_back) {
      EEPROM.writeInt(address, ax_back);
      Serial.println("EEPROM write");
      Serial.print("Time To Return The Ax(S) = ");
      Serial.println(ax_back);
      etntr_eeprom = 1;
    }

    address = 48;
    if (check_var[12] != table_ready) {
      EEPROM.writeInt(address, table_ready);
      Serial.println("EEPROM write");
      Serial.print("The Time Of Arrival Of The Table(S) = ");
      Serial.println(table_ready);
      etntr_eeprom = 1;
    }

    address = 52;
    if (check_var[13] != table_back) {
      EEPROM.writeInt(address, table_back);
      Serial.println("EEPROM write");
      Serial.print("Time To Return The Table(S) = ");
      Serial.println(table_back);
      etntr_eeprom = 1;
    }

    address = 56;
    if (check_var[14] != heater_ready) {
      EEPROM.writeInt(address, heater_ready);
      Serial.println("EEPROM write");
      Serial.print("The Time Of Arrival Of The Heater(S) = ");
      Serial.println(heater_ready);
      etntr_eeprom = 1;
    }

    address = 60;
    if (check_var[15] != heater_back) {
      EEPROM.writeInt(address, heater_back);
      Serial.println("EEPROM write");
      Serial.print("Time To Return The Heater(S) = ");
      Serial.println(heater_back);
      etntr_eeprom = 1;
    }

    address = 88;
    if (check_var[22] != press_ready) {
      EEPROM.writeInt(address, press_ready);
      Serial.println("EEPROM write");
      Serial.print("Press Ready (ms) = ");
      Serial.println(press_ready);
      etntr_eeprom = 1;
    }

    address = 92;
    if (check_var[23] != press_back) {
      EEPROM.writeInt(address, press_back);
      Serial.println("EEPROM write");
      Serial.print("Press Back (ms) = ");
      Serial.println(press_back);
      etntr_eeprom = 1;
    }

    address = 96;
    if (check_var[24] != tap_jack_1) {
      EEPROM.writeInt(address, tap_jack_1);
      Serial.println("EEPROM write");
      Serial.print("Tap Jack 1 (S)= ");
      Serial.println(tap_jack_1);
      etntr_eeprom = 1;
    }

    address = 100;
    if (check_var[25] != tap_jack_2) {
      EEPROM.writeInt(address, tap_jack_2);
      Serial.println("EEPROM write");
      Serial.print("Tap Jack 2 (S)= ");
      Serial.println(tap_jack_2);
      etntr_eeprom = 1;
    }
    //////////////////////// lock /////////////////
    address = 64;
    if (check_var[16] != lock_time) {
      EEPROM.writeInt(address, lock_time);
      Serial.println("EEPROM write");
      Serial.print("System Lock Time(DAY) = ");
      Serial.println(lock_time);
      etntr_eeprom = 1;
    }
    address = 68;
    if (check_var[17] != lock_timeing) {
      EEPROM.writeInt(address, lock_timeing);
      Serial.println("EEPROM write");
      Serial.print("Elapsed Time Of System Lock(DAY) = ");
      Serial.println(lock_timeing);
      etntr_eeprom = 1;
    }
    address = 72;
    if (check_var[18] != password) {
      EEPROM.writeInt(address, password);
      Serial.println("EEPROM write");
      Serial.print("Password = ");
      Serial.println(password);
      etntr_eeprom = 1;
    }
    address = 76;
    if (check_var[19] != lock_en) {
      EEPROM.writeInt(address, lock_en);
      Serial.println("EEPROM write");
      Serial.print("System lock = ");
      if (lock_en) {
        Serial.println("Enable");
      } else Serial.println("Disabled");
      etntr_eeprom = 1;
    }

    address = 80;
    if (check_var[20] != adding_time) {
      EEPROM.writeInt(address, adding_time);
      Serial.println("EEPROM write");
      Serial.print("Adding Time = ");
      if (adding_time) {
        Serial.println("Enable");
      } else Serial.println("Disabled");
      etntr_eeprom = 1;
    }

    address = 84;
    if (check_var[21] != hardware_test) {
      EEPROM.writeInt(address, hardware_test);
      Serial.println("EEPROM write");
      Serial.print("Hardware Test = ");
      if (hardware_test) {
        Serial.println("Enable");
      } else Serial.println("Disabled");
      etntr_eeprom = 1;
    }
    if (etntr_eeprom == 1) {
      delay(10);
      EEPROM.commit();
      delay(10);
      Serial.println("EEPROM Commit");
    }
    address = 0;
    eTime = e_up;
    cTemp = c_up;
    sTime = s_up;
    Serial.println("save eeprom");
    Serial.println("%*%*%*%*%*%*%*%* END Of Save EEPROM *%*%*%*%*%*%*%*%");
  }
}
void information(void) {
  int chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.println("════════════ Information ══════════════");
  Serial.println("IDE : V.2.3.3");
  Serial.println("Code : V." + String(ver * 0.01));
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
void display_eeprom(void) {
  Serial.println("#*#*#*#*#*#*#*#* Display EEPROM *#*#*#*#*#*#*#*#");
  Serial.print("Warm Up Time(S) = ");
  Serial.println(e_up);

  Serial.print("Set Point Heater(S) = ");
  Serial.println(c_up);

  Serial.print("Cooling Time(S) = ");
  Serial.println(s_up);

  Serial.print("PID = ");
  Serial.println(PID);

  Serial.print("Thermocouple = ");
  if (isJ) {
    Serial.println("K");
  } else Serial.println("J");


  ////////////////////////cleaner /////////////////
  Serial.print("blade_mode = ");
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

  Serial.print("ability = ");
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

  Serial.print("motor_type = ");
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

  Serial.print("device_mode = ");
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

  Serial.print("language = ");
  if (language == 1) {
    Serial.println("English");
  } else Serial.println("Turkce");


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
  Serial.print("System Lock Time(DAY) = ");
  Serial.println(lock_time);

  Serial.print("Elapsed Time Of System Lock(DAY) = ");
  Serial.println(lock_timeing);

  Serial.print("Password = ");
  Serial.println(password);

  Serial.print("System lock = ");
  if (lock_en) {
    Serial.println("Enable");
  } else Serial.println("Disabled");

  Serial.print("Adding Time = ");
  if (adding_time) {
    Serial.println("Enable");
  } else Serial.println("Disabled");

  Serial.print("Test Hardware = ");
  if (hardware_test) {
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
  if (temperature < 50) {
    Serial.println(temperature);
  } else Serial.println("no temperature");

  Serial.print("Heater Temperature = ");
  Serial.println(temp);

  Serial.print("calculateDays = ");
  Serial.println(calculateDays());

  Serial.println("#*#*#*#*#*#*#*#* END Of EEPROM Display *#*#*#*#*#*#*#*#");
}
void select_function(void) {
  average();
  ////////////////////////////// F1 & F5 ///////////////////////////
  if ((ADC_1 > 380) && (ADC_1 < 430)) {
    delay(3);
    average();

    if ((ADC_2 > 380) && (ADC_2 < 430)) {
      delay(3);
      average();
      if ((ADC_1 > 380) && (ADC_1 < 430)) {
        lcd_disabled = true;
        lcd.setCursor(0, 0);
        lcd.print("    LANGUAGE    ");
        lcd.setCursor(0, 1);
        lcd.print("                ");
        Delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("F1)INGILIZCE    ");
        lcd.setCursor(0, 1);
        lcd.print("F2)TURKCE       ");
        while (1) {
          average();
          if (ADC_1 < 20 && ADC_2 < 20)
            break;
        }
        delay(50);
        while (1) {
          Delay(1);
          if (keyNum == F1) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("    INGILIZCE    ");
            language = english;
            if (on_wi_fi) webApp.reqHandShake();
            break;
          }

          if (keyNum == F2) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("     TURKCE      ");
            language = turkce;
            if (on_wi_fi) webApp.reqHandShake();
            break;
          }
        }
        update_data = true;
        Delay(1000);
        lcd_disabled = false;
        update_lec();
      }
    }
  }
  ////////////////////////////// F2 & F6 ///////////////////////////
  if ((ADC_1 > 200) && (ADC_1 < 270)) {
    delay(3);
    average();

    if ((ADC_2 > 200) && (ADC_2 < 270)) {
      delay(3);
      average();
      if ((ADC_1 > 200) && (ADC_1 < 270)) {
        lcd_disabled = true;
        lcd.setCursor(0, 0);
        if (language == english)
          lcd.print(" Time For Jacks ");
        if (language == turkce)
          lcd.print("JaklarIn vaxtIdIr");

        lcd.setCursor(0, 1);
        lcd.print("                ");
        delay(1000);

        lcd.setCursor(0, 0);
        lcd.print("F1)+     ");
        lcd.setCursor(0, 1);
        lcd.print("F2)-     ");

        lcd.setCursor(9, 0);
        lcd.print("F8)Test");

        lcd.setCursor(9, 1);
        if (language == english)
          lcd.print("F7)Next");
        if (language == turkce) {
          lcd.setCursor(6, 1);
          lcd.print("F7)novbati");
        }
        delay(3000);

        lcd.setCursor(0, 0);
        if (language == english)
          lcd.print("    F5)Save     ");
        if (language == turkce)
          lcd.print("   F5)magaza    ");

        lcd.setCursor(0, 1);
        if (language == english)
          lcd.print("    F6)Cancel   ");
        if (language == turkce)
          lcd.print("  F6)lagv rdin  ");

        delay(2000);

        lcd.setCursor(0, 0);
        lcd.print("A");
        lcd.print(ax_ready * 0.001);

        lcd.setCursor(0, 1);
        lcd.print("A");
        lcd.print(ax_back * 0.001);

        lcd.setCursor(4, 0);
        lcd.print(" P:");
        lcd.print(press_ready * 0.001);

        lcd.setCursor(4, 1);
        lcd.print(" P:");
        lcd.print(press_back * 0.001);

        lcd.setCursor(10, 0);
        lcd.print(" T:");
        lcd.print(table_ready * 0.001);

        lcd.setCursor(10, 1);
        lcd.print(" T:");
        lcd.print(table_back * 0.001);

        lcd.setCursor(4, 0);
        lcd.print("<");

        while (1) {
          average();
          if (ADC_1 < 20 && ADC_2 < 20)
            break;
        }
        delay(100);
        int step_mode = 0;
        bool p_stat_f1 = 0, p_stat_f2 = 0, p_stat_f7 = 0;
        bool refresh_lcd = 0;
        while (1) {
          Delay(20);
          if (keyNum == F7) {
            if (p_stat_f7 == 0) {
              p_stat_f7 = 1;
              refresh_lcd = 1;
              if (step_mode < 9) {
                step_mode++;
              } else step_mode = 0;
            }
          } else p_stat_f7 = 0;

          if (keyNum == F1) {
            if (p_stat_f1 == 0) {
              p_stat_f1 = 1;
              refresh_lcd = 1;
              if (step_mode == 0 && ax_ready < 9900) ax_ready = ax_ready + 100;
              if (step_mode == 1 && ax_back < 9900) ax_back = ax_back + 100;

              if (step_mode == 2 && press_ready < 9900) press_ready = press_ready + 100;
              if (step_mode == 3 && press_back < 9900) press_back = press_back + 100;

              if (step_mode == 4 && table_ready < 9900) table_ready = table_ready + 100;
              if (step_mode == 5 && table_back < 9900) table_back = table_back + 100;

              if (step_mode == 6 && heater_ready < 9900) heater_ready = heater_ready + 100;
              if (step_mode == 7 && heater_back < 9900) heater_back = heater_back + 100;

              if (step_mode == 8 && tap_jack_1 < s_up) tap_jack_1++;
              if (step_mode == 9 && tap_jack_2 < s_up) tap_jack_2++;
            }
          } else p_stat_f1 = 0;

          if (keyNum == F2) {
            if (p_stat_f2 == 0) {
              p_stat_f2 = 1;
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
            }
          } else p_stat_f2 = 0;

          if (refresh_lcd == 1) {
            refresh_lcd = 0;
            if (step_mode < 6) {
              lcd.setCursor(0, 0);
              lcd.print("A");
              lcd.print(ax_ready * 0.001);

              lcd.setCursor(0, 1);
              lcd.print("A");
              lcd.print(ax_back * 0.001);

              lcd.setCursor(4, 0);
              lcd.print(" P:");
              lcd.print(press_ready * 0.001);

              lcd.setCursor(4, 1);
              lcd.print(" P:");
              lcd.print(press_back * 0.001);

              lcd.setCursor(10, 0);
              lcd.print(" T:");
              lcd.print(table_ready * 0.001);

              lcd.setCursor(10, 1);
              lcd.print(" T:");
              lcd.print(table_back * 0.001);
            } else {
              lcd.setCursor(0, 0);
              lcd.print("H");
              lcd.print(heater_ready * 0.001);

              lcd.setCursor(0, 1);
              lcd.print("H");
              lcd.print(heater_back * 0.001);

              lcd.setCursor(4, 0);
              lcd.print(" T1:         ");
              lcd.setCursor(8, 0);
              lcd.print(tap_jack_1);

              lcd.setCursor(4, 1);
              lcd.print(" T2:         ");
              lcd.setCursor(8, 1);
              lcd.print(tap_jack_2);
            }

            if (step_mode == 0 || step_mode == 6) {
              lcd.setCursor(4, 0);
              lcd.print("<");
            }
            if (step_mode == 1 || step_mode == 7) {
              lcd.setCursor(4, 1);
              lcd.print("<");
            }

            if (step_mode == 2 || step_mode == 8) {
              lcd.setCursor(10, 0);
              lcd.print("<");
            }
            if (step_mode == 3 || step_mode == 9) {
              lcd.setCursor(10, 1);
              lcd.print("<");
            }

            if (step_mode == 4) {
              lcd.setCursor(10, 0);
              lcd.print(">");
            }
            if (step_mode == 5) {
              lcd.setCursor(10, 1);
              lcd.print(">");
            }

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

          if (keyNum == F8) {
            lcd.setCursor(0, 0);
            lcd.print("A");
            lcd.print(ax_ready * 0.001);

            lcd.setCursor(0, 1);
            lcd.print("A");
            lcd.print(ax_back * 0.001);

            lcd.setCursor(4, 0);
            lcd.print(" T:");
            lcd.print(table_ready * 0.001);

            lcd.setCursor(4, 1);
            lcd.print(" T:");
            lcd.print(table_back * 0.001);

            lcd.setCursor(10, 0);
            lcd.print(" H:");
            lcd.print(heater_ready * 0.001);

            lcd.setCursor(10, 1);
            lcd.print(" H:");
            lcd.print(heater_back * 0.001);

            lcd.setCursor(4, 0);
            lcd.print(" ");
            lcd.setCursor(10, 0);
            lcd.print(" ");

            lcd.setCursor(4, 1);
            lcd.print(" ");
            lcd.setCursor(10, 1);
            lcd.print(" ");
            digitalWrite(shablon, Enable);
            Serial.println("shablon ON");
            for (int i = ax_ready; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(0, 0);
              lcd.print("A");
              lcd.print(i * 0.001);
              lcd.setCursor(4, 0);
              lcd.print(" T:");
            }
            digitalWrite(move_jack, Enable);
            Serial.println("move_jack ON");
            for (int i = table_ready; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(4, 0);
              lcd.print(" T:");
              lcd.print(i * 0.001);
              lcd.setCursor(10, 0);
              lcd.print(" H:");
            }
            digitalWrite(move_jack, Disabled);
            Serial.println("move_jack OFF");
            for (int i = table_back; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(4, 1);
              lcd.print(" T:");
              lcd.print(i * 0.001);
              lcd.setCursor(10, 1);
              lcd.print(" H:");
            }
            digitalWrite(shablon, Disabled);
            Serial.println("shablon OFF");
            for (int i = ax_back; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(0, 1);
              lcd.print("A");
              lcd.print(i * 0.001);
              lcd.setCursor(4, 1);
              lcd.print(" T:");
            }
            digitalWrite(jack_heat, Enable);
            Serial.println("move_jack ON");
            for (int i = heater_ready; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(10, 0);
              lcd.print(" H:");
              lcd.print(i * 0.001);
            }
            digitalWrite(move_jack, Enable);
            Serial.println("move_jack ON");
            for (int i = table_ready; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(4, 0);
              lcd.print(" T:");
              lcd.print(i * 0.001);
              lcd.setCursor(10, 0);
              lcd.print(" H:");
            }
            digitalWrite(move_jack, Disabled);
            Serial.println("move_jack OFF");
            for (int i = table_back; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(4, 1);
              lcd.print(" T:");
              lcd.print(i * 0.001);
              lcd.setCursor(10, 1);
              lcd.print(" H:");
            }
            digitalWrite(jack_heat, Disabled);
            Serial.println("move_jack OFF");
            for (int i = heater_back; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(10, 1);
              lcd.print(" H:");
              lcd.print(i * 0.001);
            }

            digitalWrite(jack_left, Enable);
            digitalWrite(jack_righ, Enable);
            Serial.println("Jack Press ON");
            for (int i = press_ready; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(0, 0);
              lcd.print("P");
              lcd.print(i * 0.001);
              lcd.setCursor(4, 0);
              lcd.print(" T:");
            }
            digitalWrite(jack_left, Disabled);
            digitalWrite(jack_righ, Disabled);
            Serial.println("Jack Press OFF");
            for (int i = press_back; i >= 0; i = i - 100) {
              delay(100);
              lcd.setCursor(0, 1);
              lcd.print("P");
              lcd.print(i * 0.001);
              lcd.setCursor(4, 1);
              lcd.print(" T:");
            }

            refresh_lcd = 1;
          }

          if (keyNum == F5) {
            lcd.clear();
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("      Save      ");
            if (language == turkce)
              lcd.print("     magaza     ");
            break;
          }
        }
        setupPin();
        update_data = true;
        Delay(1000);
        lcd_disabled = false;
        update_lec();
      }
    }
  }
  ////////////////////////////// F3 & F7 ///////////////////////////
  if ((ADC_1 > 150) && (ADC_1 < 200)) {
    delay(3);
    average();

    if ((ADC_2 > 150) && (ADC_2 < 200)) {
      delay(3);
      average();
      if ((ADC_1 > 150) && (ADC_1 < 200)) {
        lcd_disabled = true;
        lcd.setCursor(0, 0);
        if (language == english)
          lcd.print("F1)WELDING  MACH");
        if (language == turkce)
          lcd.print("F1)KAYNAK   MAKN");
        lcd.setCursor(0, 1);
        if (language == english)
          lcd.print("F2)CLEANING MACH");
        if (language == turkce)
          lcd.print("F2)KOSE TEM MAKN");

        //          lcd.setCursor(9, 1);
        //          if(language == english) lcd.print("D Met(4");
        while (1) {
          average();
          if (ADC_1 < 20 && ADC_2 < 20)
            break;
        }
        delay(50);
        while (1) {
          Delay(1);
          if (keyNum == F1) {
            lcd.clear();
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("F1)SINGEL   HEAD");
            if (language == turkce)
              lcd.print("F1)TEKLI  KAYNAK");
            lcd.setCursor(0, 1);
            if (language == english)
              lcd.print("F2)DOUBLE   HEAD");
            if (language == turkce)
              lcd.print("F2)CIFT   KAYNAK");
            while (1) {
              average();
              if (ADC_1 < 20 && ADC_2 < 20)
                break;
            }
            Delay(50);
            while (1) {
              Delay(20);
              if (keyNum == F1) {
                device_mode = MODE_WELDER_SINGLE;
                break;
              }
              if (keyNum == F2) {
                device_mode = MODE_WELDER_DOUBLE;
                break;
              }
            }
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("F1)+ F2)- PID:  ");
            lcd.setCursor(14, 0);
            lcd.print(PID);
            lcd.setCursor(0, 1);
            if (language == english)
              lcd.print("F3)TM:   F5)Save");
            if (language == turkce)
              lcd.print("F3)TM:  F5)MAGAZA");
            lcd.setCursor(6, 1);
            if (isJ == 1)
              lcd.print("K");
            if (isJ == 0)
              lcd.print("J");
            int PID_copy = PID;
            int isJ_copy = isJ;
            while (1) {
              average();
              if (ADC_1 < 20 && ADC_2 < 20)
                break;
            }
            Delay(50);
            while (1) {
              Delay(1);
              if (keyNum == F1) {
                if (PID < 100)
                  PID++;
              }
              if (keyNum == F2) {
                if (PID > 0)
                  PID--;
              }
              if (PID_copy != PID) {
                if (on_wi_fi) webApp.reqHandShake();
                if (PID < 10) {
                  lcd.setCursor(15, 0);
                  lcd.print(" ");
                }
                lcd.setCursor(14, 0);
                lcd.print(PID);
                delay(250);
                PID_copy = PID;
              }
              if (keyNum == F3)
                isJ = !isJ;

              if (isJ_copy != isJ) {
                isJ_copy = isJ;
                if (on_wi_fi) webApp.reqHandShake();
                lcd.setCursor(6, 1);
                if (isJ == 1)
                  lcd.print("K");
                if (isJ == 0)
                  lcd.print("J");
                delay(400);
              }
              if (keyNum == F5) {
                lcd.clear();
                lcd.setCursor(0, 0);
                if (language == english)
                  lcd.print("    Code Run    ");
                if (language == turkce)
                  lcd.print("   KOD YURUTME  ");
                delay(1000);
                break;
              }
            }
            break;
          }
          Delay(1);
          if (keyNum == F2) {
            device_mode = MODE_CLEANER;
            lcd.clear();
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("    Cleaning     ");
            if (language == turkce)
              lcd.print("   KOSE TEM MAKN  ");
            delay(1000);
            break;
          }
          if (0)  // key_mode == F3)
          {
            device_mode = DIGITAL_METER_MODE;
            lcd.clear();
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("  DIGITAL METER  ");
            if (language == turkce)
              lcd.print("  DIJITAL SAYAC  ");
            delay(1000);
            break;
          }

          if (keyNum == F5) {
            lcd.clear();
            lcd.setCursor(0, 0);
            if (device_mode == MODE_WELDER_SINGLE) {
              if (language == english) {
                lcd.print("    WELDING     ");
                lcd.setCursor(0, 1);
                lcd.print("  SINGEL HEAD   ");
              }
              if (language == turkce) {
                lcd.print(" KOSE TEM MAKN  ");
                lcd.setCursor(0, 1);
                lcd.print("  TEKLI  KAYNAK ");
              }
            }
            if (device_mode == MODE_WELDER_DOUBLE) {
              if (language == english) {
                lcd.print("    WELDING     ");
                lcd.setCursor(0, 1);
                lcd.print(" CIFT   KAYNAK  ");
              }
              if (language == turkce) {
                lcd.print(" KOSE TEM MAKN  ");
                lcd.setCursor(0, 1);
                lcd.print("  DOUBLE HEAD   ");
              }
            }
            if (device_mode == MODE_CLEANER) {
              if (language == english) {
                lcd.print("    CLEANING    ");
                lcd.setCursor(0, 1);
                lcd.print("  FOUR BLADES   ");
              }
              if (language == turkce) {
                lcd.print(" KOSE TEM MAKN  ");
                lcd.setCursor(0, 1);
                lcd.print("   DORT BICAK   ");
              }
            }
            delay(1000);
            break;
          }
        }
      }
      update_data = true;
      lcd_disabled = false;
      update_f();
    }
  }
  ////////////////////////////// F4 & F8 ///////////////////////////
  if ((ADC_1 > 60) && (ADC_1 < 130)) {
    delay(3);
    average();

    if ((ADC_2 > 60) && (ADC_2 < 130)) {
      delay(3);
      average();
      if ((ADC_1 > 60) && (ADC_1 < 130)) {
        lcd_disabled = true;
        if (language == english) {
          lcd.setCursor(0, 0);
          lcd.print("RST The Factory ");
          lcd.setCursor(0, 1);
          lcd.print("F5)Yes     F8)NO");
        }
        if (language == turkce) {
          lcd.setCursor(0, 0);
          lcd.print("Fabrik   qayidin");
          lcd.setCursor(0, 1);
          lcd.print("F5)Beli   F8)Yox");
        }
        Delay(1000);
        while (1) {
          average();
          if (ADC_1 < 20 && ADC_2 < 20)
            break;
        }
        delay(50);
        bool set_time = 0;
        while (1) {
          Delay(1);
          if (keyNum == F5) {
            address = 0;
            e_up = 16;
            c_up = 235;
            s_up = 16;
            PID = 7;
            isJ = false;
            blade_mode = 1;
            ability = 1;
            motor_type = 1;
            device_mode = Mode(1);
            language = english;
            ax_ready = 1200;
            ax_back = 1200;
            table_ready = 1000;
            table_back = 600;
            heater_ready = 2100;
            heater_back = 2100;
            press_ready = 600;
            press_back = 600;
            tap_jack_1 = 3;
            tap_jack_2 = 6;

            EEPROM.writeInt(address, e_up);
            address += sizeof(int);
            EEPROM.writeInt(address, c_up);
            address += sizeof(int);
            EEPROM.writeInt(address, s_up);
            address += sizeof(int);
            EEPROM.writeInt(address, PID);
            address += sizeof(int);
            EEPROM.writeInt(address, isJ);
            address += sizeof(int);
            EEPROM.writeInt(address, blade_mode);
            address += sizeof(int);
            EEPROM.writeInt(address, ability);
            address += sizeof(int);
            EEPROM.writeInt(address, motor_type);
            address += sizeof(int);
            EEPROM.writeInt(address, int(device_mode));
            address += sizeof(int);
            EEPROM.writeInt(address, language);

            address += sizeof(int);
            EEPROM.writeInt(address, ax_ready);
            address += sizeof(int);
            EEPROM.writeInt(address, ax_back);
            address += sizeof(int);
            EEPROM.writeInt(address, table_ready);
            address += sizeof(int);
            EEPROM.writeInt(address, table_back);
            address += sizeof(int);
            EEPROM.writeInt(address, heater_ready);
            address += sizeof(int);
            EEPROM.writeInt(address, heater_back);
            address = 0;
            delay(10);
            EEPROM.commit();
            delay(10);
            Serial.println("EEPROM first");

            lcd.clear();
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("     Success    ");
            if (language == turkce)
              lcd.print("      Ugur      ");
            Delay(1500);
            lcd.clear();
            break;
          }
          if (keyNum == F8) {
            lcd.clear();
            if (val_ds3231 == 0) {
              lcd.setCursor(0, 0);
              lcd.print("     NO RTC");
              delay(1000);
            } else {
              lcd.setCursor(0, 0);
              lcd.print("F1)+     ");
              lcd.setCursor(0, 1);
              lcd.print("F2)-     ");

              lcd.setCursor(9, 0);
              lcd.print("F5)OK");

              lcd.setCursor(9, 1);
              if (language == english)
                lcd.print("F7)Next");
              if (language == turkce) {
                lcd.setCursor(6, 1);
                lcd.print("F7)Novbati");
              }
              Delay(5000);
              lcd.clear();
              set_time = 1;
            }
            break;
          }
        }

        if (set_time == 1) {
          int step_mode = 0, password_copy = 1000000, password_copy_2 = 0;
          int step_lock = 0, step_pass = 0;
          bool p_stat_f1 = 0, p_stat_f2 = 0, p_stat_f7 = 0;
          bool refresh_lcd = 1;
          while (1) {
            Delay(10);
            if (keyNum == F7) {
              if (p_stat_f7 == 0) {
                p_stat_f7 = 1;
                refresh_lcd = 1;
                if (step_mode < 5) {
                  step_mode++;
                } else step_mode = 0;
              }
            } else p_stat_f7 = 0;

            if (keyNum == F1) {
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

            if (keyNum == F2) {
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

            if (refresh_lcd == 1) {
              refresh_lcd = 0;

              if (step_pass == 0) {
                lcd.setCursor(0, 1);
                lcd.print("F1)+ F2)-       ");
                lcd.setCursor(step_mode + 10, 1);
                lcd.print("^");
                lcd.setCursor(9, 0);
                lcd.print(password_copy);
                lcd.setCursor(0, 0);
              }
              if (step_pass == 1) {
                lcd.setCursor(0, 0);
                if (language == english)
                  lcd.print("New PIN         ");
                if (language == turkce)
                  lcd.print("YeniParol       ");
                lcd.setCursor(step_mode + 10, 0);
                lcd.print("v");
                lcd.setCursor(9, 1);
                lcd.print(password_copy);
                lcd.setCursor(0, 1);
              }
              if (step_pass == 2) {
                lcd.setCursor(0, 0);
                if (language == english)
                  lcd.print("Confirm         ");
                if (language == turkce)
                  lcd.print("Tesdiqle        ");
                lcd.setCursor(step_mode + 10, 0);
                lcd.print("v");
                lcd.setCursor(9, 1);
                lcd.print(password_copy);
                lcd.setCursor(0, 1);
              }

              if (language == english)
                lcd.print("Password :");
              if (language == turkce)
                lcd.print("   Parol :");
            }
            if (keyNum == F5) {
              if (step_pass == 1) {
                step_pass = 2;
                step_mode = 0;
                password_copy_2 = password_copy;
                lcd.clear();
                lcd.setCursor(0, 0);
                if (language == english)
                  lcd.print("Confirm Password");
                if (language == turkce)
                  lcd.print("Kecmeyi Tesdiqle");
                delay(1500);
                refresh_lcd = 1;
                password_copy = 1000000;
                lcd.clear();

              } else if (step_pass == 2) {
                lcd.clear();
                lcd.setCursor(0, 0);
                if (password_copy == password_copy_2) {
                  if (language == english)
                    lcd.print("Pass : Confirmed");
                  if (language == turkce)
                    lcd.print("Tesdiqle Kecmeyi");
                  step_pass = 3;
                  password = password_copy;
                  EEPROM.writeInt(72, password);
                  delay(10);
                  EEPROM.commit();
                  delay(1500);
                  lcd.clear();
                } else {
                  if (language == english)
                    lcd.print("Password : False");
                  if (language == turkce)
                    lcd.print(" Parol : Yalan  ");
                  delay(1500);
                  refresh_lcd = 1;
                  step_pass = 1;
                  step_mode = 0;
                  password_copy = 1000000;
                  lcd.setCursor(0, 0);
                  if (language == english)
                    lcd.print("New Password:ENT");
                  if (language == turkce)
                    lcd.print("YeniParol:DaxilE");
                  delay(1500);
                  lcd.clear();
                }
              } else if (password_copy == password || password_copy == PasswordAmin) {
                step_lock = 0;
                step_mode = 0;
                lcd.clear();
                lcd.setCursor(0, 0);
                if (language == english)
                  lcd.print("Password :  True");
                if (language == turkce)
                  lcd.print("  Parol : Sag   ");
                delay(1500);
                if (password != 1111111) {
                  lcd.setCursor(0, 0);
                  if (language == english) {
                    lcd.print("Password :Change");
                    lcd.setCursor(0, 1);
                    lcd.print("F5)Yes     F8)NO");
                  }
                  if (language == turkce) {
                    lcd.print("Parol:deyisikliy");
                    lcd.setCursor(0, 1);
                    lcd.print("F5)Beli   F8)Yox");
                  }
                  delay(400);
                  while (1) {
                    Delay(1);
                    if (keyNum == F5) {
                      password = 1111111;
                      break;
                    }
                    if (keyNum == F8) break;
                  }
                }
                if (password == 1111111) {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  if (language == english)
                    lcd.print("New Password:ENT");
                  if (language == turkce)
                    lcd.print("YeniParo:DaxilEd");
                  delay(1500);
                  refresh_lcd = 1;
                  step_pass = 1;
                  password_copy = 1000000;
                } else step_pass = 3;
                lcd.clear();
              } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                if (language == english)
                  lcd.print("Password : False");
                if (language == turkce)
                  lcd.print(" Parol : Yalan  ");
                delay(1500);
                refresh_lcd = 1;
                step_mode = 0;
                step_lock++;
                if (step_lock == 4) {
                  lcd.setCursor(0, 0);
                  if (language == english)
                    lcd.print("System is Locked");
                  if (language == turkce)
                    lcd.print("SistemKilidlenib");
                  delay(60000);
                  lcd.clear();
                  step_lock = 0;
                }
              }
              if (step_pass == 3) {
                lcd.setCursor(0, 0);
                if (language == english) {
                  lcd.print("Active The Timer");
                  lcd.setCursor(0, 1);
                  lcd.print("F5)Yes     F8)NO");
                }
                if (language == turkce) {
                  lcd.print("Taymeri Aktivles");
                  lcd.setCursor(0, 1);
                  lcd.print("F5)Beli   F8)Yox");
                }
                while (1) {
                  Delay(1);
                  if (keyNum == F5) {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    if (language == english) {
                      lcd.print("Duration in Daye");
                      lcd.setCursor(0, 1);
                      lcd.print("N.O Days : 0");
                    }
                    if (language == turkce) {
                      lcd.print("Muddet  gunlerle");
                      lcd.setCursor(0, 1);
                      lcd.print("Gunler   : 0");
                    }
                    Delay(400);
                    Serial.println("enter time");
                    break;
                  } else if (keyNum == F8) break;
                }
                if (keyNum == F8) {
                  lock_time = 0;
                  lock_en = false;
                  adding_time = 0;
                  EEPROM.writeInt(64, lock_time);
                  EEPROM.writeInt(76, lock_en);
                  EEPROM.writeInt(80, adding_time);
                  lock_timeing = calculateDays();     //////////////////RTC ////////////
                  EEPROM.writeInt(68, lock_timeing);  ///////////////
                  Serial.println("Deactive Timer");
                  Serial.print("Day : ");
                  Serial.println(lock_timeing);
                  delay(10);
                  EEPROM.commit();
                  delay(10);
                  break;
                }
                int days_unit = 0;
                Serial.println("enter Day");
                while (1) {
                  Delay(200);
                  if (keyNum == F1 && days_unit < 200) {
                    days_unit++;
                    Serial.print("Day = ");
                    Serial.println(days_unit);
                  }
                  if (keyNum == F2 && days_unit > 0) {
                    days_unit--;
                    Serial.print("Day = ");
                    Serial.println(days_unit);
                  }
                  if (days_unit < 10) {
                    lcd.setCursor(12, 1);
                    lcd.print("    ");
                  } else if (days_unit >= 10) {
                    lcd.setCursor(13, 1);
                    lcd.print("   ");
                  } else if (days_unit >= 100) {
                    lcd.setCursor(14, 1);
                    lcd.print("  ");
                  }
                  lcd.setCursor(11, 1);
                  lcd.print(days_unit);

                  if (keyNum == F5) {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    if (language == english)
                      lcd.print("     Success    ");
                    if (language == turkce)
                      lcd.print("      Ugur      ");

                    lcd.setCursor(6, 1);
                    lcd.print("D: ");
                    lcd.setCursor(9, 1);
                    lcd.print(days_unit);
                    lock_timeing = calculateDays();
                    lock_time = lock_timeing + days_unit;
                    lock_en = true;
                    adding_time = 0;
                    EEPROM.writeInt(64, lock_time);
                    EEPROM.writeInt(76, lock_en);
                    EEPROM.writeInt(68, lock_timeing);  //////////////////RTC ////////////
                    EEPROM.writeInt(80, 0);

                    Serial.println("Active Timer");
                    Serial.print("Day : ");
                    Serial.println(lock_timeing);
                    Serial.print("END Time : ");
                    Serial.println(lock_time);
                    Serial.print("Step Day : ");
                    Serial.println(days_unit);
                    delay(10);
                    EEPROM.commit();
                    delay(10);
                    break;
                  }
                }
                break;
              }
            }
          }
          Delay(1000);
          lcd_disabled = false;
          update_lec();
        }
      }
    }
  }

  if (device_mode == MODE_WELDER_SINGLE && 0) {  /////////////
    while (1) {
      if (on_wi_fi == true) webApp.Loop();
      if (temp <= 1000 || scan_key() == F6 || read_key(Stop)) {
        break;
      } else {
        lcd.setCursor(0, 0);
        if (language == english)
          lcd.print("!HEATING  ERROR!");
        if (language == turkce)
          lcd.print("!ISITMADA  HATA!");
        delay(100);
      }
      delay(1);
    }
  } else if (device_mode == MODE_CLEANER) {
    delay(500);
    setupPin();
    while (1) {
      webApp.Loop();
      if ((read_key(mic_sw_blade) == 1 && read_key(mic_sw_motor) == 0) || F6 == scan_key() || read_key(Stop) || device_mode != MODE_CLEANER) {
        break;
      } else {
        if (language == english) {
          lcd.setCursor(0, 0);
          lcd.print("SCRAPER SWC  OFF");
          lcd.setCursor(0, 1);
          lcd.print("  CHECK PLEASE  ");
        } else if (language == turkce) {
          lcd.setCursor(0, 0);
          lcd.print("KAZIMA  SWC ACIK");
          lcd.setCursor(0, 1);
          lcd.print(" NONTROL EDINAZ ");
        }
      }
      delay(100);
    }
  }
}
void average(void) {
  for (int i = 0; i < 25; i++) {
    ADC_1 = ADC_1 + analogRead(ADC_KEY_1);
    ADC_2 = ADC_2 + analogRead(ADC_KEY_2);
    delayMicroseconds(3);
  }
  ADC_1 = ADC_1 / 250;
  ADC_2 = ADC_2 / 250;
}
int scan_key(void) {
  average();
  // 90    //180  //250  //400
  // 413   234  176  92

  if ((ADC_2 > 200) && (ADC_2 < 270)) {
    delay(3);
    average();
    if ((ADC_2 > 200) && (ADC_2 < 270)) {
      return 6;
    }
  }

  if ((ADC_1 > 60) && (ADC_1 < 130)) {
    delay(3);
    average();
    if ((ADC_1 > 60) && (ADC_1 < 130)) {
      return 4;
    }
  }

  if ((ADC_1 > 150) && (ADC_1 < 200)) {
    delay(3);
    average();
    if ((ADC_1 > 150) && (ADC_1 < 200)) {
      return 3;
    }
  }

  if ((ADC_1 > 200) && (ADC_1 < 270)) {
    delay(3);
    average();
    if ((ADC_1 > 200) && (ADC_1 < 270)) {
      return 2;
    }
  }

  if ((ADC_1 > 380) && (ADC_1 < 430)) {
    delay(3);
    average();
    if ((ADC_1 > 380) && (ADC_1 < 430)) {
      return 1;
    }
  }
  /////////////////////////////////////////////////////
  if ((ADC_2 > 60) && (ADC_2 < 130)) {
    delay(3);
    average();
    if ((ADC_2 > 60) && (ADC_2 < 130)) {
      return 8;
    }
  }

  if ((ADC_2 > 150) && (ADC_2 < 200)) {
    delay(3);
    average();
    if ((ADC_2 > 150) && (ADC_2 < 200)) {
      return 7;
    }
  }

  if ((ADC_2 > 380) && (ADC_2 < 430)) {
    delay(3);
    average();
    if ((ADC_2 > 380) && (ADC_2 < 430)) {
      return 5;
    }
  }
  return 100;
}
void Delay(int step_delay) {
  unsigned long int delay_ms = millis() + step_delay;
  while (1) {
    esp_task_wdt_reset();
    update_f();
    if (millis() > delay_ms)
      break;
  }
}
void update_f(void) {
  if (analogRead(ADC_KEY_1) > 100 || analogRead(ADC_KEY_2) > 100) {
    keyNum = scan_key();
  } else
    keyNum = 100;

  save_var();

  if (digitalRead(Stop) == 0 || keyNum == F6) {
    if (read_key(Stop) == 1 || keyNum == F6) {
      EMR();
    }
  }
  if (on_wi_fi)
    webApp.Loop();
  int step_time_update = 500;
  if (map1 > 3)
    step_time_update = 200;

  if (millis() > time_u_lcd + step_time_update) {
    if (lcd_disabled == false)
      update_lec();
    time_u_lcd = millis();
  }

  if (keyNum < 100) {
    Serial.println(keyNum);
    delay(100);
  }
}
void update_lec(void) {
  if (device_mode == MODE_WELDER_SINGLE || device_mode == MODE_WELDER_DOUBLE) {
    lcd.setCursor(0, 0);
    lcd.print("E:");

    if (e_up < 10) {
      lcd.setCursor(3, 0);
      lcd.print("  ");
    } else {
      lcd.setCursor(4, 0);
      lcd.print(" ");
    }
    lcd.setCursor(2, 0);
    lcd.print(e_up);

    lcd.setCursor(5, 0);
    lcd.print("C:");

    if (c_up < 10) {
      lcd.setCursor(8, 0);
      lcd.write(byte(0));
      lcd.setCursor(9, 0);
      lcd.print("  ");
    } else if (c_up < 100) {
      lcd.setCursor(9, 0);
      lcd.write(byte(0));
      lcd.setCursor(10, 0);
      lcd.print(" ");
    } else {
      lcd.setCursor(10, 0);
      lcd.write(byte(0));
    }
    if (map1 == 0) {
      lcd.setCursor(11, 0);
      lcd.print(" ");
    } else {
      lcd.setCursor(11, 0);
      lcd.write("!");
    }

    lcd.setCursor(7, 0);
    lcd.print(c_up);

    lcd.setCursor(12, 0);
    lcd.print("S:");

    if (s_up < 10) {
      lcd.setCursor(15, 0);
      lcd.print(" ");
    }
    lcd.setCursor(14, 0);
    lcd.print(s_up);

    ////////////////////////////////DOWN
    lcd.setCursor(0, 1);
    lcd.print("E:");

    if (eTime < 10) {
      lcd.setCursor(3, 1);
      lcd.print("  ");
    } else {
      lcd.setCursor(4, 1);
      lcd.print(" ");
    }
    lcd.setCursor(2, 1);
    lcd.print(eTime);

    lcd.setCursor(5, 1);
    lcd.print("C:");

    lcd.setCursor(11, 1);
    lcd.print(" ");

    lcd.setCursor(7, 1);
    if (temp < 1000) {
      if (temp <= 0) {
        show_temp = -32000;
      }
      if ((3 + temp >= c_up_copy) && (temp <= c_up_copy_n + 3)) {
        temp_dis = c_up;
        temp_dis_2 = c_up;
        lcd.print(temp_dis);
        con_temp = true;
        c_up_copy = c_up;
        c_up_copy_n = c_up;
        show_temp = millis();
      } else if (millis() > show_temp + 32000) {
        temp_dis = temp;
        temp_dis_2 = temp;
        lcd.print(temp_dis);
        con_temp = false;
        c_up_copy = c_up + 3;
        c_up_copy_n = c_up - 3;
      } else if (con_temp == true) {
        c_up_copy = c_up + 3;
        c_up_copy_n = c_up - 3;
        float stpe_set = millis() - show_temp;
        stpe_set = stpe_set / 32000;
        stpe_set = round(temp_dis - stpe_set * (temp_dis - temp));
        temp_dis_2 = stpe_set;
        lcd.print(temp_dis_2);
      }
    } else {
      lcd.print("NO");
      temp_dis = 1000;
    }
    if (temp_copy != temp_dis_2) {
      temp_copy = temp_dis_2;
    }

    if (temp_dis < 10) {
      lcd.setCursor(8, 1);
      lcd.write(byte(0));
      lcd.setCursor(9, 1);
      lcd.print("  ");
    } else if (temp_dis < 100 || temp >= 1000) {
      lcd.setCursor(9, 1);
      lcd.write(byte(0));
      lcd.setCursor(10, 1);
      lcd.print(" ");
    } else {
      lcd.setCursor(10, 1);
      lcd.write(byte(0));
    }

    lcd.setCursor(12, 1);
    lcd.print("S:");

    if (sTime < 10) {
      lcd.setCursor(15, 1);
      lcd.print(" ");
    }
    lcd.setCursor(14, 1);
    lcd.print(sTime);
  } else if (device_mode == MODE_CLEANER) {
    lcd.setCursor(0, 0);
    if (language == turkce)
      lcd.print("ISLEM:");
    if (language == english)
      lcd.print("OPER :");
    lcd.setCursor(6, 0);
    switch (ability) {
      case 1:
        if (language == english)
          lcd.print("FULL    ");
        break;

      case 2:
        if (language == english)
          lcd.print("SETTING ");
        if (language == turkce)
          lcd.print("AYAR    ");
        break;

      case 3:
        if (language == english)
          lcd.print("SCRAPE  ");
        if (language == turkce)
          lcd.print("YUZEY   ");
        break;

      case 4:
        if (language == english)
          lcd.print("MILLING ");
        if (language == turkce)
          lcd.print("KOSE    ");
        break;

      default:
        lcd.print("NO    ");
        ability = 1;
        break;
    }

    lcd.setCursor(14, 0);
    lcd.print("M");
    lcd.setCursor(15, 0);
    lcd.print("6");

    lcd.setCursor(0, 1);
    if (language == english)
      lcd.print("BLADE:");
    if (language == turkce)
      lcd.print("BICAK:");
    lcd.setCursor(6, 1);
    switch (blade_mode) {
      case 1:
        if (language == english)
          lcd.print("WING    ");
        if (language == turkce)
          lcd.print("D.KNT   ");
        break;

      case 2:
        if (language == english)
          lcd.print("FRAME   ");
        if (language == turkce)
          lcd.print("KASA    ");
        break;

      case 3:
        if (language == english)
          lcd.print("DOOR    ");
        if (language == turkce)
          lcd.print("KAPI    ");
        break;

      case 4:
        if (language == english)
          lcd.print("B.DOOR  ");
        if (language == turkce)
          lcd.print("S.SER   ");
        break;

      default:
        lcd.print("NO    ");
        blade_mode = 1;
        break;
    }

    lcd.setCursor(14, 1);
    lcd.print("S");
    lcd.setCursor(15, 1);
    switch (motor_type) {
      case 1:
        lcd.print("1");
        break;

      case 2:
        lcd.print("2");
        break;

      default:
        lcd.print("N");
        motor_type = 1;
        break;
    }
  } else if (device_mode == DIGITAL_METER_MODE) {
    //    lcd.setCursor(0, 0);
    //    if(language == english) lcd.print("set : "); //5
    //    lcd.setCursor(6, 0);
    //    if(language == english) lcd.print("      "); //5
    //    lcd.setCursor(6, 0);
    //    if(language == english) lcd.print(Adjust_size);
  }
}
void EMR(void) {
  welding_temp = 0;
  setupPin();  // clear all pin output
  lcd.clear();
  lcd.setCursor(0, 0);
  if (language == english)
    lcd.print("!EMERGENCY MODE!");
  if (language == turkce)
    lcd.print("! ACIL KAPATMA !");
  lcd.setCursor(0, 1);
  if (language == english)
    lcd.print(" REBOOT DEVICE  ");
  if (language == turkce)
    lcd.print(" TEKRAR BASLAT  ");
  while (1) {
    if (read_key(Stop) == 0 && scan_key() != F6)
      break;

    if (lock_en == 1 && adding_time == 0) {
      keyNum = scan_key();
      if (read_key(Start) && keyNum == F4 && read_key(Stop) && EEPROM.readInt(80) == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Adding Days : 10");
        lock_timeing = calculateDays();
        lock_time = lock_timeing + 10;  //////////////////RTC ////////////
        adding_time = 1;
        EEPROM.writeInt(64, lock_time);
        EEPROM.writeInt(80, adding_time);
        delay(10);
        EEPROM.commit();
        delay(1500);
        delay(10);
        ESP.restart();
      }
    }
    delay(250);
    if (on_wi_fi)
      webApp.Loop();
    lcd.setCursor(0, 0);
    lcd.print("                ");
    delay(250);
    if (on_wi_fi)
      webApp.Loop();
    lcd.setCursor(0, 0);
    if (language == english)
      lcd.print("!EMERGENCY MODE!");
    if (language == turkce)
      lcd.print("! ACIL KAPATMA !");
  }
  error_display();
  ESP.restart();
}
void check_hardware(void) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Test Hardware  ");
  lcd.setCursor(0, 1);
  if (val_ds3231 == 1) lcd.print("  Time : 30(S)  ");
  if (val_ds3231 == 0) lcd.print("  Time : 27(S)  ");
  delay(1000);

  if (val_ds3231 == 1) {
    lcd.setCursor(0, 0);
    lcd.print("  Time : 29(S)  ");
    lcd.setCursor(0, 1);
    lcd.print(" RTC : Active ");
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("  Time : 28(S)  ");
    lcd.setCursor(0, 1);
    lcd.print(String(year) + "/" + String(month) + "/" + String(day) + " " + String(hour) + ":" + String(minute) + ":" + String(second));
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("  Time : 27(S)  ");
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("  Time : 26(S)  ");
    delay(1000);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("  Time : 26(S)  ");
    lcd.setCursor(0, 1);
    lcd.print(" RTC : Disabel");
    delay(1000);
  }


  lcd.setCursor(0, 0);
  lcd.print("  Time : 25(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("  Tabel: Enable ");
  digitalWrite(move_jack, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 24(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" Press Left: EN ");
  digitalWrite(move_jack, Disabled);
  digitalWrite(jack_left, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 23(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" Press Right: EN");
  digitalWrite(jack_left, Disabled);
  digitalWrite(jack_righ, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 22(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" Jack Heater: EN");
  digitalWrite(jack_righ, Disabled);
  digitalWrite(jack_heat, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 21(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" Shablon: Enable");
  digitalWrite(jack_heat, Disabled);
  digitalWrite(shablon, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 20(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("   SSR: Enable  ");
  digitalWrite(shablon, Disabled);
  digitalWrite(heater, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 19(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" Tap Tabel 1: EN");
  digitalWrite(heater, Disabled);
  digitalWrite(tap_move_1, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 18(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" Tap Tabel 2: EN");
  digitalWrite(tap_move_1, Disabled);
  digitalWrite(tap_move_2, Enable);
  delay(1000);
  digitalWrite(tap_move_2, Disabled);

  /////////////////////////////////////////////////
  lcd.setCursor(0, 0);
  lcd.print("    SUM Jacks   ");
  lcd.setCursor(0, 1);
  lcd.print("  Time : 17(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 16(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("  Tabel: Enable ");
  digitalWrite(move_jack, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 15(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 14(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("  Jack Left: EN ");
  digitalWrite(jack_left, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 13(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 12(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" Jack Right: EN ");
  digitalWrite(jack_righ, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 11(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 10(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("Jack Heater: EN");
  digitalWrite(jack_heat, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 9(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 8(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("  Shablon: EN  ");
  digitalWrite(move_jack, Disabled);
  digitalWrite(shablon, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 7(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 6(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("  SSR : Enable ");
  digitalWrite(jack_left, Disabled);
  digitalWrite(heater, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 5(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 4(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("tap Tabel 1: EN");
  digitalWrite(jack_righ, Disabled);
  digitalWrite(tap_move_1, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 3(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 2(S)  ");
  lcd.setCursor(0, 1);
  lcd.print("tap Tabel 2: EN");
  digitalWrite(jack_heat, Disabled);
  digitalWrite(tap_move_2, Enable);
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 1(S)  ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("  Time : 0(S)  ");
  lcd.setCursor(0, 1);
  lcd.print(" All Jack : Dis");

  digitalWrite(shablon, Disabled);
  digitalWrite(jack_left, Disabled);
  digitalWrite(move_jack, Disabled);
  digitalWrite(jack_righ, Disabled);
  digitalWrite(heater, Disabled);
  digitalWrite(tap_move_1, Disabled);
  digitalWrite(jack_heat, Disabled);
  digitalWrite(jack_righ_2, Disabled);

  lcd.clear();
  while (1) {
    int time_left_test = 0;
    delay(300);
    average();
    lcd.setCursor(0, 0);
    lcd.print("R1:   ");
    lcd.setCursor(3, 0);
    lcd.print(ADC_1);
    lcd.setCursor(6, 0);
    lcd.print("|");
    lcd.setCursor(7, 0);

    if ((ADC_1 > 380) && (ADC_1 < 430)) lcd.print("41");
    if ((ADC_1 > 200) && (ADC_1 < 270)) lcd.print("25");
    if ((ADC_1 > 150) && (ADC_1 < 200)) lcd.print("19");
    if ((ADC_1 > 60) && (ADC_1 < 130)) lcd.print("12");
    if (ADC_1 < 10) lcd.print("  ");

    lcd.setCursor(0, 1);
    lcd.print("R2:   ");
    lcd.setCursor(3, 1);
    lcd.print(ADC_2);
    lcd.setCursor(6, 1);
    lcd.print("|");
    lcd.setCursor(7, 1);

    if ((ADC_2 > 380) && (ADC_2 < 430)) lcd.print("41");
    if ((ADC_2 > 200) && (ADC_2 < 270)) {
      lcd.print("25");
      time_left_test = 0;
      while (1) {
        average();
        if ((ADC_2 < 220) || (ADC_2 > 280)) break;
        delay(1);
        time_left_test++;
        lcd.setCursor(9, 1);
        if (time_left_test == 200) lcd.print("ext");
        if (time_left_test == 400) lcd.print("EXT");
        if (time_left_test > 600) break;
      }
      lcd.print("   ");
    }
    if ((ADC_2 > 150) && (ADC_2 < 200)) lcd.print("19");
    if ((ADC_2 > 60) && (ADC_2 < 130)) lcd.print("12");
    if (ADC_2 < 10) lcd.print("  ");

    lcd.setCursor(9, 0);
    lcd.print("T:");

    int temp_read_ = thermocouple.readCelsius();
    lcd.setCursor(11, 0);
    if (temp_read_ < 500) {
      lcd.print(temp_read_ / 1.38);
    } else lcd.print("        ");

    lcd.setCursor(12, 1);
    lcd.print("K:");
    lcd.setCursor(14, 1);
    if (read_key(Stop)) {
      lcd.print("S1");
    } else if (read_key(Start)) {
      lcd.print("S2");
    } else if (read_key(back_ward)) {
      lcd.print("S3");
    } else if (read_key(Start_2)) {
      lcd.print("S4");
    }
    if (time_left_test > 600) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    END TEST    ");
      delay(1000);
      hardware_test = 0;
      EEPROM.writeInt(84, hardware_test);
      delay(10);
      EEPROM.commit();
      delay(10);
      break;
    }
  }
  lcd.clear();
  // Serial.println("restart");
  // delay(10);
  // ESP.restart();
}
int readTemp(void) {
  int av_temp = 0;  //temp_adj

  String b = (String)thermocouple.readCelsius();
  if (b == "nan") {
    //led_stat = led_stat & ~temp_signal;
    delay(202);
    return 10000;
  } else {
    for (int i = 0; i < 6; i++) {
      delay(202);
      av_temp = ((int)(thermocouple.readCelsius() * 4)) + av_temp;
    }  // av_temp / 6
    //led_stat = led_stat | temp_signal;
    return av_temp / 6;
  }
}
bool read_key(int key_pin) {
  if (digitalRead(key_pin) == 0) {
    delay(10);
    if (digitalRead(key_pin) == 0) {
      delay(10);
      if (digitalRead(key_pin) == 0) {
        delay(10);
        return true;
      }
    }
  }
  return false;
}
void read_write_eeprom(void) {
  if (EEPROM.readInt(100) == -1 || EEPROM.readInt(100) == 255) {
    address = 0;
    EEPROM.writeInt(address, e_up);
    address = 4;
    EEPROM.writeInt(address, c_up);
    address = 8;
    EEPROM.writeInt(address, s_up);
    address = 12;
    EEPROM.writeInt(address, PID);
    address = 16;
    EEPROM.writeInt(address, isJ);
    address = 20;
    EEPROM.writeInt(address, blade_mode);
    address = 24;
    EEPROM.writeInt(address, ability);
    address = 28;
    EEPROM.writeInt(address, motor_type);
    address = 32;
    EEPROM.writeInt(address, int(device_mode));
    address = 36;
    EEPROM.writeInt(address, language);

    address = 40;
    EEPROM.writeInt(address, ax_ready);
    address = 44;
    EEPROM.writeInt(address, ax_back);
    address = 48;
    EEPROM.writeInt(address, table_ready);
    address = 52;
    EEPROM.writeInt(address, table_back);
    address = 56;
    EEPROM.writeInt(address, heater_ready);
    address = 60;
    EEPROM.writeInt(address, heater_back);

    address = 88;
    EEPROM.writeInt(address, press_ready);
    address = 92;
    EEPROM.writeInt(address, press_back);
    address = 96;
    EEPROM.writeInt(address, tap_jack_1);
    address = 100;
    EEPROM.writeInt(address, tap_jack_2);

    address = 64;
    EEPROM.writeInt(address, lock_time);
    address = 68;
    EEPROM.writeInt(address, lock_timeing);
    address = 72;  //72
    EEPROM.writeInt(address, password);
    address = 76;
    EEPROM.writeInt(address, lock_en);
    address = 80;
    EEPROM.writeInt(address, adding_time);
    address = 84;
    EEPROM.writeInt(address, hardware_test);

    delay(10);
    EEPROM.commit();
    delay(10);
    Serial.println("EEPROM first");
  }
  address = 0;
  e_up = EEPROM.readInt(address);
  address += sizeof(int);
  c_up = EEPROM.readInt(address);
  address += sizeof(int);
  s_up = EEPROM.readInt(address);
  address += sizeof(int);
  PID = EEPROM.readInt(address);
  address += sizeof(int);
  isJ = EEPROM.readInt(address);
  //////////////////////////cleaner //////////
  address += sizeof(int);
  blade_mode = EEPROM.readInt(address);
  address += sizeof(int);
  ability = EEPROM.readInt(address);
  address += sizeof(int);
  motor_type = EEPROM.readInt(address);
  address += sizeof(int);
  device_mode = ((EEPROM.readInt(address)) > MODES_SIZE) ? MODE_WELDER_SINGLE : Mode(EEPROM.readInt(address));
  if (device_mode > MODES_SIZE || device_mode == 0)
    Mode(1);
  ch_mode = device_mode;
  address += sizeof(int);
  language = EEPROM.readInt(address);

  address += sizeof(int);
  ax_ready = EEPROM.readInt(address);
  address += sizeof(int);
  ax_back = EEPROM.readInt(address);
  address += sizeof(int);
  table_ready = EEPROM.readInt(address);
  address += sizeof(int);
  table_back = EEPROM.readInt(address);
  address += sizeof(int);
  heater_ready = EEPROM.readInt(address);
  address += sizeof(int);
  heater_back = EEPROM.readInt(address);
  address += sizeof(int);
  lock_time = EEPROM.readInt(address);
  address += sizeof(int);
  lock_timeing = EEPROM.readInt(address);
  address += sizeof(int);
  password = EEPROM.readInt(address);
  address += sizeof(int);
  lock_en = EEPROM.readInt(address);
  address += sizeof(int);
  adding_time = EEPROM.readInt(address);
  address += sizeof(int);
  hardware_test = EEPROM.readInt(address);

  if (lock_en == 1) {
    lock_timeing = calculateDays();  //////////////////RTC ////////////
    EEPROM.writeInt(68, lock_timeing);
    delay(10);
    EEPROM.commit();
    delay(10);
  }

  address = 0;
  sTime = s_up;
  cTemp = c_up;
  eTime = e_up;
  Serial.println("Set Variable");
}
/////////////////////////////////////////// upvc_cleaner /////////////////////////
void error_display(void) {
  lcd_disabled = true;
  for (int i = 0; i < 2; i++) {
    Delay(200);
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.setCursor(15, 0);
    lcd.print(" ");
    Delay(200);
    lcd.setCursor(0, 0);
    lcd.print("!");
    lcd.setCursor(15, 0);
    lcd.print("!");
  }
  lcd_disabled = false;
  update_f();
}
void upvc_cleaner(void) {
  if (ch_mode != device_mode) {
    ch_mode = device_mode;
    setupPin();
  }

  if (read_key(mic_sw_blade) == 1)  //////////////////1 0
  {
    if (read_key(mic_sw_motor) == 0) {
      if (read_key(Start) || keyNum == F5)  //&& map1 == 0)
      {
        on_wi_fi = false;
        digitalWrite(profile_holder_jack, Enable);
        lcd.setCursor(13, 1);
        lcd.write(byte(1));
        lcd_disabled = true;
        int press_time = 0;
        while (1) {
          press_time++;
          Delay(50);
          if (read_key(Start) == 0 && keyNum != F5)
            break;
          if (press_time >= 20)
            break;
        }
        lcd.setCursor(13, 1);
        lcd.print(" ");
        webApp.wStop();
        if ((ability == 1 || ability == 3) && press_time >= 20) {
          digitalWrite(up_down_nails, Enable);
          for (int i = 0; i < 6; i++) {
            lcd.setCursor(6, 0);
            lcd.print("      ");
            Delay(250);
            lcd.setCursor(6, 0);
            if (ability == 1) {
              lcd.print("FULL    ");
            } else {
              if (language == english)
                lcd.print("SCRAPE  ");
              if (language == turkce)
                lcd.print("YUZEY   ");
            }
            Delay(250);
          }
          if (read_key(mic_sw_blade) == 0) {
            digitalWrite(up_down_nails, Disabled);
            int step_error = 0;
            while (1) {
              lcd.setCursor(6, 0);
              lcd.print("      ");
              Delay(250);
              step_error++;
              bool av_key_mic = read_key(mic_sw_blade);
              lcd.setCursor(6, 0);
              if (ability == 1) {
                lcd.print("FULL    ");
              } else {
                if (language == english)
                  lcd.print("SCRAPE  ");
                if (language == turkce)
                  lcd.print("YUZEY   ");
              }
              Delay(300);
              if (av_key_mic == 1)
                break;
              if (step_error > 10)  // delay == 6(s)
              {
                lcd.setCursor(0, 0);
                if (language == english)
                  lcd.print("   ERROR  In    ");
                if (language == turkce)
                  lcd.print("     HATA       ");
                lcd.setCursor(0, 1);
                if (language == english)
                  lcd.print("NAILS JACK: BACK");
                if (language == turkce)
                  lcd.print("CIVI JAKINDA:GER");
                error_display();
                lcd.clear();
                break;
              }
            }
          } else {
            digitalWrite(up_down_nails, Disabled);
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("   ERROR  In    ");
            if (language == turkce)
              lcd.print("     HATA       ");
            lcd.setCursor(0, 1);
            if (language == english)
              lcd.print("Nails Jack:Front");
            if (language == turkce)
              lcd.print("CIVI JAKINDA: ON");
            error_display();
            Delay(1000);
            lcd.clear();
          }
        }
        if ((ability == 1 || ability == 4) && press_time >= 20) {
          if (motor_type == 1) {
            digitalWrite(motor_1, Enable);
          } else
            digitalWrite(motor_2, Enable);
          digitalWrite(motor_blade_move, Enable);

          int step_error = 0;
          while (1) {
            lcd.setCursor(6, 0);
            lcd.print("        ");
            Delay(250);
            step_error++;
            bool av_key_mic = read_key(mic_sw_motor);
            lcd.setCursor(6, 0);
            if (ability == 1) {
              lcd.print("FULL    ");
            } else {
              if (language == english)
                lcd.print("MILLING ");
              if (language == turkce)
                lcd.print("KOSE    ");
            }
            Delay(250);
            if (av_key_mic == 1) {
              Delay(1000);
              break;
            }
            if (step_error > 13)  // 7s
            {
              lcd.setCursor(0, 0);
              if (language == english)
                lcd.print("   ERROR  In    ");
              if (language == turkce)
                lcd.print("     HATA       ");
              lcd.setCursor(0, 1);
              if (language == english)
                lcd.print("MOTOR JACK:FRONT");
              if (language == turkce)
                lcd.print("MOTOR KRIKOSU:ON");
              error_display();
              lcd.clear();
              break;
            }
          }
          digitalWrite(motor_blade_move, Disabled);
          for (int i = 0; i < 6; i++) {
            lcd.setCursor(6, 0);
            lcd.print("        ");
            Delay(250);
            lcd.setCursor(6, 0);
            if (ability == 1) {
              lcd.print("FULL    ");
            } else {
              if (language == english)
                lcd.print("MILLING ");
              if (language == turkce)
                lcd.print("KOSE    ");
            }
            Delay(250);
          }
          if (read_key(mic_sw_motor) == 1) {
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("   ERROR  In    ");
            if (language == turkce)
              lcd.print("     HATA       ");
            lcd.setCursor(0, 1);
            if (language == english)
              lcd.print("MOTOR JACK:FRONT");
            if (language == turkce)
              lcd.print("MOTOR KRIKOSU:GE");
            error_display();
            lcd.clear();
          }
        }
        digitalWrite(motor_1, Disabled);
        digitalWrite(motor_2, Disabled);
        if (ability == 2 && press_time >= 20) {
          while (1) {
            lcd.setCursor(6, 0);
            lcd.print("        ");
            Delay(250);
            lcd.setCursor(6, 0);
            if (language == english)
              lcd.print("SETTING ");
            if (language == turkce)
              lcd.print("AYAR    ");
            Delay(250);
          }
        } else {
          digitalWrite(profile_holder_jack, Disabled);
        }
        while (1) {
          average();
          if ((read_key(mic_sw_blade) == 1 && read_key(mic_sw_motor) == 0) || scan_key() == F6 || read_key(Stop)) {
            break;
          } else {
            if (language == english) {
              lcd.setCursor(0, 0);
              lcd.print("SCRAPER SWC  OFF");
              lcd.setCursor(0, 1);
              lcd.print("  CHECK PLEASE  ");
            }
            if (language == turkce) {
              lcd.setCursor(0, 0);
              lcd.print("KAZIMA  SWC ACIK");
              lcd.setCursor(0, 1);
              lcd.print(" NONTROL EDINAZ ");
            }
            delay(100);
          }
        }
        Serial.println("End");
        // ability
        while (1) {
          if (read_key(Start) == 0 && keyNum != F5) break;
          Delay(10);
        }
        lcd_disabled = false;
      }
    } else {
      if (read_key(Start) || keyNum == F5) {
        lcd.setCursor(0, 0);
        if (language == english)
          lcd.print("   ERROR  In    ");
        if (language == turkce)
          lcd.print("     HATA       ");
        lcd.setCursor(0, 1);
        if (language == english)
          lcd.print("MOTOR JACK:FRONT");
        if (language == turkce)
          lcd.print("MOTOR KRIKOSU:GE");
        delay(1500);
        lcd.clear();
      }
    }
  } else {
    if (read_key(Start) || keyNum == F5) {
      lcd.setCursor(0, 0);
      if (language == english)
        lcd.print("   ERROR  In    ");
      if (language == turkce)
        lcd.print("     HATA       ");
      lcd.setCursor(0, 1);
      if (language == english)
        lcd.print("Nails Jack:Front");
      if (language == turkce)
        lcd.print("CIVI JAKINDA:GRE");
      delay(1500);
      lcd.clear();
    }
  }
  if (keyNum != 100) {
    if (p_stat == 0) {
      p_stat = 1;
      switch (keyNum) {
        case F1:
          digitalWrite(blade_mode_1, Disabled);
          digitalWrite(blade_mode_2, Disabled);
          blade_mode = 1;
          blade_mode_copy = blade_mode;
          update_lec();
          update_data = true;
          break;

        case F2:
          digitalWrite(blade_mode_1, Enable);
          digitalWrite(blade_mode_2, Disabled);
          blade_mode = 2;
          blade_mode_copy = blade_mode;
          update_lec();
          update_data = true;
          break;

        case F3:
          digitalWrite(blade_mode_1, Disabled);
          digitalWrite(blade_mode_2, Enable);
          blade_mode = 3;
          blade_mode_copy = blade_mode;
          update_lec();
          update_data = true;
          break;

        case F4:
          digitalWrite(blade_mode_1, Enable);
          digitalWrite(blade_mode_2, Enable);
          blade_mode = 4;
          blade_mode_copy = blade_mode;
          update_lec();
          update_data = true;
          break;

        case F7:
          if (ability < 4) {
            ability++;
          } else
            ability = 1;
          update_lec();
          update_data = true;
          break;

        case F8:
          if (motor_type == 1) {
            motor_type = 2;
            motor_type_copy = motor_type;
            digitalWrite(motor_replace, Enable);
          } else {
            motor_type = 1;
            motor_type_copy = motor_type;
            digitalWrite(motor_replace, Disabled);
          }
          update_lec();
          update_data = true;
          break;

        default:
          break;
      }
      if (on_wi_fi) webApp.reqHandShake();
    }
  } else
    p_stat = 0;

  if (blade_mode_copy != blade_mode) {
    blade_mode_copy = blade_mode;
    switch (blade_mode) {
      case 1:
        digitalWrite(blade_mode_1, Disabled);
        digitalWrite(blade_mode_2, Disabled);
        break;

      case 2:
        digitalWrite(blade_mode_1, Enable);
        digitalWrite(blade_mode_2, Disabled);
        break;

      case 3:
        digitalWrite(blade_mode_1, Disabled);
        digitalWrite(blade_mode_2, Enable);
        break;

      case 4:
        digitalWrite(blade_mode_1, Enable);
        digitalWrite(blade_mode_2, Enable);
        break;

      default:
        break;
    }
  }

  if (motor_type_copy != motor_type) {
    motor_type_copy = motor_type;
    if (motor_type == 2) digitalWrite(motor_replace, Enable);
    if (motor_type == 1) digitalWrite(motor_replace, Disabled);
  }
  lcd_disabled = false;
}
void check_lock_time(void) {
  if (lock_timeing >= lock_time) {
    int step_mode = 0, password_copy = 1000000;
    int count_false_pass = 0;
    bool p_stat_f1 = 0, p_stat_f2 = 0, p_stat_f7 = 0;
    bool refresh_lcd = 1;
    lcd_disabled = true;
    lcd.clear();
    digitalWrite(heater, Disabled);
    Serial.println("System Lock");
    if (val_ds3231 == 0) {
      lcd.print("Disconect Ds3231");
      lcd.setCursor(0, 0);
      Serial.println("Disconect Ds3231");
      Delay(2000);
    }
    if (no_battery == 1) {
      lcd.print("No Battery Ds3231");
      lcd.setCursor(0, 0);
      Serial.println("No Battery Ds3231");
      Delay(2000);
    }
    if (lock_timeing >= lock_time) {
      lcd.print("   End Time");
      lcd.setCursor(0, 0);
      Serial.println("End Time");
      Delay(2000);
    }
    if (language == english)
      lcd.print("System is Locked");
    if (language == turkce)
      lcd.print("SistemKilidlenib");
    Delay(1500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("F1)+     ");
    lcd.setCursor(0, 1);
    lcd.print("F2)-     ");

    lcd.setCursor(9, 0);
    lcd.print("F8)OK");

    lcd.setCursor(9, 1);
    if (language == english)
      lcd.print("F7)Next");
    if (language == turkce) {
      lcd.setCursor(6, 1);
      lcd.print("F7)Novbati");
    }
    Delay(5000);
    lcd.clear();
    while (1) {
      Delay(10);
      if (keyNum == F7) {
        if (p_stat_f7 == 0) {
          p_stat_f7 = 1;
          refresh_lcd = 1;
          if (step_mode < 5) {
            step_mode++;
          } else step_mode = 0;
        }
      } else p_stat_f7 = 0;

      if (keyNum == F1) {
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

      if (keyNum == F2) {
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

      if (refresh_lcd == 1) {
        refresh_lcd = 0;
        lcd.setCursor(0, 1);
        lcd.print("F1)+ F2)-       ");
        lcd.setCursor(step_mode + 10, 1);
        lcd.print("^");
        lcd.setCursor(9, 0);
        lcd.print(password_copy);
        lcd.setCursor(0, 0);
        if (language == english)
          lcd.print("Password :");
        if (language == turkce)
          lcd.print("   Parol :");
      }
      if (keyNum == F5) {
        if (password_copy == password || password_copy == PasswordAdmin || password_copy == PasswordAdmin_2) {
          lock_time = 0;
          lock_en = false;
          adding_time = 0;
          EEPROM.writeInt(64, lock_time);
          EEPROM.writeInt(76, lock_en);
          lock_timeing = calculateDays();     //////////////////RTC ////////////
          EEPROM.writeInt(68, lock_timeing);  ///////////////
          EEPROM.writeInt(80, adding_time);
          delay(10);
          EEPROM.commit();
          delay(10);
          while (1) {
            average();
            if (ADC_1 < 20 && ADC_2 < 20)
              break;
          }
          delay(200);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(" Password : Open ");
          delay(1500);
          lcd_disabled = false;
          Delay(20);
          break;
        } else if (password_copy == PasswordAddTime && adding_time == 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Add Time: 10 Day");
          lock_timeing = calculateDays();
          lock_time = lock_timeing + 10;  //////////////////RTC ////////////
          adding_time = 1;
          EEPROM.writeInt(64, lock_time);
          EEPROM.writeInt(68, lock_timeing);
          EEPROM.writeInt(80, adding_time);
          Serial.print("Set Time : 10 (Day)");
          Serial.println(lock_time);
          delay(20);
          EEPROM.commit();
          delay(1500);
          Serial.println("restart");
          delay(10);
          ESP.restart();
        } else {
          while (1) {
            average();
            if (ADC_1 < 20 && ADC_2 < 20)
              break;
          }
          delay(200);
          count_false_pass++;
          refresh_lcd = 1;
          if (count_false_pass > 3) {
            lcd.clear();
            count_false_pass = 0;
            lcd.setCursor(0, 0);
            if (language == english)
              lcd.print("System is Locked");
            if (language == turkce)
              lcd.print("SistemKilidlenib");
            delay(60000);
            lcd.clear();
            password_copy = 1000000;
            step_mode = 0;
          } else {
            lcd.clear();
            if (language == english)
              lcd.print("Password : False");
            if (language == turkce)
              lcd.print(" Parol : Yalan  ");
            Delay(1500);
          }
          lcd.clear();
        }
      }
    }
  }
}
void read_time_ds3231(void) {
  Serial.end();
  Wire.begin(1, 3);
  Wire.beginTransmission(0x68);  //address : DS3231
  byte error = Wire.endTransmission();
  if (error == 0) {
    val_ds3231 = true;
    ///setRTCDateTime();          ///////////// تنظیم زمان با زمان پروگرم
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
    bool h12Flag, pmFlag, century = false;
    year = rtc.getYear() + 2000;
    month = rtc.getMonth(century);
    day = rtc.getDate();
    hour = rtc.getHour(h12Flag, pmFlag);
    minute = rtc.getMinute();
    second = rtc.getSecond();
    temperature = rtc.getTemperature();
  }
  Wire.end();
  Serial.begin(115200);
  if (val_ds3231 == 0) Serial.println("DS3231 : False");
  if (no_battery == 1) Serial.println("Battery disconnected or oscillator stopped!");
  if (set_time_ds3231 == 1) Serial.println("Set Time");
  if (falt_ds3231 == 1) Serial.println("DS3231 : Cheng");
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
bool isLeapYear(int year) {  // بررسی سال کبیسه

  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}
// void upvc_digital_meter(void) {
//  Delay(1);

//  if (keyNum == F1 && Adjust_size < 10000) {
//    Adjust_size = Adjust_size + pow(10, Coefficient);
//    ;
//    lcd.setCursor(0, 0);
//    if (language == english)
//      lcd.print("set:           ");  // 5
//    lcd.setCursor(5, 0);
//    if (language == english)
//      lcd.print(Adjust_size);  // 5
//  }
//  if (keyNum == F2 && Adjust_size > 0) {
//    Adjust_size = Adjust_size - pow(10, Coefficient);
//    lcd.setCursor(0, 0);
//    if (language == english)
//      lcd.print("set:           ");  // 5
//    lcd.setCursor(5, 0);
//    if (language == english)
//      lcd.print(Adjust_size);  // 5
//  }

//  if (keyNum == F3 && Coefficient < 3) {
//    Coefficient++;
//    lcd.setCursor(15, 0);
//    if (language == english)
//      lcd.print(" ");  // 5
//    lcd.setCursor(15, 0);
//    if (language == english)
//      lcd.print(Coefficient);  // 5
//  }
//  if (keyNum == F4 && Coefficient > 0) {
//    Coefficient--;
//    lcd.setCursor(15, 0);
//    if (language == english)
//      lcd.print(" ");  // 5
//    lcd.setCursor(15, 0);
//    if (language == english)
//      lcd.print(Coefficient);  // 5
//  }
//  if (keyNum == F5 || read_key(Start)) {
//    steper_motor(Adjust_size, 1000);
//  }
// }

// void steper_motor(float step_unit, int rotational_speed) {
//  rotational_speed = 1000 / rotational_speed;
//  int min_speed_ms = 1000 / min_speed_hz;
//  float destination = step_unit;
//  step_unit = step_unit - last_location;

//  int rotational_speed_copy = rotational_speed;
//  if (step_unit > step_size || step_unit < step_size) {
//    start_meter = true;
//    if (step_unit > 0) {
//      step_unit = step_unit / step_size;

//      for (int i = 0; i < step_unit; i++) {
//        last_location = last_location + step_size;
//        if (i < 25) {
//          rotational_speed = 1 + min_speed_ms - ((min_speed_ms - rotational_speed_copy) * 0.04 * i);
//          if (i == 24)
//            rotational_speed = rotational_speed_copy;
//        } else if (i + 25 >= step_unit) {
//          rotational_speed = ((min_speed_ms - rotational_speed_copy) * (1 - (0.04 * (step_unit - i)))) + rotational_speed_copy + 1;
//        }
//        digitalWrite(L1, 0);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 0);
//        digitalWrite(L2, 0);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 0);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 0);
//        digitalWrite(L3, 0);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 0);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 0);
//        digitalWrite(L4, 0);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 0);
//        delay(rotational_speed);

//        digitalWrite(L1, 0);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 0);
//        delay(rotational_speed);
//        if (analogRead(ADC_KEY_1) > 100 || analogRead(ADC_KEY_2) > 100) {
//          keyNum = scan_key();
//        } else
//          keyNum = 100;

//        if (digitalRead(mic_first) == 0 || digitalRead(mic_end) == 0 || digitalRead(Stop) == 0 || keyNum == F6) {
//          if (read_key(Stop) == 1 || keyNum == F6) {
//            EMR();
//          } else if (read_key(mic_first))
//            break;
//        }

//        if (display_time_on + 200 < millis()) {
//          display_time_on = millis();
//          lcd.setCursor(5, 1);
//          if (language == english)
//            lcd.print("      ");  // 5
//          lcd.setCursor(5, 1);
//          if (language == english)
//            lcd.print(last_location);
//        }
//      }
//    } else {
//      step_unit = -step_unit;
//      step_unit = step_unit / step_size;
//      for (int i = 0; i < step_unit; i++) {
//        last_location = last_location - step_size;

//        if (i < 25) {
//          rotational_speed = 1 + min_speed_ms - ((min_speed_ms - rotational_speed_copy) * 0.04 * i);
//          if (i == 24)
//            rotational_speed = rotational_speed_copy;
//        } else if (i + 25 >= step_unit) {
//          rotational_speed = ((min_speed_ms - rotational_speed_copy) * (1 - (0.04 * (step_unit - i)))) + rotational_speed_copy + 1;
//        }

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 0);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 0);
//        digitalWrite(L4, 0);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 0);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 0);
//        digitalWrite(L3, 0);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 1);
//        digitalWrite(L2, 0);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 0);
//        digitalWrite(L2, 0);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 0);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 1);
//        delay(rotational_speed);

//        digitalWrite(L1, 0);
//        digitalWrite(L2, 1);
//        digitalWrite(L3, 1);
//        digitalWrite(L4, 0);
//        delay(rotational_speed);
//        if (analogRead(ADC_KEY_1) > 100 || analogRead(ADC_KEY_2) > 100) {
//          keyNum = scan_key();
//        } else
//          keyNum = 100;

//        if (digitalRead(mic_first) == 0 || digitalRead(mic_end) == 0 || digitalRead(Stop) == 0 || keyNum == F6) {
//          if (read_key(Stop) == 1 || keyNum == F6) {
//            EMR();
//          } else if (read_key(mic_end))
//            break;
//        }
//        if (display_time_on + 200 < millis()) {
//          display_time_on = millis();
//          lcd.setCursor(5, 1);
//          if (language == english)
//            lcd.print("      ");  // 5
//          lcd.setCursor(5, 1);
//          if (language == english)
//            lcd.print(last_location);
//        }
//      }
//    }
//    lcd.setCursor(5, 1);
//    if (language == english)
//      lcd.print("      ");  // 5
//    lcd.setCursor(5, 1);
//    if (digitalRead(mic_first) == 1 && digitalRead(mic_end) == 1) {
//      if (language == english)
//        lcd.print(destination);
//    } else if (language == english)
//      lcd.print(last_location);
//    start_meter = false;
//  }
// }