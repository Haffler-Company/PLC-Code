// task: task handle runtime input triggers

void jog_apply_engage(int new_dir);
void jog_ensure_numlock_mode();
void jog_freeze_target_at_current();

// خروج از حالت دستی فقط اگر ورود با جویستیک بوده و موتور واقعاً ایستاده
static void jog_maybe_exit_joystick_numlock() {
  if (!jog_numlock_from_joystick) return;
  if (!GUI.NumLock) {
    jog_numlock_from_joystick = false;
    return;
  }
  if (IOS.joystick_state_L || IOS.joystick_state_R) return;
  if (STP.jogEnabled || STP.motorIs_moving || STP.moveStepperCMD || isr_clutch) return;
  if (sensor_alert_motion_lock || GUI.lock_screen) return;
  GUI.NumLock = false;
  jog_numlock_from_joystick = false;
  GUI.edit_mode_lable1_1 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  GUI.update_lable1_1 = true;
  usb_keyboard_set_numlock_led(true);  // برگشت به حالت عدد
}

// -------------------------trig JOG upon input joystick-----------------------------//
bool lastSwitchState_r = HIGH;
void trig_joystick_jog_R() {
  if ((int32_t)(millis() - reference_jog_inhibit_until_ms) < 0) {
    lastSwitchState_r = IOS.joystick_state_R;
    STP.jogEnabled = false;
    return;
  }
  if (IOS.joystick_state_R == lastSwitchState_r) return;
  lastSwitchState_r = IOS.joystick_state_R;
  //-----------------------------------------//
  if (STP.referenced && GUI.scr_id == 1) {
    if (IOS.joystick_state_R == false) {
      if (IOS.joystick_state_L) {
        jog_apply_engage(-1);
        return;
      }
      STP.jogEnabled = false;
      // FIX Bug4: motorIs_moving را اینجا پاک نکن
      if (GUI.NumLock) jog_freeze_target_at_current();
    }
  }
  if (STP.referenced && GUI.scr_id == 1 && STP.currentPosition_p > STP.final_position_p) {
    STP.jogEnabled = false;
    STP.mvm_interupt = true;
    stepperStopMotion(true);
    STP.motorIs_moving = false;
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (STP.referenced && GUI.scr_id == 1) {
    if (IOS.joystick_state_R == true) {
      if (sensor_alert_motion_lock || GUI.lock_screen) {
        GUI.buzzer_cmd_long = true;
        return;
      }
      if (STP.flag_is_cutting) {
        GUI.buzzer_cmd_long = true;
        return;
      }
      if (main_queue_mode_active()) {
        request_exit_queue_mode_prompt(2);
        GUI.buzzer_cmd_long = true;
        return;
      }
      jog_ensure_numlock_mode();
      jog_apply_engage(1);
    }
  }
}

bool lastSwitchState_l = HIGH;
void trig_joystick_jog_L() {
  if ((int32_t)(millis() - reference_jog_inhibit_until_ms) < 0) {
    lastSwitchState_l = IOS.joystick_state_L;
    STP.jogEnabled = false;
    return;
  }
  if (IOS.joystick_state_L == lastSwitchState_l) return;
  lastSwitchState_l = IOS.joystick_state_L;
  //----------------------------------------------
  // هم‌ساختار با راست: رها فقط روی اصلی+رفرنس → فریز
  if (STP.referenced && GUI.scr_id == 1) {
    if (IOS.joystick_state_L == false) {
      if (IOS.joystick_state_R) {
        jog_apply_engage(1);
        return;
      }
      STP.jogEnabled = false;
      // FIX Bug4: motorIs_moving را اینجا پاک نکن
      if (GUI.NumLock) jog_freeze_target_at_current();
    }
  }
  if (STP.referenced && GUI.scr_id == 1 && STP.currentPosition_p < STP.initial_Position_p) {
    STP.jogEnabled = false;
    STP.mvm_interupt = true;
    stepperStopMotion(true);
    STP.motorIs_moving = false;
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (STP.referenced && GUI.scr_id == 1) {
    if (IOS.joystick_state_L == true) {
      if (sensor_alert_motion_lock || GUI.lock_screen) {
        GUI.buzzer_cmd_long = true;
        return;
      }
      if (STP.flag_is_cutting) {
        GUI.buzzer_cmd_long = true;
        return;
      }
      if (main_queue_mode_active()) {
        request_exit_queue_mode_prompt(2);
        GUI.buzzer_cmd_long = true;
        return;
      }
      jog_ensure_numlock_mode();
      jog_apply_engage(-1);
    }
  }
}
//------------------------------------------------------------------------------------//
void Task11(void *pvParameters) {
  vTaskDelay(650 / portTICK_PERIOD_MS);
  Serial.println("task11 start");
  while (1) {
    trig_joystick_jog_R();
    trig_joystick_jog_L();
    jog_maybe_exit_joystick_numlock();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
