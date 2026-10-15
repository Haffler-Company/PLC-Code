// task:  lvgl timerHandler (lvgl rendering)

#include "images.h"
#include <esp_task_wdt.h>

extern volatile bool g_flag_time_synced_toast;
extern volatile bool g_flag_update_available_toast;

//-------------update lable1_2-------------------------
static char buff_currentPosition_mm[12];
float temp_currentPosition_p;
static void main_format_mm_for_boot(char *out, size_t n, float mm) {
  const float rounded = roundf(mm * 10.0f) / 10.0f;
  if (fabsf(rounded - roundf(rounded)) < 0.05f)
    snprintf(out, n, "%d", (int)lroundf(rounded));
  else
    snprintf(out, n, "%.1f", (double)rounded);
}

static void main_format_position_for_boot(char *out, size_t n) {
  float ppmm = axisPulsesPerMm();
  if (ppmm < 1.0f) ppmm = 1.0f;
  main_format_mm_for_boot(out, n, (float)STP.currentPosition_p / ppmm);
}

void update_lable1_2() {
  if (GUI.scr_id == 2 || GUI.scr_id == 3) return;
  if (!GUI.flag_update_lable1_2) return;
  GUI.flag_update_lable1_2 = false;
  temp_currentPosition_p = STP.currentPosition_p;
  {
    float mm = temp_currentPosition_p / axisPulsesPerMm();
    float r1 = roundf(mm * 10.0f) / 10.0f;
    bool show_decimal = false;
    int display_units = 0;
    // حالت جاگ: همیشه صحیح — قالب اعشار گیج‌کننده نباشد
    if (GUI.NumLock) {
      display_units = (int)lroundf(mm);
    } else {
      const bool target_has_dec = fabsf(STP.Man_Sp_mm - roundf(STP.Man_Sp_mm)) >= 0.05f;
      const float remain = fabsf(mm - blade_err_motor_target_mm(STP.Man_Sp_mm));
      // در حرکت: بدون ممیز؛ نزدیک آخر (≤۱mm) اگر هدف ممیز دارد → ممیز فعال
      const bool near_end = target_has_dec && (remain <= 1.0f);
      if (STP.motorIs_moving && !near_end) {
        display_units = (int)lroundf(mm);
      } else if (near_end || (fabsf(r1 - roundf(r1)) >= 0.05f) ||
                 (!STP.motorIs_moving && target_has_dec)) {
        show_decimal = true;
        display_units = (int)lroundf(r1 * 10.0f);
      } else {
        display_units = (int)lroundf(r1);
      }
    }
    // بدون محدودیت زمانی: فقط وقتی مقدار قابل‌دیدن عوض شد متن/رندر ساخته شود.
    static int last_display_units = INT32_MIN;
    static bool last_show_decimal = false;
    if (display_units == last_display_units && show_decimal == last_show_decimal) return;
    last_display_units = display_units;
    last_show_decimal = show_decimal;
    if (show_decimal) {
      const int av = abs(display_units);
      snprintf(buff_currentPosition_mm, sizeof(buff_currentPosition_mm), "%s%d.%d",
               display_units < 0 ? "-" : "", av / 10, av % 10);
    } else {
      snprintf(buff_currentPosition_mm, sizeof(buff_currentPosition_mm), "%d", display_units);
    }
  }
  static char last_currentPosition_mm[12] = "";
  if (strcmp(last_currentPosition_mm, buff_currentPosition_mm) == 0) {
    // موقعیت عوض نشده — ولی فلگ مقصد ممکن است بالا باشد
    return;
  }
  strncpy(last_currentPosition_mm, buff_currentPosition_mm, sizeof(last_currentPosition_mm) - 1);
  last_currentPosition_mm[sizeof(last_currentPosition_mm) - 1] = '\0';
  if (!main_fast_position_digits_set_text(buff_currentPosition_mm))
    lv_label_set_text(objects.lable1_2, buff_currentPosition_mm);
  // استاپ همیشه بالای عدد وسط
  if (objects.emg && lv_obj_is_valid(objects.emg)) {
    if ((GUI.scr_id == 0 || GUI.scr_id == 1) && IOS.stop_button_state)
      lv_obj_move_foreground(objects.emg);
    else {
      lv_obj_del(objects.emg);
      objects.emg = NULL;
    }
  }
}
//-------------update lable1_1-------------------------
char buff_inputBuffer_sp[16];
void update_lable1_1() {
  if (GUI.scr_id != 1) return;
  if (!GUI.update_lable1_1) return;
  GUI.update_lable1_1 = false;
  // همگام‌سازی مقصد از جاگ (فریز/اسنپ) — فقط اینجا String نوشته شود
  if (jog_dest_buf_pending) {
    jog_dest_buf_pending = false;
    char tb[12];
    snprintf(tb, sizeof(tb), "%d", (int)jog_dest_buf_whole);
    if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)0) == pdTRUE) {
      GUI.inputBuffer_sp = tb;
      xSemaphoreGive(lable1_1_Semaphore);
    } else {
      GUI.inputBuffer_sp = tb;
    }
  }
  //
  const bool jog_held = keyboardJogActive || IOS.joystick_state_L || IOS.joystick_state_R;
  if (GUI.NumLock && jog_held) {
    // جاگ: مقصد زنده = موتور + پیش‌بینی شیب؛ رشد بعد از رها: فقط زمان از پایه
    float mm = (float)STP.currentPosition_p / axisPulsesPerMm();
    // فاصله توقف فقط به سرعت لحظه‌ای بستگی دارد — Rc مرجع همیشه کروز کامل (نه فاین).
    // اگر Rc با رها کردن کلید ۲ عوض شود، ahead جمع می‌شود و مقصد ~۱ث هنگ می‌کند.
    const int cruise_full = constrain(STP.global_speed, 8, 100);
    int cruise_iv = cruise_full;
    if (jog_fine_held) {
      // فقط برای مسیر رشد (نرخ مقصد با موتور ÷۶)
      float rate = 500000.0f / (float)cruise_full;
      float slow = rate / 6.0f;
      if (slow < 1.0f) slow = 1.0f;
      cruise_iv = constrain((int)lroundf(500000.0f / slow), cruise_full, JOG_TIMER_MAX_INTERVAL_US);
    }
    const bool growing = jog_grow_active;  // حتی وسط بکاپ عوض‌جهت — مقصد قفل نشود
    float ahead = 0.0f;
    if (!growing) {
      int configured_stop_iv = constrain(jog_init_speed, cruise_full, 400);
      int stop_iv = (jog_ramp_ms > 0)
                      ? max(configured_stop_iv, (int)jog_live_interval_us)
                      : (int)jog_live_interval_us;
      int a = constrain((int)jog_live_interval_us, 8, JOG_TIMER_MAX_INTERVAL_US);
      int b = constrain(stop_iv, 8, JOG_TIMER_MAX_INTERVAL_US);
      int c = constrain(cruise_full, 8, JOG_TIMER_MAX_INTERVAL_US);  // همیشه کامل — نه cruise_iv فاین
      float R0 = 500000.0f / (float)a;
      float R1 = 500000.0f / (float)b;
      float Rc = 500000.0f / (float)c;
      if (jog_ramp_ms > 0 && R0 > R1 + 1.0f) {
        float frac = (Rc > R1 + 1.0f) ? (R0 - R1) / (Rc - R1) : 1.0f;
        frac = constrain(frac, 0.0f, 1.0f);
        float T = ((float)jog_ramp_ms / 1000.0f) * frac;
        if (T >= 0.001f) {
          float pulses = T * (R0 - R1) / logf(R0 / R1);
          float ppmm = axisPulsesPerMm();
          if (ppmm < 1.0f) ppmm = 1.0f;
          ahead = pulses / ppmm;
        }
      }
    }
    // رشد/عوض‌جهت: همیشه wish؛ وگرنه DIR فیزیکی
    int dir_src = STP.system_set_dir;
    if ((jog_backup_active || growing) && (jog_wish_dir == 1 || jog_wish_dir == -1))
      dir_src = jog_wish_dir;
    int dir = (dir_src * STP.system_base_dir >= 0) ? 1 : -1;
    static int jog_ui_last_whole = -999999;
    static int jog_ui_blend_from = 0;
    static uint32_t jog_ui_blend_t0 = 0;
    static bool jog_ui_blending = false;
    static bool grow_session = false;
    static uint32_t grow_session_t0 = 0;
    static bool grow_fine_latched = false;
    static bool grow_same_dir_session = false;
    const uint32_t fade_ms = 400u;
    if (growing) {
      jog_ui_blending = false;
    } else {
      grow_session = false;
      grow_session_t0 = 0;
      grow_same_dir_session = false;
      if (jog_pred_dir != 1 && jog_pred_dir != -1) {
        jog_pred_dir = dir;
        jog_pred_dir_ms = 0;
        jog_ui_blending = false;
      } else if (dir != jog_pred_dir) {
        int prev = jog_ui_last_whole;
        if (prev == -999999) prev = (int)lroundf(STP.Man_Sp_mm);
        jog_ui_blend_from = prev;
        jog_ui_blend_t0 = millis();
        jog_ui_blending = (jog_ramp_ms > 0);
        jog_pred_dir = dir;
        jog_pred_dir_ms = (jog_ramp_ms > 0) ? millis() : 0;
        ahead = 0.0f;
      } else if (jog_pred_dir_ms != 0 && jog_ramp_ms > 0) {
        uint32_t age = millis() - jog_pred_dir_ms;
        if (age < fade_ms) ahead *= (float)age / (float)fade_ms;
      }
    }
    int raw = (int)lroundf(mm + (float)dir * ahead);
    int frozen = (int)lroundf(STP.Man_Sp_mm);
    int whole = raw;
    if (!growing && jog_ui_blending && jog_ramp_ms > 0) {
      uint32_t age = millis() - jog_ui_blend_t0;
      if (age >= fade_ms) {
        jog_ui_blending = false;
        whole = raw;
      } else {
        float t = (float)age / (float)fade_ms;
        whole = (int)lroundf((float)jog_ui_blend_from + t * ((float)raw - (float)jog_ui_blend_from));
      }
    }
    if (jog_grow_active) {
      static int grow_r0_iv = 320;
      const bool same_grow_session =
        grow_session && grow_session_t0 == jog_grow_t0;
      // لبهٔ کلید ۲: grow را قطع کن — مقصد برگردد به موتور+پیش‌بینی (هم‌پا)
      if (same_grow_session && (bool)jog_fine_held != grow_fine_latched) {
        grow_fine_latched = jog_fine_held;
        jog_grow_active = false;
        grow_session = false;
        grow_session_t0 = 0;
        whole = raw;
        if (dir > 0 && frozen > whole) whole = frozen;
        if (dir < 0 && frozen < whole) whole = frozen;
      } else {
        if (!same_grow_session) {
          grow_session = true;
          grow_session_t0 = jog_grow_t0;
          grow_fine_latched = jog_fine_held;
          grow_same_dir_session = (jog_wish_dir == STP.system_set_dir);
          // ادامهٔ هم‌جهت باید با سرعت لحظه‌ای موتور رشد کند؛
          // جهت معکوس بعد از توقف از سرعت راه‌اندازی خودش شروع می‌شود.
          grow_r0_iv = grow_same_dir_session
                         ? constrain((int)jog_live_interval_us, 8, JOG_TIMER_MAX_INTERVAL_US)
                         : constrain(jog_init_speed, 8, JOG_TIMER_MAX_INTERVAL_US);
        }
        int base = (int)lroundf(jog_grow_base_mm);
        float ppmm = axisPulsesPerMm();
        if (ppmm < 1.0f) ppmm = 1.0f;
        float R0 = 500000.0f / (float)grow_r0_iv;
        float Rc = 500000.0f / (float)cruise_iv;
        if (R0 < 1.0f) R0 = 1.0f;
        if (Rc < 1.0f) Rc = 1.0f;
        float t = (float)(millis() - jog_grow_t0) / 1000.0f;
        if (t < 0.0f) t = 0.0f;
        float add_mm = 0.0f;
        if (jog_ramp_ms <= 0) {
          add_mm = (Rc / ppmm) * t;
        } else {
          float T = (float)jog_ramp_ms / 1000.0f;
          if (T < 0.001f) T = 0.001f;
          float ratio = Rc / R0;
          float ln_r = logf(ratio);
          float pulses;
          if (t <= T) {
            float p = t / T;
            pulses = (fabsf(ln_r) < 1e-5f) ? (R0 * t)
                                           : (R0 * T / ln_r) * (powf(ratio, p) - 1.0f);
          } else {
            float pulses_ramp = (fabsf(ln_r) < 1e-5f) ? (R0 * T)
                                                      : (R0 * T / ln_r) * (ratio - 1.0f);
            pulses = pulses_ramp + Rc * (t - T);
          }
          add_mm = pulses / ppmm;
        }
        int delta = (int)floorf(add_mm + 1e-4f);
        if (delta < 0) delta = 0;
        whole = base + dir * delta;
        if (dir > 0) {
          if (whole < base) whole = base;
          if (whole < jog_grow_peak_whole) whole = jog_grow_peak_whole;
          else jog_grow_peak_whole = whole;
        } else {
          if (whole > base) whole = base;
          if (whole > jog_grow_peak_whole) whole = jog_grow_peak_whole;
          else jog_grow_peak_whole = whole;
        }
      }
    } else if (!jog_backup_active) {
      // مقصد زنده پیش‌بینی است؛ از مقدار قبلی عقب‌تر نشود (پریدن عقب ممنوع)
      // با کلید ۲ اجازه بده به موتور نزدیک شود
      if (!jog_fine_held) {
        if (dir > 0 && frozen > whole) whole = frozen;
        if (dir < 0 && frozen < whole) whole = frozen;
      }
    }
    // سقف = همان پیش‌بینی توقف — مقصد از موتور+ahead جلوتر نرود و عقب‌تر هم نماند
    {
      float ahead_cap = ahead;
      if (growing) {
        ahead_cap = 0.0f;
        int configured_stop_iv = constrain(jog_init_speed, cruise_full, 400);
        int stop_iv = (jog_ramp_ms > 0)
                        ? max(configured_stop_iv, (int)jog_live_interval_us)
                        : (int)jog_live_interval_us;
        int a = constrain((int)jog_live_interval_us, 8, JOG_TIMER_MAX_INTERVAL_US);
        int b = constrain(stop_iv, 8, JOG_TIMER_MAX_INTERVAL_US);
        int c = constrain(cruise_full, 8, JOG_TIMER_MAX_INTERVAL_US);
        float R0c = 500000.0f / (float)a;
        float R1c = 500000.0f / (float)b;
        float Rcc = 500000.0f / (float)c;
        if (jog_ramp_ms > 0 && R0c > R1c + 1.0f) {
          float frac = (Rcc > R1c + 1.0f) ? (R0c - R1c) / (Rcc - R1c) : 1.0f;
          frac = constrain(frac, 0.0f, 1.0f);
          float T = ((float)jog_ramp_ms / 1000.0f) * frac;
          if (T >= 0.001f) {
            float pulses = T * (R0c - R1c) / logf(R0c / R1c);
            float ppmm = axisPulsesPerMm();
            if (ppmm < 1.0f) ppmm = 1.0f;
            ahead_cap = pulses / ppmm;
          }
        }
      }
      int cap = (int)lroundf(mm + (float)dir * ahead_cap);
      if (!growing) {
        // مسیر عادی: مقصد دقیقاً روی پیش‌بینی بایستد (جلوتر از موتور)
        whole = cap;
        if (!jog_fine_held) {
          if (dir > 0 && frozen > whole) whole = frozen;
          if (dir < 0 && frozen < whole) whole = frozen;
        }
      } else if (!grow_same_dir_session) {
        // فقط تغییر جهت معکوس سقف موتور+توقف را نگه می‌دارد.
        // ادامهٔ هم‌جهت باید از مقصد فریز‌شده جلو برود و هرگز به موتور برنگردد.
        if (dir > 0 && whole > cap) whole = cap;
        if (dir < 0 && whole < cap) whole = cap;
      }
    }
    int lim_lo = (int)lroundf((float)STP.initial_Position_p / axisPulsesPerMm());
    int lim_hi = STP.final_position_mm.toInt();
    if (whole < lim_lo) whole = lim_lo;
    if (whole > lim_hi) whole = lim_hi;
    jog_ui_last_whole = whole;
    if ((int)lroundf(STP.Man_Sp_mm) != whole) {
      STP.Man_Sp_mm = (float)whole;
      STP.position_Setpoint_p = (int)lroundf((float)whole * axisPulsesPerMm());
      char tb[12];
      snprintf(tb, sizeof(tb), "%d", whole);
      if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)0) == pdTRUE) {
        GUI.inputBuffer_sp = tb;
        xSemaphoreGive(lable1_1_Semaphore);
      } else {
        GUI.inputBuffer_sp = tb;
      }
    }
    snprintf(buff_inputBuffer_sp, sizeof(buff_inputBuffer_sp), "%d", whole);
    static char last_jog_dest_str[16] = "";
    if (strcmp(last_jog_dest_str, buff_inputBuffer_sp) == 0) return;
    strncpy(last_jog_dest_str, buff_inputBuffer_sp, sizeof(last_jog_dest_str) - 1);
    last_jog_dest_str[sizeof(last_jog_dest_str) - 1] = '\0';
    if (!main_fast_destination_set_text(buff_inputBuffer_sp))
      lv_label_set_text(objects.lable1_1, buff_inputBuffer_sp);
    return;
  }
  // رها شده / غیرجاگ: عدد ثابت هدف (inputBuffer)
  if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)5) == pdTRUE) {
    strncpy(buff_inputBuffer_sp, GUI.inputBuffer_sp.c_str(), sizeof(buff_inputBuffer_sp) - 1);
    buff_inputBuffer_sp[sizeof(buff_inputBuffer_sp) - 1] = '\0';
    xSemaphoreGive(lable1_1_Semaphore);
  }
  // بافر خالی حین تایپ/بک‌اسپیس: کادر خاکستری بماند و «۰» نشان بده.
  // پنهان‌کردن فقط با main_clear_destination_sp (اتمام صف) — نه با خالی شدن کیبورد.
  if (buff_inputBuffer_sp[0] == '\0') {
    if (!GUI.edit_mode_lable1_1 && !GUI.NumLock &&
        objects.lable1_1 && lv_obj_has_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN)) {
      // عمداً پنهان مانده (اتمام لیست/بهینه) — دست نزن
      return;
    }
    strncpy(buff_inputBuffer_sp, "0", sizeof(buff_inputBuffer_sp));
  }
  static char last_dest_str[16] = "";
  if (strcmp(last_dest_str, buff_inputBuffer_sp) == 0 &&
      objects.lable1_1 && !lv_obj_has_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN)) {
    return;
  }
  strncpy(last_dest_str, buff_inputBuffer_sp, sizeof(last_dest_str) - 1);
  last_dest_str[sizeof(last_dest_str) - 1] = '\0';
  if (objects.panel1_2) lv_obj_clear_flag(objects.panel1_2, LV_OBJ_FLAG_HIDDEN);
  if (objects.lable1_1) lv_obj_clear_flag(objects.lable1_1, LV_OBJ_FLAG_HIDDEN);
  if (!main_fast_destination_set_text(buff_inputBuffer_sp))
    lv_label_set_text(objects.lable1_1, buff_inputBuffer_sp);
}
//-----------------------flag driven lable 1_3 update-----------------------
char buff_lable_1_3[12];
static void layout_main_count_footer(bool dash_mode) {
  if (objects.lable1_3) lv_obj_set_pos(objects.lable1_3, 112, dash_mode ? 236 : 239);
  if (objects.obj2) lv_obj_set_pos(objects.obj2, 141, dash_mode ? 253 : 259);
  if (objects.obj3) lv_obj_set_pos(objects.obj3, 316, dash_mode ? 260 : 266);
}
void update_lable1_3() {
  if (GUI.scr_id != 1) return;
  if (!GUI.update_lable1_3) return;
  GUI.update_lable1_3 = false;
  //
  if (AUT.selected_count_val == -1) {
    layout_main_count_footer(true);
    lv_label_set_text(objects.lable1_3, "---");
    opt_main_bar_lbl_sync();
    return;
  }
  layout_main_count_footer(false);
  snprintf(buff_lable_1_3, sizeof(buff_lable_1_3), "%d", AUT.temp_list_count_Val);
  static char last_lable_1_3[12] = "";
  if (strcmp(last_lable_1_3, buff_lable_1_3) != 0) {
    strncpy(last_lable_1_3, buff_lable_1_3, sizeof(last_lable_1_3) - 1);
    last_lable_1_3[sizeof(last_lable_1_3) - 1] = '\0';
    lv_label_set_text(objects.lable1_3, buff_lable_1_3);
  }
  // در مود بهینه، temp = تعداد run برنامه است؛ موجودی لیست را خراب نکن
  if (!OPT.active) {
    int idx = AUT.selected_count_val;
    if (idx >= 0 && idx < LIST_MAX) {
      AUT.list_count_Val[idx] = AUT.temp_list_count_Val;
      GUI.inputBuffer_L_3_2_x[idx] = buff_lable_1_3;
      GUI.pending_count_save_bits[idx >> 3] |= (uint8_t)(1u << (idx & 7));
    }
  }
  opt_main_bar_lbl_sync();
}
//--------------reinitialize settings widgets when re-entering settings screen-----------------//
void reinit_settings_widgets() {
  if (!GUI.flag_reinit_settings) return;
  GUI.flag_reinit_settings = false;
  // Reset sliders from actual (saved) values
  lv_slider_set_value(objects.slider2_1, (int)map(STP.global_speed, 8, 100, 1000, 0), LV_ANIM_OFF);
  // اسلایدر راه‌اندازی = شیب زمانی ۰..۵۰۰۰ms (۰٪=بدون شیب، ۱۰۰٪=۵ثانیه)
  lv_slider_set_value(objects.slider2_2, (int)map(auto_ramp_ms, 0, 5000, 0, 1000), LV_ANIM_OFF);
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(objects.slider2_1) / 10);
    lv_label_set_text(objects.lbl_spd2_1, buf);
    snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(objects.slider2_2) / 10);
    lv_label_set_text(objects.lbl_spd2_2, buf);
  }
  // Reset rollers from actual values
  lang_selector_update_visual((GUI.system_language == 1) ? 0 : 1);
  lv_roller_set_selected(objects.roller2_2, (int)constrain(IOS.cut_prcs_timer / 1000 - 1, 0, 4), LV_ANIM_OFF);
  // Reset switch from actual value
  if (STP.system_base_dir == 1) lv_obj_add_state(objects.sw2_1, LV_STATE_CHECKED);
  else lv_obj_clear_state(objects.sw2_1, LV_STATE_CHECKED);
  // بازر فقط روی چیپ جدید — تصاویر EEZ قدیمی را هرگز نشان نده
  if (objects.img2_1) lv_obj_add_flag(objects.img2_1, LV_OBJ_FLAG_HIDDEN);
  if (objects.img2_2) lv_obj_add_flag(objects.img2_2, LV_OBJ_FLAG_HIDDEN);
  vkb_apply_effective(true);
  vkb_settings_sync();
  f1_apply_language_texts();  // عنوان تب + اول/آخر خط
  f2_apply_jog_slider_from_value(jog_ramp_ms);
  f2_sync_line_box_colors();
  settings_refresh_after_reinit();
  settings_hide_eez_clutter();
  // بعد از هنگ: reinit نباید تب F2..F5 را به F1 برگرداند
  if (ui_resume_help_delay > 0 && objects.tabview2_1) {
    int tab = (int)ui_resume_apply_settings_tab;
    if (tab < 0) tab = 0;
    if (tab > 4) tab = 4;
    if ((int)lv_tabview_get_tab_act(objects.tabview2_1) != tab) {
      lv_tabview_set_active(objects.tabview2_1, tab, LV_ANIM_OFF);
      admin_tabs_reflow();
      f1_header_title_sync();
    }
  }
}
//--------------update lables in screen 2 intervally ---------------------------------------------//
void update_lables_2() {
  if (GUI.scr_id != 2) return;
  if (!GUI.flag_update_lables_2) return;
  GUI.flag_update_lables_2 = false;
  if (STP.begin_calibration) {
    const char *l2_txt = (STP.calib_stage <= 2) ? GUI.inputBuffer_initial_L1.c_str() : GUI.inputBuffer_initial_L2.c_str();
    static char last_l2[20] = "";
    static char last_l3[20] = "";
    if (strcmp(last_l2, l2_txt) != 0) {
      strncpy(last_l2, l2_txt, sizeof(last_l2) - 1);
      last_l2[sizeof(last_l2) - 1] = '\0';
      lv_label_set_text(objects.lable2_2, l2_txt);
    }
    if (strcmp(last_l3, STP.final_position_mm.c_str()) != 0) {
      strncpy(last_l3, STP.final_position_mm.c_str(), sizeof(last_l3) - 1);
      last_l3[sizeof(last_l3) - 1] = '\0';
      lv_label_set_text(objects.lable2_3, STP.final_position_mm.c_str());
    }
  } else if (GUI.enable_edit_lable2_3) {
    static char last_l2_edit[20] = "";
    if (strcmp(last_l2_edit, GUI.inputBuffer_initial_L2.c_str()) != 0) {
      strncpy(last_l2_edit, GUI.inputBuffer_initial_L2.c_str(), sizeof(last_l2_edit) - 1);
      last_l2_edit[sizeof(last_l2_edit) - 1] = '\0';
      lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L2.c_str());
    }
  }
}
//------------------------process lable colors upon edit----------------------------//
bool prev_edit_mode_lable1_1;
bool prev_edit_mode_lable2_2;
bool prev_edit_mode_lable2_3;
void prcs_lable_clr() {
  // B2 fix: بدون return زودهنگام — قبلاً اگر edit-mode لیبل 1_1 خارج از صفحهٔ 1 عوض می‌شد،
  // prev هرگز sync نمی‌شد و آپدیت رنگ لیبل‌های صفحهٔ 2 برای همیشه بلاک می‌ماند.
  // ست‌کردن استایل روی آبجکت صفحهٔ غیرفعال در LVGL بی‌خطر است.
  if (prev_edit_mode_lable1_1 != GUI.edit_mode_lable1_1) {
    lv_obj_set_style_text_color(objects.lable1_1, lv_color_hex((GUI.edit_mode_lable1_1) ? 0x0011d1 : 0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    prev_edit_mode_lable1_1 = GUI.edit_mode_lable1_1;
  }
  if (prev_edit_mode_lable2_2 != GUI.edit_mode_lable2_2) {
    lv_obj_set_style_text_color(objects.lable2_2, lv_color_hex((GUI.edit_mode_lable2_2) ? 0x0011d1 : 0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
    prev_edit_mode_lable2_2 = GUI.edit_mode_lable2_2;
  }
  if (prev_edit_mode_lable2_3 != GUI.edit_mode_lable2_3) {
    // edit_mode_lable2_3 = ویرایش اول خط روی کادر بالا (lable2_2)
    lv_obj_set_style_text_color(objects.lable2_2, lv_color_hex((GUI.edit_mode_lable2_3 || GUI.enable_edit_lable2_3) ? 0x0011d1 : 0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
    prev_edit_mode_lable2_3 = GUI.edit_mode_lable2_3;
  }
  // رنگ کادر آخر خط هنگام ویرایش لمسی F2
  static bool prev_en_end = false;
  static bool prev_en_start = false;
  if (prev_en_end != GUI.enable_edit_lable2_5 || prev_en_start != GUI.enable_edit_lable2_3) {
    if (objects.lable2_3)
      lv_obj_set_style_text_color(objects.lable2_3,
                                  lv_color_hex(GUI.enable_edit_lable2_5 ? 0x0a84ff : 0x212121),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    if (objects.lable2_2 && !GUI.edit_mode_lable2_3 && !GUI.edit_mode_lable2_2)
      lv_obj_set_style_text_color(objects.lable2_2,
                                  lv_color_hex(GUI.enable_edit_lable2_3 ? 0x0a84ff : 0x212121),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    prev_en_end = GUI.enable_edit_lable2_5;
    prev_en_start = GUI.enable_edit_lable2_3;
  }
}
//-----------------------------prcs led indicators---------------------------------------
bool prev_ref_sensor_state = true;
bool prev_up_lim_sensor_state = true;
bool prev_start1_button_state = true;
bool prev_start2_button_state = true;
bool prev_stop_button_state = true;
bool prev_blade_return_state = true;
bool prev_motor_fault_state = true;
bool prev_micro_switch_state = true;
void led_indicator_update() {
  if (GUI.scr_id != 2) return;
  bool line_sensor_changed = false;
  if (prev_ref_sensor_state != IOS.ref_sensor_state) {
    lv_led_set_brightness(objects.led2_3_1, (IOS.ref_sensor_state) ? 255 : 0);
    prev_ref_sensor_state = IOS.ref_sensor_state;
    line_sensor_changed = true;
  }
  if (prev_up_lim_sensor_state != IOS.up_lim_sensor_state) {
    lv_led_set_brightness(objects.led2_3_2, (IOS.up_lim_sensor_state) ? 255 : 0);
    prev_up_lim_sensor_state = IOS.up_lim_sensor_state;
    line_sensor_changed = true;
  }
  // Reapplying six styles to both F2 boxes at 40 Hz forced about 142% RF and
  // 85% UI busy on an otherwise idle real screen. Repaint only on state change.
  if (line_sensor_changed) f2_sync_line_box_colors();
  // در تب F3، تمامی LEDهای ورودی به‌صورت پویا از جدول پین‌مپ در f3_pinmap_poll مدیریت می‌شوند
  if (ui_active_settings_tab != 2) {
    if (prev_start1_button_state != IOS.start1_button_state) {
      lv_led_set_brightness(objects.led2_3_3, (IOS.start1_button_state) ? 255 : 0);
      prev_start1_button_state = IOS.start1_button_state;
    }
    if (prev_start2_button_state != IOS.start2_button_state) {
      lv_led_set_brightness(objects.led2_3_7, (IOS.start2_button_state) ? 255 : 0);
      prev_start2_button_state = IOS.start2_button_state;
    }
    if (prev_stop_button_state != IOS.stop_button_state) {
      lv_led_set_brightness(objects.led2_3_4, (IOS.stop_button_state) ? 255 : 0);
      prev_stop_button_state = IOS.stop_button_state;
    }
    if (prev_blade_return_state != IOS.blade_return_state) {
      lv_led_set_brightness(objects.led2_3_5, (IOS.blade_return_state) ? 255 : 0);
      prev_blade_return_state = IOS.blade_return_state;
    }
    if (prev_motor_fault_state != IOS.motor_fault_state) {
      lv_led_set_brightness(objects.led2_3_6, (IOS.motor_fault_state) ? 255 : 0);
      prev_motor_fault_state = IOS.motor_fault_state;
    }
    if (prev_micro_switch_state != IOS.guard_switch_state) {
      lv_led_set_brightness(objects.led2_3_8, (IOS.guard_switch_state) ? 255 : 0);
      prev_micro_switch_state = IOS.guard_switch_state;
    }
  }
}
//-------------------visible cut list raws--------------------------------------------
lv_obj_t *get_row_panel(int i) {
  switch (i) {
    case 0: return objects.panel3_1_1;
    case 1: return objects.panel3_1_2;
    case 2: return objects.panel3_1_3;
    case 3: return objects.panel3_1_4;
    case 4: return objects.panel3_1_5;
    case 5: return objects.panel3_1_6;
    case 6: return objects.panel3_1_7;
    case 7: return objects.panel3_1_8;
    case 8: return objects.panel3_1_9;
    case 9: return objects.panel3_1_10;
  }
  return NULL;
}
int prev_visible_raws = -1;
static int prev_list_page_for_vis = -999;
// ورود لیست را در چند فریم انجام بده؛ پیمایش و ساخت اشیای ردیف‌ها پشت‌سرهم WDT را می‌گرفت.
static uint8_t list_entry_phase = 0;

void prcs_visible_raws() {
  if (GUI.scr_id != 3) return;
  // تعداد ردیف فعال روی صفحهٔ جاری (نه کل لیست)
  int rows_on_page = GUI.visible_raws - page_base();
  if (rows_on_page < 0) rows_on_page = 0;
  if (rows_on_page > LIST_ROWS_PER_PAGE) rows_on_page = LIST_ROWS_PER_PAGE;
  const int vis_cap = list_rows_on_screen();
  if (rows_on_page > vis_cap) rows_on_page = vis_cap;
  // حتماً با عوض‌شدن صفحه هم رفرش کن — قبلاً فقط تعداد ردیف چک می‌شد و متن/کادر قاطی می‌شد
  bool page_changed = (prev_list_page_for_vis != GUI.list_page);
  if (!page_changed && prev_visible_raws == rows_on_page && !GUI.flag_refresh_visible) return;
  GUI.flag_refresh_visible = false;
  prev_visible_raws = rows_on_page;
  prev_list_page_for_vis = GUI.list_page;
  if (page_changed) {
    GUI.flag_update_labels_3_1 = true;
    GUI.flag_update_labels_3_2 = true;
    GUI.flag_update_panel3_2_x_clr = true;
    GUI.flag_panel3_edit_labels_txt = true;
  }
  for (int i = 0; i < 10; i++) {
    lv_obj_t *p = get_row_panel(i);
    if (!p) continue;
    int gidx = page_base() + i;
    bool show = (i < LIST_ROWS_PER_PAGE) && (i < rows_on_page) && (gidx < LIST_MAX) && (gidx < GUI.visible_raws);
    if (show) lv_obj_clear_flag(p, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
  }
  list_sync_index_labels();
  // بعد از show/hide ردیف‌ها زردی تمام‌برش را همان لحظه بزن (نه فقط بعد از اسکرول)
  list_apply_visible_values_now();
}
//-------------------lable1_2 color change (CL-MAIN-01)--------------------------------
static lv_obj_t *main_pos_shadow_ring = NULL;
static lv_obj_t *main_pos_disk_obj = NULL;
static int8_t prev_main_in_pos = -1;
static uint8_t main_pos_count_flash_phase = 0;  // 0=off 1=red 2=green
static uint32_t main_pos_count_flash_until = 0;

static void main_pos_apply_state_color(uint32_t color) {
  main_fast_position_digits_set_color(color);
  if (!main_fast_position_digits_ready() && objects.lable1_2)
    lv_obj_set_style_text_color(objects.lable1_2, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
  if (main_pos_shadow_ring)
    lv_obj_set_style_bg_color(main_pos_shadow_ring, lv_color_hex(color),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void prcs_main_pos_count_flash() {
  if (GUI.lock_screen) {
    // حین دیالوگ اتمام لیست فلش رنگ را نگه دار
    if (GUI.flag_flash_count_dec) GUI.flag_flash_count_dec = false;
    return;
  }
  if (GUI.flag_flash_count_dec) {
    GUI.flag_flash_count_dec = false;
    main_pos_count_flash_phase = 1;
    main_pos_count_flash_until = millis() + 120UL;
    main_pos_apply_state_color(0xb90000u);
    usb_keyboard_set_numlock_led_force(false);  // چشمک چراغ کیبرد
    return;
  }
  if (main_pos_count_flash_phase == 0) return;
  if ((int32_t)(millis() - main_pos_count_flash_until) < 0) return;
  if (main_pos_count_flash_phase == 1) {
    main_pos_count_flash_phase = 2;
    main_pos_count_flash_until = millis() + 140UL;
    main_pos_apply_state_color(0x34C759u);
    usb_keyboard_set_numlock_led_force(true);
    return;
  }
  // پایان فلش → رنگ واقعی موقعیت + بازگردانی LED (عدد=روشن، جاگ=خاموش)
  main_pos_count_flash_phase = 0;
  prev_main_in_pos = -1;
  usb_keyboard_set_numlock_led(!GUI.NumLock);
}

void prcs_lable1_2_clr() {
  if (GUI.scr_id != 1) return;
  if (main_pos_count_flash_phase != 0) return;  // فلش تعداد اولویت دارد
  const int ppmm = (int)lroundf(axisPulsesPerMm());
  if (ppmm <= 0) return;
  // هدف بالا ثابت است؛ رسیدن بر اساس هدف واقعی موتور با اصلاح فعال سنجیده می‌شود.
  const int target_p = blade_err_motor_target_p(STP.Man_Sp_mm);
  const bool in_pos = STP.referenced &&
                      (abs(STP.currentPosition_p - target_p) < ppmm);
  if ((int8_t)in_pos == prev_main_in_pos) return;
  prev_main_in_pos = (int8_t)in_pos;
  main_pos_apply_state_color(in_pos ? 0x34C759u : 0xb90000u);
}
//-------------prcs edit mode lables in screen 3 --------------------------
bool prev_edit_mode_sp[10] = { false };
bool prev_edit_mode_count[10] = { false };
void set_label_color(lv_obj_t *label, bool active) {
  if (!label) return;
  // فوکوس ویرایش: آبی ملایم — هرگز سبز پررنگ
  if (!active) {
    // FOCUSED/PRESSED باقی‌مانده باعث کادر آبی دوم می‌شد
    lv_obj_clear_state(label, (lv_state_t)(LV_STATE_FOCUSED | LV_STATE_PRESSED));
  }
  lv_obj_set_style_text_color(label, lv_color_hex(active ? 0xffffff : 0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(label, lv_color_hex(active ? 0x0a84ff : 0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(label, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(label, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(label, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(label, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_set_style_bg_opa(label, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_FOCUSED);
}
void set_label_clear_color(lv_obj_t *label, bool active) {
  if (!label) return;
  lv_obj_set_style_text_color(label, lv_color_hex(active ? 0x111111 : 0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(label, lv_color_hex(active ? 0xffd84d : 0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(label, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
}
lv_obj_t *get_sp_label(int i) {
  switch (i) {
    case 0: return objects.lable3_1_1;
    case 1: return objects.lable3_1_2;
    case 2: return objects.lable3_1_3;
    case 3: return objects.lable3_1_4;
    case 4: return objects.lable3_1_5;
    case 5: return objects.lable3_1_6;
    case 6: return objects.lable3_1_7;
    case 7: return objects.lable3_1_8;
    case 8: return objects.lable3_1_9;
    case 9: return objects.lable3_1_10;
  }
  return NULL;
}
lv_obj_t *get_count_label(int i) {
  switch (i) {
    case 0: return objects.lable3_2_1;
    case 1: return objects.lable3_2_2;
    case 2: return objects.lable3_2_3;
    case 3: return objects.lable3_2_4;
    case 4: return objects.lable3_2_5;
    case 5: return objects.lable3_2_6;
    case 6: return objects.lable3_2_7;
    case 7: return objects.lable3_2_8;
    case 8: return objects.lable3_2_9;
    case 9: return objects.lable3_2_10;
  }
  return NULL;
}
void update_label_colors_screen3() {
  if (GUI.scr_id != 3) return;
  if (!GUI.flag_panel3_edit_labels_txt) return;  // Flag-controlled
  GUI.flag_panel3_edit_labels_txt = false;       // Reset flag
  int active_sp = -1;
  int active_count = -1;
  for (int i = 0; i < LIST_MAX; i++) {
    if (AUT.list_edit_mode_sp[i]) active_sp = i;
    if (AUT.list_edit_mode_count[i]) active_count = i;
  }
  bool any_active = (active_sp != -1 || active_count != -1);
  for (int i = 0; i < 10; i++) {
    int gidx = page_base() + i;
    bool show_sp = (any_active && gidx == active_sp);
    bool show_count = (any_active && gidx == active_count);
    bool show_clear = (GUI.clear_visual_idx >= 0 && gidx == GUI.clear_visual_idx);
    // همیشه اعمال شود — reflow/کیبورد استایل را پاک می‌کند
    set_label_color(get_sp_label(i), show_sp);
    prev_edit_mode_sp[i] = show_sp;
    if (GUI.clear_visual_idx >= 0) {
      set_label_clear_color(get_count_label(i), show_clear);
      prev_edit_mode_count[i] = false;
    } else {
      set_label_color(get_count_label(i), show_count);
      prev_edit_mode_count[i] = show_count;
    }
  }
}
void update_label_texts_sp() {
  if (GUI.scr_id != 3) return;
  if (!GUI.flag_update_labels_3_1) return;
  GUI.flag_update_labels_3_1 = false;
  // هم‌مسیر با تعداد: خط‌خوردگی/ضربدر ردیف تمام حفظ شود
  list_apply_visible_values_now();
}
void update_label_texts_count() {
  if (GUI.scr_id != 3) return;
  if (!GUI.flag_update_labels_3_2) return;
  GUI.flag_update_labels_3_2 = false;
  // فقط متن خام نگذار — خط‌خوردگی/ضربدر ردیف تمام هم باید اعمال شود
  list_apply_visible_values_now();
}
void prcs_label_texts_discount() {
}
void process_screen3_updates() {
  // Call all updates (they will self-skip if flags are false)
  update_label_colors_screen3();
  update_label_texts_sp();
  update_label_texts_count();
}
//--------------process direction animations -----------------
bool prev_motorIs_moving_2 = false;
static int prev_dir_anim_dir = 0;
void prcs_dir_anim() {
  const bool moving = STP.motorIs_moving;
  const int dir = STP.system_set_dir;
  // هم شروع/توقف و هم عوض‌شدن جهت حین حرکت (مثلاً مخالف وسط decel)
  if (moving == prev_motorIs_moving_2 && (!moving || dir == prev_dir_anim_dir)) return;
  prev_motorIs_moving_2 = moving;
  prev_dir_anim_dir = dir;
  // تقارن: چپ x=71 → راست 371 ؛ y=237 (۱۲px پایین‌تر از ۲۲۵)
  if (objects.anim1_2) lv_obj_set_pos(objects.anim1_2, 71, 237);
  if (objects.anim1_1) lv_obj_set_pos(objects.anim1_1, 371, 237);
  if (objects.anim1_1) lv_obj_add_flag(objects.anim1_1, LV_OBJ_FLAG_HIDDEN);
  if (objects.anim1_2) lv_obj_add_flag(objects.anim1_2, LV_OBJ_FLAG_HIDDEN);
  if (!moving) return;
  if (dir == 1) {
    if (objects.anim1_1) lv_obj_clear_flag(objects.anim1_1, LV_OBJ_FLAG_HIDDEN);
  } else if (dir == -1) {
    if (objects.anim1_2) lv_obj_clear_flag(objects.anim1_2, LV_OBJ_FLAG_HIDDEN);
  }
}
//---------------prcs tabview with f keys ------------------
void prcs_tabview_f_keys() {
  if (GUI.tab_view_select == 0) return;
  // نامپد لایهٔ top پیش از اولین paint تب بعدی بسته شود.
  numpad_close();
  // if leaving F3 tab (index 2), turn off all test outputs
  if (GUI.scr_id == 2 && lv_tabview_get_tab_act(objects.tabview2_1) == 2) {
    turn_off_test_outputs();
  }
  // قبل از تعویض تب: تغییرات ذخیره‌نشده برگردد
  settings_on_leave_settings();
  settings_hide_eez_clutter();
  lv_tabview_set_act(objects.tabview2_1, GUI.tab_view_select - 1, LV_ANIM_OFF);
  // VALUE_CHANGED → settings_on_tab_changed؛ کمربند ایمنی اگر رویداد نیاید:
  settings_hide_eez_clutter();
  settings_on_enter_settings();
  GUI.flag_reinit_settings = true;
  GUI.buzzer_cmd_short = true;
  GUI.tab_view_select = 0;
}
//-----------------prcs_error1_1----------------------
void prcs_cast_error1_1() {
  if (!GUI.flag_cast_error1_1) return;
  GUI.flag_cast_error1_1 = false;
  lv_obj_clear_flag(objects.panel1_1, LV_OBJ_FLAG_HIDDEN);
  GUI.lock_screen = true;
}
void prcs_remove_error1_1() {
  if (!GUI.flag_remove_error1_1) return;
  GUI.flag_remove_error1_1 = false;
  lv_obj_add_flag(objects.panel1_1, LV_OBJ_FLAG_HIDDEN);
  GUI.lock_screen = false;
}

// [UI/Task2] Task8 owns the machine state; this is only its coalesced operator feedback.
static void cut_safety_ui_tick() {
  const uint8_t notice = cut_safety_ui_notice_req;
  if (notice == CUT_UI_NOTICE_NONE) return;
  cut_safety_ui_notice_req = CUT_UI_NOTICE_NONE;

  const char *fa = NULL;
  const char *en = NULL;
  uint32_t duration = 3200;
  bool alarm = false;
  switch (notice) {
    case CUT_UI_NOTICE_UI_LOCKED:
      fa = "ابتدا پیام یا عملیات باز را کامل کنید"; en = "Finish the open message or operation first"; break;
    case CUT_UI_NOTICE_DEVICE_LOCKED:
      fa = "دستگاه قفل است؛ ابتدا قفل را برطرف کنید"; en = "Machine is locked; clear the lock first"; alarm = true; break;
    case CUT_UI_NOTICE_PINMAP:
      fa = "پایه‌های لازم برش در تنظیمات مدیر متصل نیست"; en = "Required cut I/O is not mapped in Admin settings"; alarm = true; break;
    case CUT_UI_NOTICE_STOP:
      fa = "استاپ فعال است؛ ابتدا استاپ را آزاد کنید"; en = "STOP is active; release STOP first"; alarm = true; break;
    case CUT_UI_NOTICE_GUARD_OPEN:
      fa = "درب محافظ باز است؛ آن را ببندید"; en = "Guard door is open; close it first"; alarm = true; break;
    case CUT_UI_NOTICE_BLADE_NOT_HOME:
      fa = "تیغه در موقعیت برگشت نیست؛ بازگشت تیغه را بررسی کنید"; en = "Blade is not at return position; check blade return"; alarm = true; break;
    case CUT_UI_NOTICE_MOTOR_FAULT:
      fa = "خطای موتور فعال است؛ درایو و موتور را بررسی کنید"; en = "Motor fault is active; check drive and motor"; alarm = true; break;
    case CUT_UI_NOTICE_HOLDER_ENGAGED:
      fa = "گیره فعال شد؛ هر دو استارت را نگه دارید"; en = "Holder engaged; keep both START keys pressed"; break;
    case CUT_UI_NOTICE_BLADE_LEAVE_TIMEOUT:
      fa = "تیغه از سنسور برگشت خارج نشد؛ جک و سنسور تیغه را بررسی کنید"; en = "Blade did not leave return sensor; check lift and sensor"; alarm = true; duration = 4500; break;
    case CUT_UI_NOTICE_BLADE_RETURN_TIMEOUT:
      fa = "تیغه برنگشت؛ گیره‌ها تا بازگشت تیغه آزاد نمی‌شوند"; en = "Blade did not return; clamps stay held until blade returns"; alarm = true; duration = 4500; break;
    case CUT_UI_NOTICE_BLADE_RETURNING:
      fa = "استارت رها شد؛ تیغه در حال برگشت است"; en = "START released; blade is returning"; break;
    case CUT_UI_NOTICE_BLADE_RETURNED:
      fa = "تیغه برگشت؛ با یک استارت برش را تأیید کنید"; en = "Blade returned; press one START to confirm the cut"; duration = 4200; break;
    case CUT_UI_NOTICE_FAULT_RECOVERED:
      fa = "تیغه برگشت؛ خطا رفع شد. هر دو استارت را رها و دوباره شروع کنید"; en = "Blade returned; fault recovered. Release both START keys, then restart"; duration = 4200; break;
    case CUT_UI_NOTICE_ANGLE_90_CONFIRM_NEEDED:
      fa = "لطفاً ابتدا زاویه ۹۰ درجه را روی صفحه تأیید کنید"; en = "Please confirm 90° angle on the screen first"; alarm = true; duration = 3000; break;
    default:
      return;
  }
  ui_toast(fa, en, duration);
  if (alarm) GUI.buzzer_cmd_long = true;
  else GUI.buzzer_cmd_short = true;
}
//-----------------prcs_error1_3----------------------
static uint32_t wrong_dir_start_ms           = 0;
static uint32_t wrong_dir_elapsed_at_pause   = 0;
static bool     wrong_dir_timer_active       = false;
static bool     wrong_dir_paused             = false;
static uint32_t panel_error_timeout_ms       = 7000;
static const uint32_t WRONG_DIR_TIMEOUT_MS   = 7000;

void prcs_cast_error1_3() {
  if (!GUI.flag_cast_error1_3) return;
  GUI.flag_cast_error1_3 = false;
  // خروج قطعی از مود لیست/بهینه
  AUT.selected_count_val = -1;
  AUT.temp_list_count_Val = 0;
  OPT.active = false;
  OPT.waiting_move = false;
  OPT.flag_refresh = true;
  GUI.update_lable1_3 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  if (objects.lable1_3) {
    layout_main_count_footer(true);
    lv_label_set_text(objects.lable1_3, "---");
  }
  opt_main_bar_lbl_hide();

  // مودال+lock_screen اینجا ناپایدار بود (پیام دیده نمی‌شد، +/تنظیمات قفل می‌ماند)
  // توست + مقصد=موقعیت فعلی = رفتار پایدار
  if (objects.panel1_3) lv_obj_add_flag(objects.panel1_3, LV_OBJ_FLAG_HIDDEN);
  if (objects.btn1_7) lv_obj_add_flag(objects.btn1_7, LV_OBJ_FLAG_HIDDEN);
  if (objects.bar1_1) lv_obj_add_flag(objects.bar1_1, LV_OBJ_FLAG_HIDDEN);
  if (objects.lbl_bar1_1) lv_obj_add_flag(objects.lbl_bar1_1, LV_OBJ_FLAG_HIDDEN);
  wrong_dir_timer_active = false;
  wrong_dir_paused = false;
  if (sensor_alert_is_open()) sensor_alert_close_now();
  sensor_alert_motion_lock = false;
  sensor_alert_kind = 0;
  cut_done_notify_req = false;
  GUI.lock_screen = false;
  GUI.flag_remove_error1_3 = false;

  main_restore_destination_to_current_pos();
  AUT.in_position_flag = true;
  STP.in_position_flag = true;
  prev_main_in_pos = -1;  // همگام رنگ سایه با موقعیت

  ui_toast("برش ليست تمام شد — جدول را پر كنيد",
           "Cut list finished — refill the table", 4500);
  GUI.buzzer_cmd_long = true;
}
void prcs_remove_error1_3() {
  if (!GUI.flag_remove_error1_3) return;
  GUI.flag_remove_error1_3    = false;
  wrong_dir_timer_active      = false;
  wrong_dir_paused            = false;
  wrong_dir_elapsed_at_pause  = 0;
  if (sensor_alert_is_open()) sensor_alert_close_now();
  sensor_alert_motion_lock    = false;
  sensor_alert_kind           = 0;
  STP.mvm_interupt = false;
  lv_bar_set_value(objects.bar1_1, 0, LV_ANIM_OFF);
  lv_obj_add_flag(objects.bar1_1,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.lbl_bar1_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.btn1_7,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.btn1_6,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.panel1_3,   LV_OBJ_FLAG_HIDDEN);
  GUI.lock_screen = false;
  if (GUI.inputBuffer_sp.length() == 0)
    main_restore_destination_to_current_pos();
}

//-----------------prcs_wrong_dir_error----------------------
void prcs_wrong_dir_error() {
  if (!ref_wrong_dir_flag) return;
  if (GUI.scr_id != 1) return;
  ref_wrong_dir_flag = false;
  const int which = (ref_wrong_dir_sensor == 2) ? 2 : 1;
  sensor_alert_open(1, which, 0);
}
void prcs_blade_guard_timeout_error() {
  if (!blade_guard_timeout_flag) return;
  blade_guard_timeout_flag = false;
  if (GUI.scr_id != 1) {
    // سوییچ خودکار صفحه نباید فوکوس پنج‌کلیده را به صفحهٔ اصلی قاچاق کند؛
    // وگرنه STOP فقط decrement می‌شود و هرگز کنسل/توقف نمی‌زند (شاخهٔ adjust در prcs_panel_stop_key)
    panel_five_nav_exit();
    GUI.scr_id = 1;
    loadScreen(SCREEN_ID_MAIN_1);
  }
  wrong_dir_paused = false;
  wrong_dir_elapsed_at_pause = 0;
  panel_error_timeout_ms = WRONG_DIR_TIMEOUT_MS;
  lv_label_set_text(objects.lablef1_6, "محافظ تیغه را پایین بیاورید");
  lv_label_set_text(objects.lablee1_6, "Lower the blade guard");
  lv_obj_add_flag(objects.btn1_6, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(objects.btn1_7, LV_OBJ_FLAG_HIDDEN);
  lv_bar_set_value(objects.bar1_1, 0, LV_ANIM_OFF);
  lv_obj_clear_flag(objects.bar1_1, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(objects.lbl_bar1_1, "0%");
  lv_obj_clear_flag(objects.lbl_bar1_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(objects.panel1_3, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(objects.panel1_3);
  wrong_dir_start_ms = lv_tick_get();
  wrong_dir_timer_active = true;
  GUI.lock_screen = true;
}
//-----------------پیام اصلاح خودکار کالیبراسیون بعد از برخورد با سنسور حد----------------------
void prcs_limit_recal_msg() {
  if (!limit_recal_flag) return;
  if (GUI.scr_id != 1) return;
  limit_recal_flag = false;
  const int which = (limit_recal_which == 1) ? 1 : 2;
  sensor_alert_open(2, which, limit_recal_err_mm);
}
void prcs_jog_soft_limit_toast() {
  if (!jog_soft_limit_flag) return;
  jog_soft_limit_flag = false;
  // حد نرم بدون برخورد سنسور فیزیکی → همان مودال (نه toast)
  const int which = (jog_soft_limit_which == 1) ? 1 : 2;
  sensor_alert_open(3, which, 0);
}
void prcs_wrong_dir_timer() {
  if (!wrong_dir_timer_active) return;

  // تشخیص لمس: فقط وقتی دست روی خود پنل ارور باشد
  bool is_touched = lv_obj_has_state(objects.panel1_3, LV_STATE_PRESSED);

  if (is_touched && !wrong_dir_paused) {
    // لحظه لمس — تایمر متوقف می‌شود
    wrong_dir_paused          = true;
    wrong_dir_elapsed_at_pause = lv_tick_elaps(wrong_dir_start_ms);
  } else if (!is_touched && wrong_dir_paused) {
    // لحظه رها کردن — تایمر از همان نقطه ادامه می‌یابد
    wrong_dir_paused  = false;
    wrong_dir_start_ms = lv_tick_get() - wrong_dir_elapsed_at_pause;
  }

  if (wrong_dir_paused) return;  // تایمر متوقف است — bar بدون تغییر

  uint32_t elapsed = lv_tick_elaps(wrong_dir_start_ms);
  if (elapsed >= panel_error_timeout_ms) {
    // اول bar را به دقیقاً 100% برسان، سپس در iteration بعد پنل بسته می‌شود
    lv_bar_set_value(objects.bar1_1, 100, LV_ANIM_OFF);
    lv_label_set_text(objects.lbl_bar1_1, "100%");
    wrong_dir_timer_active   = false;
    GUI.flag_remove_error1_3 = true;
    return;
  }
  // val را با clamp به [0,99] محدود کن تا قبل از اتمام به 100 نرسد
  int val = (int)(elapsed * 99UL / panel_error_timeout_ms);
  lv_bar_set_value(objects.bar1_1, val, LV_ANIM_OFF);
  char pct_buf[6];
  snprintf(pct_buf, sizeof(pct_buf), "%d%%", val);
  lv_label_set_text(objects.lbl_bar1_1, pct_buf);
}
//-----------------prcs panel1_2 clr------------------------
bool prev_NumLock = false;
void prcs_panel1_2_clr() {
  if (GUI.NumLock == prev_NumLock) return;
  prev_NumLock = GUI.NumLock;
  //-------------------------//
  lv_obj_set_style_bg_color(objects.panel1_2, lv_color_hex((!GUI.NumLock) ? 0xffe0e0e0 : 0xffaad4ff), LV_PART_MAIN | LV_STATE_DEFAULT);
  if (objects.lable1_1) {
    lv_obj_set_style_text_color(objects.lable1_1,
      lv_color_hex(GUI.NumLock ? 0x0a84ff : 0x0011d1), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  GUI.update_lable1_1 = true;  // جاگ: موقعیت زنده؛ عادی: مقدار هدف
  jog_manual_badge_sync();
}
//-----------------update lable2_5 (+ نمایش روی کادر آخر خط)------------------------
void update_lable_2_5() {
  if (!GUI.flag_update_lable2_5) return;
  GUI.flag_update_lable2_5 = false;
  lv_label_set_text(objects.lable2_5, GUI.inputBuffer_max_L.c_str());
  // کادر پایین F2 = آخر خط
  if (objects.lable2_3 && !STP.begin_calibration)
    lv_label_set_text(objects.lable2_3, GUI.inputBuffer_max_L.c_str());
}
//--------------prcs panel3-2-x clr--------------------
lv_obj_t *get_sel_panel(int i) {
  switch (i) {
    case 0: return objects.panel3_2_1;
    case 1: return objects.panel3_2_2;
    case 2: return objects.panel3_2_3;
    case 3: return objects.panel3_2_4;
    case 4: return objects.panel3_2_5;
    case 5: return objects.panel3_2_6;
    case 6: return objects.panel3_2_7;
    case 7: return objects.panel3_2_8;
    case 8: return objects.panel3_2_9;
    case 9: return objects.panel3_2_10;
  }
  return NULL;
}
void update_panel3_2_x_clr() {
  if (!GUI.flag_update_panel3_2_x_clr) return;
  GUI.flag_update_panel3_2_x_clr = false;
  for (int i = 0; i < 10; i++) {
    lv_obj_t *p = get_sel_panel(i);
    if (!p) continue;
    const int gidx = page_base() + i;
    bool sel = (AUT.selected_count_val >= 0 && gidx == AUT.selected_count_val);
    bool depleted = false;
    if (gidx >= 0 && gidx < LIST_MAX) {
      const float spv = AUT.list_sp_Val_p[gidx];
      const int cur = AUT.list_count_Val[gidx];
      int base = AUT.list_count_base_Val[gidx];
      if (base < cur) base = cur;
      const bool editing =
          AUT.list_edit_mode_sp[gidx] || AUT.list_edit_mode_count[gidx];
      depleted = (!editing && spv > 0.05f && base > 0 &&
                  cur == 0 && base > cur);
    }
    uint32_t c = sel ? 0xFFD600u : (depleted ? 0x8E8E93u : 0x34C759u);
    lv_obj_set_style_bg_color(p, lv_color_hex(c), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (lv_obj_get_child_count(p) > 0) {
      lv_obj_t *il = lv_obj_get_child(p, 0);
      lv_obj_set_style_text_color(il, lv_color_hex(sel ? 0x1C1C1E : 0xffffff),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
  // ردیف‌های تمام‌برش و هایلایت زرد ردیف فعال: اعمال پس‌زمینه
  if (GUI.scr_id == 3) list_apply_visible_values_now();
}
//*******************************************************
void prcs_menu_key_save() {
  if (!GUI.flag_save_key) return;
  GUI.flag_save_key = false;
  GUI.flag_cancl_key = false;
  if (f3_pinmap_dialog_is_open()) {
    panel_five_nav_exit();
    f3_pinmap_panel_confirm();
    return;
  }
  if (panel_five_nav_is_active()) panel_five_nav_exit();
  if (GUI.scr_id == 3) {
    flush_pending_list_saves_now();
    if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
    ui_toast("لیست ذخیره شد", "List saved", 1200);
    return;
  }
  if (GUI.scr_id != 2) {
    if (GUI.flag_save_all) {
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
      ui_toast("تنظیمات ذخیره شد", "Settings saved", 1200);
    }
    return;
  }
  //
  PEND.global_speed = map(lv_slider_get_value(objects.slider2_1), 0, 1000, 100, 8);
  auto_ramp_ms = (int)map(lv_slider_get_value(objects.slider2_2), 0, 1000, 0, 5000);
  STP.global_lowSpeedTime = auto_ramp_ms;
  PEND.global_init_speed = (auto_ramp_ms > 0) ? 400 : STP.global_speed;
  // جاگ همان سرعت اتوماتیک را بگیرد
  jog_speed_interval = constrain(PEND.global_speed, 8, 100);
  jog_ramp_ms = f2_read_jog_ramp_ms();
  jog_init_speed = (jog_ramp_ms > 0) ? 400 : 0;
  panel_save_feedback_pending = true;
  GUI.flag_save_all = true;  // همان مسیر Save لمسی
  if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_SUCCESS);
  numpad_close();
  if (g_oobe_done) GUI.pending_goto_scr = 1;
}
void prcs_menu_key_cancl() {
  if (!GUI.flag_cancl_key) return;
  GUI.flag_cancl_key = false;
  GUI.flag_save_key = false;
  if (f3_pinmap_panel_cancel()) {
    panel_five_nav_exit();
    return;
  }
  if (panel_five_nav_is_active()) {
    panel_five_nav_cancel_level();
    GUI.buzzer_cmd_short = true;
    return;
  }
  if (GUI.scr_id == 0 && ref_skip_panel_is_open()) {
    ref_homing_confirm_skip_no();
    return;
  }
  if (ref_manual_pos_prompt_is_open()) {
    GUI.buzzer_cmd_short = true;
    lv_async_call(ref_manual_pos_close_async, NULL);
    return;
  }
  if (GUI.scr_id == 3) {
    GUI.buzzer_cmd_short = true;
    numpad_close();
    GUI.scr_id = 1;
    loadScreen(SCREEN_ID_MAIN_1);
    return;
  }
  if (GUI.scr_id != 2) return;
  if (!oobe_guard_leave_screen()) return;
  //
  GUI.buzzer_cmd_short = true;
  settings_on_leave_settings();
  numpad_close();
  GUI.scr_id = 1;
  loadScreen(SCREEN_ID_MAIN_1);
}
// کلید START پنل (آنالوگ pin2) — حرکت استپر؛ استارت۱/۲ فقط برش‌اند
void prcs_panel_start_key() {
  if (!GUI.flag_start_key) return;
  GUI.flag_start_key = false;
  if (cut_start_wrong_screen_modal_active()) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (panel_five_nav_is_active()) {
    GUI.buzzer_cmd_short = true;
    panel_five_nav_start_action();
    return;
  }
  if (GUI.lock_screen) {
    GUI.flag_abort_opt_calc = true;
    GUI.buzzer_cmd_long = true;
    return;
  }
  // صفحه موقعیت‌یابی: همان تاچ سبز؛ اگر پیغام بله/انصراف باز است → بله
  if (GUI.scr_id == 0) {
    if (ref_skip_panel_is_open()) {
      ref_homing_confirm_skip_yes();
      return;
    }
    ref_homing_try_start();
    return;
  }
  if (GUI.scr_id != 1) return;
  if (STP.flag_is_cutting || STP.begin_calibration) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  // مسیر کامل Enter اصلی (جاگ/صف/حرکت) — تعریف در Task4
  extern void main_mirror_keyboard_enter_on_main();
  main_mirror_keyboard_enter_on_main();
}

// کلید STOP پنل (آنالوگ pin2) — توقف فوری حرکت استپر
void prcs_panel_stop_key() {
  if (!GUI.flag_stop_key) return;
  GUI.flag_stop_key = false;
  // در صفحهٔ موقعیت‌یابی، STOP باید همیشه اولویت داشته باشد؛ حتی اگر فوکوس پنج‌کلیده باقی مانده باشد.
  if (GUI.scr_id == 0 && panel_five_nav_is_active()) panel_five_nav_exit();
  if (f3_stepper_test_busy || f3_stepper_test_cmd) {
    extern void panel_request_stop_motion(void);
    panel_request_stop_motion();
    GUI.buzzer_cmd_short = true;
    return;
  }
  if (panel_five_nav_is_active()) {
    GUI.buzzer_cmd_short = true;
    panel_five_nav_stop_action();
    return;
  }
  GUI.flag_abort_opt_calc = true;
  GUI.flag_close_numpad = true;
  // صفحه موقعیت‌یابی: حین جایابی = قطع فوری؛ پیغام باز = انصراف؛ وگرنه = ضربدر
  if (GUI.scr_id == 0) {
    if (ref_skip_panel_is_open()) {
      ref_homing_confirm_skip_no();
      return;
    }
    const bool homing =
        STP.motorIs_moving ||
        (objects.spinner0_1 && !lv_obj_has_flag(objects.spinner0_1, LV_OBJ_FLAG_HIDDEN));
    if (homing) {
      extern void panel_request_stop_motion(void);
      panel_request_stop_motion();
      ref_homing_restore_ready_ui();
      GUI.buzzer_cmd_short = true;
      return;
    }
    if (ref_error_ui_active) {
      ref_homing_restore_ready_ui();
      GUI.buzzer_cmd_short = true;
      return;
    }
    // در حالت آماده، STOP پنل همان نقش ضربدر قرمز را دارد.
    ref_homing_open_skip_panel();
    return;
  }
  const bool was_moving = STP.motorIs_moving || STP.moveStepperCMD ||
                          keyboardJogActive || STP.jogEnabled;
  extern void panel_request_stop_motion(void);
  panel_request_stop_motion();
  if (was_moving) flag_panel_stop_motion_toast = true;
  GUI.buzzer_cmd_short = true;
}
//-----------------کلید MODE (F3) — ناوبری کامل بدون تاچ----------------------
void prcs_mode_key() {
  if (!GUI.flag_mode_key) return;
  GUI.flag_mode_key = false;
  if (cut_start_wrong_screen_modal_active()) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if ((GUI.lock_screen && !f3_pinmap_dialog_is_open() && !panel_five_nav_modal_available()) ||
      OPT.active) { GUI.buzzer_cmd_long = true; return; }
  if (STP.motorIs_moving || STP.flag_is_cutting) { GUI.buzzer_cmd_long = true; return; }
  GUI.buzzer_cmd_short = true;
  panel_five_nav_mode_next();
}

static void prcs_panel_nav_external_exit() {
  if (!GUI.flag_panel_nav_exit_key) return;
  GUI.flag_panel_nav_exit_key = false;
  if (panel_five_nav_is_active()) {
    panel_five_nav_exit();
    GUI.buzzer_cmd_short = true;
  }
}

// F1..F5 روی صفحهٔ اصلی: رها کردن کلید = فراخوانی عدد ذخیره‌شده به مقصد (خارج از جاگ)
static void prcs_main_f_mem_keys() {
  int edge = panel_f_press_edge;
  if (edge == 0) return;
  panel_f_press_edge = 0;
  if (panel_five_nav_is_active()) {
    panel_five_nav_exit();
    GUI.buzzer_cmd_short = true;
    return;
  }
  if (GUI.scr_id != 1) return;
  if (GUI.NumLock) return;  // جاگ: فقط استاپ/جهت — حافظه نه
  GUI.flag_close_numpad = true;  // هر کلید پنل F → بستن نامپد مقصد
  if (GUI.lock_screen || STP.flag_is_cutting || STP.begin_calibration) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (STP.motorIs_moving) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (edge < 1 || edge > 5) return;
  int idx = edge - 1;
  if (!main_f_mem_valid[idx]) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  float mm = main_f_mem_mm[idx];
  char buf[16];
  if (fabsf(mm - roundf(mm)) < 0.05f) snprintf(buf, sizeof(buf), "%d", (int)lroundf(mm));
  else snprintf(buf, sizeof(buf), "%.1f", (double)mm);
  if (xSemaphoreTake(lable1_1_Semaphore, (TickType_t)10) == pdTRUE) {
    GUI.inputBuffer_sp = buf;
    xSemaphoreGive(lable1_1_Semaphore);
  } else {
    GUI.inputBuffer_sp = buf;
  }
  STP.Man_Sp_mm = mm;
  jog_keep_dest = false;
  AUT.in_position_flag = false;
  STP.in_position_flag = false;
  GUI.update_lable1_1 = true;
  GUI.edit_mode_lable1_1 = true;
  clear_buffer_sp_flag = true;
  ui_toast("عدد ذخیره شده در مقصد قرار گرفت", "Saved value loaded to target", 2500);
  GUI.buzzer_cmd_short = true;
}

static void prcs_save_main_f_mem() {
  if (!flag_save_main_f_mem) return;
  flag_save_main_f_mem = false;
  int idx = pending_main_f_mem_idx;
  pending_main_f_mem_idx = -1;
  if (idx >= 0 && idx <= 4) save_main_f_mem(idx);
}
void uncheck_saw_output_btns() {
  if (GUI.scr_id != 2) return;
  if (!GUI.flag_uncheck_saw_btns) return;
  GUI.flag_uncheck_saw_btns = false;
  //-------------------
  // lv_obj_clear_state(objects.btn2_3_1, LV_STATE_CHECKED);
  lv_obj_clear_state(objects.btn2_3_3, LV_STATE_CHECKED);
  // lv_obj_clear_state(objects.btn2_3_5, LV_STATE_CHECKED);
}
void check_saw_output_btns() {
  if (GUI.scr_id != 2) return;
  if (!GUI.flag_check_saw_btns) return;
  GUI.flag_check_saw_btns = false;
  //-------------------
  lv_obj_add_state(objects.btn2_3_3, LV_STATE_CHECKED);
}
void cast_img_emg() {
  if (!GUI.flag_cast_emg) return;
  GUI.flag_cast_emg = false;
  // فقط صفحهٔ اصلی و موقعیت‌یابی — روی F1/لیست و بقیه هرگز
  if ((GUI.scr_id != 0 && GUI.scr_id != 1) || !IOS.stop_button_state) {
    if (objects.emg != NULL && lv_obj_is_valid(objects.emg)) {
      lv_obj_del(objects.emg);
      objects.emg = NULL;
    }
    return;
  }
  if (GUI.buzzer_enable) GUI.buzzer_cmd_long = true;
  if (objects.roller2_1) lv_obj_add_state(objects.roller2_1, LV_STATE_DISABLED);
  if (objects.roller2_2) lv_obj_add_state(objects.roller2_2, LV_STATE_DISABLED);
  if (objects.emg) {
    lv_obj_del(objects.emg);
    objects.emg = NULL;
  }
  {
    // آیکن استاپ بزرگ روی لایهٔ top
    lv_obj_t *obj = lv_img_create(lv_layer_top());
    objects.emg = obj;
    lv_img_set_src(obj, &img_emg);
    const int emg_sz = 220;
    const int emg_src = (img_emg.header.w > 0) ? (int)img_emg.header.w : 256;
    lv_obj_set_size(obj, emg_sz, emg_sz);
    lv_obj_set_pos(obj, (480 - emg_sz) / 2, (320 - emg_sz) / 2);
    lv_image_set_scale(obj, (int)(256.0f * emg_sz / (float)emg_src));
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(obj);
  }
}
void delete_img_emg() {
  if (!GUI.flag_del_emg) return;
  GUI.flag_del_emg = false;
  lv_obj_clear_state(objects.roller2_1, LV_STATE_DISABLED);
  lv_obj_clear_state(objects.roller2_2, LV_STATE_DISABLED);
  if (objects.emg != NULL && lv_obj_is_valid(objects.emg)) {
    lv_obj_del(objects.emg);
    objects.emg = NULL;
    if (GUI.buzzer_enable) GUI.buzzer_cmd_short = true;
  }
}
//-----------------دیالوگ تأیید ریست فکتوری (تایمر ۵ثانیه؛ فقط تأیید در مهلت = ریست)----------------------
static lv_obj_t *factory_reset_mbox = NULL;
static lv_obj_t *factory_reset_bar = NULL;
static uint32_t factory_reset_t0 = 0;
static bool factory_reset_armed = false;  // هنوز داخل مهلت تأیید

static void factory_reset_dialog_close() {
  if (factory_reset_mbox && lv_obj_is_valid(factory_reset_mbox))
    lv_obj_delete_async(factory_reset_mbox);
  factory_reset_mbox = factory_reset_bar = NULL;
  factory_reset_armed = false;
  GUI.lock_screen = false;
}

static void factory_reset_yes_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  if (!factory_reset_armed) return;  // بعد از اتمام تایمر ریست نکن
  GUI.buzzer_cmd_long = true;
  factory_reset_armed = false;
  factory_reset();  // restart — برنمی‌گردد
}
static void factory_reset_no_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  GUI.buzzer_cmd_short = true;
  factory_reset_dialog_close();
}

static void factory_reset_dialog_tick() {
  if (!factory_reset_mbox || !lv_obj_is_valid(factory_reset_mbox)) return;
  const uint32_t MS = 10000;
  uint32_t el = millis() - factory_reset_t0;
  if (factory_reset_bar && lv_obj_is_valid(factory_reset_bar)) {
    int w = (int)((el >= MS) ? 400 : (400UL * el) / MS);
    lv_obj_set_width(factory_reset_bar, w);
  }
  if (el >= MS && factory_reset_armed) {
    // مهلت تمام → بستن بدون ریست
    factory_reset_armed = false;
    factory_reset_dialog_close();
  }
}

void prcs_factory_reset_dialog() {
  factory_reset_dialog_tick();
  if (!GUI.flag_factory_reset_dialog) return;
  GUI.flag_factory_reset_dialog = false;
  if (factory_reset_mbox && lv_obj_is_valid(factory_reset_mbox)) return;
  const bool en = (GUI.system_language == 2);
  const bool has_fac = factory_snapshot_exists();

  factory_reset_mbox = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(factory_reset_mbox, 0, 0);
  lv_obj_set_size(factory_reset_mbox, 480, 320);
  lv_obj_set_style_bg_color(factory_reset_mbox, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(factory_reset_mbox, LV_OPA_50, 0);
  lv_obj_set_style_border_width(factory_reset_mbox, 0, 0);
  lv_obj_set_style_pad_all(factory_reset_mbox, 0, 0);
  lv_obj_clear_flag(factory_reset_mbox, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *card = lv_obj_create(factory_reset_mbox);
  lv_obj_set_size(card, 432, 250);
  lv_obj_center(card);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_width(title, 400);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(title, en ? &lv_font_montserrat_18 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x1c1c1e), 0);
  if (!en) lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, 0);
  lv_label_set_text(title, en ? "Restore factory settings" : "بازگردانی به تنظیمات کارخانه");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *body = lv_label_create(card);
  lv_obj_set_width(body, 400);
  lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(body, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(body, lv_color_hex(0x3a3a3c), 0);
  if (!en) lv_obj_set_style_base_dir(body, LV_BASE_DIR_RTL, 0);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  if (en) {
    lv_label_set_text(body,
      has_fac
        ? "Tap Save to store current changes.\n"
          "Hold Save to restore the admin factory snapshot.\n"
          "Cut counters and warranty remain unchanged.\n"
          "Confirm within 10 seconds."
        : "Tap Save to store current changes.\n"
          "Hold Save to restore built-in defaults.\n"
          "Cut counters and warranty remain unchanged.\n"
          "Confirm within 10 seconds.");
  } else {
    lv_label_set_text(body,
      has_fac
        ? "لمس کوتاه، تغییرات فعلی را ذخیره می‌کند.\n"
          "نگه‌داشتن، تنظیمات کارخانه مدیر را برمی‌گرداند.\n"
          "شمارنده برش و گارانتی پاک نمی‌شود.\n"
          "برای ادامه تا ده ثانیه «بله» را بزنید."
        : "لمس کوتاه، تغییرات فعلی را ذخیره می‌کند.\n"
          "نگه‌داشتن، مقدار اولیه برنامه را برمی‌گرداند.\n"
          "شمارنده برش و گارانتی پاک نمی‌شود.\n"
          "برای ادامه تا ده ثانیه «بله» را بزنید.");
  }
  lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 40);

  factory_reset_bar = lv_obj_create(card);
  lv_obj_set_pos(factory_reset_bar, 16, 180);
  lv_obj_set_size(factory_reset_bar, 1, 8);
  lv_obj_set_style_bg_color(factory_reset_bar, lv_color_hex(0xFF9500), 0);
  lv_obj_set_style_radius(factory_reset_bar, 4, 0);
  lv_obj_set_style_border_width(factory_reset_bar, 0, 0);
  lv_obj_clear_flag(factory_reset_bar, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *bn = lv_button_create(card);
  lv_obj_set_size(bn, 150, 42);
  lv_obj_align(bn, LV_ALIGN_BOTTOM_LEFT, 16, -12);
  lv_obj_set_style_bg_color(bn, lv_color_hex(0x34C759), 0);
  lv_obj_set_style_radius(bn, 10, 0);
  lv_obj_add_event_cb(bn, factory_reset_no_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *ln = lv_label_create(bn);
  lv_label_set_text(ln, en ? "Cancel" : "انصراف");
  lv_obj_set_style_text_font(ln, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(ln, lv_color_hex(0xffffff), 0);
  lv_obj_center(ln);

  lv_obj_t *by = lv_button_create(card);
  lv_obj_set_size(by, 150, 42);
  lv_obj_align(by, LV_ALIGN_BOTTOM_RIGHT, -16, -12);
  lv_obj_set_style_bg_color(by, lv_color_hex(0xFF3B30), 0);
  lv_obj_set_style_radius(by, 10, 0);
  lv_obj_add_event_cb(by, factory_reset_yes_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *ly = lv_label_create(by);
  lv_label_set_text(ly, en ? "Yes" : "بله");
  lv_obj_set_style_text_font(ly, en ? &lv_font_montserrat_16 : &ui_font_naz20b, 0);
  lv_obj_set_style_text_color(ly, lv_color_hex(0xffffff), 0);
  lv_obj_center(ly);

  factory_reset_t0 = millis();
  factory_reset_armed = true;
  GUI.lock_screen = true;
}
//-----------------پورتال WiFi (روشن/خاموش SoftAP وب‌اپ)----------------------
static void web_portal_apply_toggle(bool want_on) {
  if (STP.motorIs_moving || STP.flag_is_cutting || IOS.saw_is_on) {
    GUI.buzzer_cmd_long = true;
    return;
  }
  if (web_portal_transitioning) {
    GUI.buzzer_cmd_long = true;
    ui_toast("وای فای در حال تغییر وضعیت است",
             "WiFi is changing state", 2200);
    return;
  }
  if (want_on) {
    if (g_update_mode || update_flow_network_busy()) {
      GUI.buzzer_cmd_long = true;
      ui_toast("شبکه در اختیار بروزرسانی است",
               "WiFi is busy with the updater", 3500);
      return;
    }
    if (web_portal_running) {
      GUI.flag_web_portal_chip_refresh = true;
      ui_toast_wifi_ready(ap_ssid, ap_pass);
      return;
    }
    web_portal_ap_result = 0;
    web_portal_stop_request = false;
    web_portal_start_request = true;
    if (!startWebPortalTask()) {
      web_portal_start_request = false;
      GUI.buzzer_cmd_long = true;
      ui_toast("خطای حافظه پورتال", "Not enough memory for portal", 4000);
    } else {
      ui_toast("در حال روشن کردن وای فای...", "Starting WiFi AP...", 2000);
    }
  } else {
    if (web_portal_ota_active) {
      GUI.buzzer_cmd_long = true;
      ui_toast("نصب برنامه در حال انجام است",
               "Firmware install is in progress", 3500);
      return;
    }
    web_portal_start_request = false;
    if (!web_portal_running && webPortalTaskHandle == NULL) {
      GUI.flag_web_portal_chip_refresh = true;
      return;
    }
    web_portal_stop_request = true;
    ui_toast("وای فای خاموش شد", "WiFi portal stopped", 2500);
  }
  GUI.flag_web_portal_chip_refresh = true;
}
void prcs_web_portal_toggle() {
  if (!GUI.flag_web_portal_toggle) return;
  GUI.flag_web_portal_toggle = false;
  web_portal_apply_toggle(!web_portal_running);
}
void prcs_web_portal_chip_refresh() {
  if (!GUI.flag_web_portal_chip_refresh) return;
  GUI.flag_web_portal_chip_refresh = false;
  f1_sync_wifi_chip();
}

extern void web_apply_pending_now();
extern void web_discard_pending_now();
extern uint8_t web_pending_kind;
extern uint32_t web_pending_ms;
extern bool web_policy_ask_request;
extern uint32_t web_portal_started_ms;
extern uint8_t web_save_policy;
extern void save_web_save_policy();

static lv_obj_t *web_policy_modal_root = NULL;
static lv_obj_t *web_confirm_modal_root = NULL;

static void web_policy_modal_close() {
  if (web_policy_modal_root && lv_obj_is_valid(web_policy_modal_root)) {
    lv_obj_delete(web_policy_modal_root);
  }
  web_policy_modal_root = NULL;
  web_policy_ask_request = false;
}

static void web_policy_auto_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  web_save_policy = 1;
  save_web_save_policy();
  web_policy_modal_close();
  GUI.buzzer_cmd_short = true;
  ui_toast("ذخیره خودکار تغییرات وب فعال شد", "Web auto-save enabled", 2000);
}

static void web_policy_confirm_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  web_save_policy = 2;
  save_web_save_policy();
  web_policy_modal_close();
  GUI.buzzer_cmd_short = true;
  ui_toast("تأیید هر تغییر از وب فعال شد", "Web confirm-each enabled", 2000);
}

static void web_policy_modal_show() {
  if (web_policy_modal_root && lv_obj_is_valid(web_policy_modal_root)) return;
  const bool en = (GUI.system_language == 2);
  web_policy_modal_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(web_policy_modal_root, 480, 320);
  lv_obj_set_pos(web_policy_modal_root, 0, 0);
  lv_obj_set_style_bg_color(web_policy_modal_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(web_policy_modal_root, LV_OPA_70, 0);
  lv_obj_set_style_border_width(web_policy_modal_root, 0, 0);
  lv_obj_set_style_pad_all(web_policy_modal_root, 0, 0);
  lv_obj_clear_flag(web_policy_modal_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(web_policy_modal_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(web_policy_modal_root);
  lv_obj_set_size(card, 380, 170);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_obj_set_width(t, 356);
  lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(t, en ? &lv_font_montserrat_20 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0x1c1c1e), 0);
  lv_label_set_text(t, en ? "Web Save" : "ذخیره در وب");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 6);

  lv_obj_t *b = lv_label_create(card);
  lv_obj_set_width(b, 356);
  lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(b, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(b, lv_color_hex(0x3a3a3c), 0);
  lv_label_set_text(b, en ? "Allow saving data in the web app?" : "اجازه می‌دهید اطلاعات در اپلیکیشن وب ذخیره شود؟");
  lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 48);

  lv_obj_t *btn_auto = lv_button_create(card);
  lv_obj_set_size(btn_auto, 160, 44);
  lv_obj_set_pos(btn_auto, 24, 90);
  lv_obj_set_style_bg_color(btn_auto, lv_color_hex(0x1c9b67), 0);
  lv_obj_set_style_radius(btn_auto, 10, 0);
  lv_obj_set_style_border_width(btn_auto, 0, 0);
  lv_obj_add_event_cb(btn_auto, web_policy_auto_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *al = lv_label_create(btn_auto);
  lv_label_set_text(al, en ? "Auto-Save" : "ذخیره خودکار");
  lv_obj_set_style_text_font(al, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(al, lv_color_hex(0xffffff), 0);
  lv_obj_center(al);

  lv_obj_t *btn_conf = lv_button_create(card);
  lv_obj_set_size(btn_conf, 160, 44);
  lv_obj_set_pos(btn_conf, 196, 90);
  lv_obj_set_style_bg_color(btn_conf, lv_color_hex(0x8e8e93), 0);
  lv_obj_set_style_radius(btn_conf, 10, 0);
  lv_obj_set_style_border_width(btn_conf, 0, 0);
  lv_obj_add_event_cb(btn_conf, web_policy_confirm_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(btn_conf);
  lv_label_set_text(cl, en ? "Confirm Each" : "تأیید هر تغییر");
  lv_obj_set_style_text_font(cl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), 0);
  lv_obj_center(cl);
}

static void web_confirm_modal_close() {
  if (web_confirm_modal_root && lv_obj_is_valid(web_confirm_modal_root)) {
    lv_obj_delete(web_confirm_modal_root);
  }
  web_confirm_modal_root = NULL;
}

static void web_confirm_save_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  web_apply_pending_now();
  web_confirm_modal_close();
  GUI.buzzer_cmd_short = true;
  ui_toast("تغییرات وب ذخیره شد", "Web changes saved", 2000);
}

static void web_confirm_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  web_discard_pending_now();
  web_confirm_modal_close();
  GUI.buzzer_cmd_short = true;
  ui_toast("تغییرات وب لغو شد", "Web changes discarded", 2000);
}

static void web_confirm_modal_show(uint8_t kind) {
  if (web_confirm_modal_root && lv_obj_is_valid(web_confirm_modal_root)) return;
  const bool en = (GUI.system_language == 2);
  web_confirm_modal_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(web_confirm_modal_root, 480, 320);
  lv_obj_set_pos(web_confirm_modal_root, 0, 0);
  lv_obj_set_style_bg_color(web_confirm_modal_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(web_confirm_modal_root, LV_OPA_70, 0);
  lv_obj_set_style_border_width(web_confirm_modal_root, 0, 0);
  lv_obj_set_style_pad_all(web_confirm_modal_root, 0, 0);
  lv_obj_clear_flag(web_confirm_modal_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(web_confirm_modal_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(web_confirm_modal_root);
  lv_obj_set_size(card, 380, 170);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_obj_set_width(t, 356);
  lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(t, en ? &lv_font_montserrat_20 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0x1c1c1e), 0);
  lv_label_set_text(t, en ? "Web App Change Request" : "درخواست تغییر از اپ وب");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 6);

  lv_obj_t *b = lv_label_create(card);
  lv_obj_set_width(b, 356);
  lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(b, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(b, lv_color_hex(0x3a3a3c), 0);
  if (kind == 1) {
    lv_label_set_text(b, en ? "Settings modified from web. Save now?" : "تنظیمات از اپ وب تغییر کرده است. ذخیره شود؟");
  } else {
    lv_label_set_text(b, en ? "Cut list modified from web. Save now?" : "لیست برش از اپ وب تغییر کرده است. ذخیره شود؟");
  }
  lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 48);

  lv_obj_t *btn_save = lv_button_create(card);
  lv_obj_set_size(btn_save, 160, 44);
  lv_obj_set_pos(btn_save, 24, 112);
  lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x1c9b67), 0);
  lv_obj_set_style_radius(btn_save, 10, 0);
  lv_obj_set_style_border_width(btn_save, 0, 0);
  lv_obj_add_event_cb(btn_save, web_confirm_save_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *sl = lv_label_create(btn_save);
  lv_label_set_text(sl, en ? "Save" : "ذخیره");
  lv_obj_set_style_text_font(sl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(sl, lv_color_hex(0xffffff), 0);
  lv_obj_center(sl);

  lv_obj_t *btn_cancel = lv_button_create(card);
  lv_obj_set_size(btn_cancel, 160, 44);
  lv_obj_set_pos(btn_cancel, 196, 112);
  lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x8e8e93), 0);
  lv_obj_set_style_radius(btn_cancel, 10, 0);
  lv_obj_set_style_border_width(btn_cancel, 0, 0);
  lv_obj_add_event_cb(btn_cancel, web_confirm_cancel_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(btn_cancel);
  lv_label_set_text(cl, en ? "Discard" : "انصراف");
  lv_obj_set_style_text_font(cl, en ? &lv_font_montserrat_16 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), 0);
  lv_obj_center(cl);
}

void prcs_web_confirm_gate() {
  if (web_policy_ask_request && web_portal_running && web_save_policy == 0) {
    if (millis() - web_portal_started_ms >= 5000) {
      web_policy_ask_request = false;
      web_policy_modal_show();
    }
  }

  if (web_pending_kind != 0) {
    if (!web_portal_running || (millis() - web_pending_ms > 60000)) {
      web_discard_pending_now();
      web_confirm_modal_close();
      ui_toast("درخواست وب منقضی شد", "Web request expired", 2000);
    } else if (!web_confirm_modal_root) {
      web_confirm_modal_show(web_pending_kind);
    }
  } else if (web_confirm_modal_root) {
    web_confirm_modal_close();
  }
}

static lv_obj_t *angle_90_modal_root = NULL;

static void angle_90_modal_close() {
  if (angle_90_modal_root && lv_obj_is_valid(angle_90_modal_root)) {
    lv_obj_delete(angle_90_modal_root);
  }
  angle_90_modal_root = NULL;
  cut_angle_90_prompt_active = false;
}

static void angle_90_confirm_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  cut_angle_90_confirmed = true;
  angle_90_modal_close();
  GUI.buzzer_cmd_short = true;
  ui_toast("زاویه ۹۰ درجه تأیید شد — آماده برش", "90° angle confirmed — Ready to cut", 2000);
}

static void angle_90_cancel_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
  cut_angle_90_confirmed = false;
  angle_90_modal_close();
  GUI.buzzer_cmd_short = true;
}

static void angle_90_modal_show() {
  if (angle_90_modal_root && lv_obj_is_valid(angle_90_modal_root)) return;
  const bool en = (GUI.system_language == 2);
  cut_angle_90_prompt_active = true;
  angle_90_modal_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(angle_90_modal_root, 480, 320);
  lv_obj_set_pos(angle_90_modal_root, 0, 0);
  lv_obj_set_style_bg_color(angle_90_modal_root, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(angle_90_modal_root, LV_OPA_70, 0);
  lv_obj_set_style_border_width(angle_90_modal_root, 0, 0);
  lv_obj_set_style_pad_all(angle_90_modal_root, 0, 0);
  lv_obj_clear_flag(angle_90_modal_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(angle_90_modal_root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *card = lv_obj_create(angle_90_modal_root);
  lv_obj_set_size(card, 400, 185);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 28);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_obj_set_width(t, 376);
  lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(t, en ? &lv_font_montserrat_20 : &ui_font_naz24, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0x1c1c1e), 0);
  lv_label_set_text(t, en ? "90° Angle Setup" : "تنظیم زاویه ۹۰ درجه");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);

  lv_obj_t *b = lv_label_create(card);
  lv_obj_set_width(b, 376);
  lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(b, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(b, lv_color_hex(0x3a3a3c), 0);
  lv_label_set_text(b, en ? "45° cylinder is released.\nPlease set machine to 90° and confirm."
                          : "شیر برقی جک ۴۵° آزاد شد.\nدستگاه را روی ۹۰° تنظیم کرده و تأیید را بزنید.");
  lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 46);

  lv_obj_t *btn_conf = lv_button_create(card);
  lv_obj_set_size(btn_conf, 175, 44);
  lv_obj_set_pos(btn_conf, 195, 120);
  lv_obj_set_style_bg_color(btn_conf, lv_color_hex(0x1c9b67), 0);
  lv_obj_set_style_radius(btn_conf, 10, 0);
  lv_obj_set_style_border_width(btn_conf, 0, 0);
  lv_obj_add_event_cb(btn_conf, angle_90_confirm_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *cl = lv_label_create(btn_conf);
  lv_label_set_text(cl, en ? "Confirm & Ready" : "تأیید و آماده برش");
  lv_obj_set_style_text_font(cl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(cl, lv_color_hex(0xffffff), 0);
  lv_obj_center(cl);

  lv_obj_t *btn_canc = lv_button_create(card);
  lv_obj_set_size(btn_canc, 155, 44);
  lv_obj_set_pos(btn_canc, 10, 120);
  lv_obj_set_style_bg_color(btn_canc, lv_color_hex(0x8e8e93), 0);
  lv_obj_set_style_radius(btn_canc, 10, 0);
  lv_obj_set_style_border_width(btn_canc, 0, 0);
  lv_obj_add_event_cb(btn_canc, angle_90_cancel_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_t *canl = lv_label_create(btn_canc);
  lv_label_set_text(canl, en ? "Cancel" : "لغو");
  lv_obj_set_style_text_font(canl, en ? &lv_font_montserrat_14 : &ui_font_naz18b, 0);
  lv_obj_set_style_text_color(canl, lv_color_hex(0xffffff), 0);
  lv_obj_center(canl);
}

void prcs_angle_90_prompt() {
  if (cut_angle_90_prompt_req) {
    cut_angle_90_prompt_req = false;
    angle_90_modal_show();
  }
}

void prcs_web_portal_ap_result() {
  int8_t r = web_portal_ap_result;
  if (r == 0) return;
  web_portal_ap_result = 0;
  GUI.flag_web_portal_chip_refresh = true;
  if (r > 0) {
    ui_toast_wifi_ready(ap_ssid, ap_pass);
  } else if (r == -2) {
    GUI.buzzer_cmd_long = true;
    ui_toast("شبکه در اختیار بروزرسانی است",
             "WiFi is busy with the updater", 3500);
  } else {
    GUI.buzzer_cmd_long = true;
    ui_toast("روشن شدن وای فای شکست خورد", "SoftAP failed — check serial log", 5000);
  }
}
void prcs_web_portal_dialog() {
  if (!GUI.flag_web_portal_dialog) return;
  if (GUI.ui_screen_settle > 0) return;
  GUI.flag_web_portal_dialog = false;
  web_portal_apply_toggle(!web_portal_running);
}
//-----------------دیالوگ تأیید ایمپورت سایزها از سریال----------------------
static lv_obj_t *serial_import_mbox = NULL;
static void serial_import_yes_cb(lv_event_t *e) {
  GUI.buzzer_cmd_short = true;
  GUI.lock_screen = false;
  serial_import_apply();
  si_dialog_busy = false;
  if (serial_import_mbox && lv_obj_is_valid(serial_import_mbox)) lv_obj_delete_async(serial_import_mbox);
  serial_import_mbox = NULL;
}
static void serial_import_no_cb(lv_event_t *e) {
  GUI.buzzer_cmd_short = true;
  GUI.lock_screen = false;
  si_dialog_busy = false;
  Serial0.println("CANCELLED");
  if (serial_import_mbox && lv_obj_is_valid(serial_import_mbox)) lv_obj_delete_async(serial_import_mbox);
  serial_import_mbox = NULL;
}
void prcs_serial_import_dialog() {
  if (!GUI.flag_serial_import_dialog) return;
  GUI.flag_serial_import_dialog = false;
  if (serial_import_mbox != NULL) return;
  serial_import_mbox = lv_msgbox_create(NULL);
  lv_msgbox_add_title(serial_import_mbox, "Serial Size List");
  char txt[128];
  snprintf(txt, sizeof(txt), "File with %d size rows received on serial port.\nImport into cut list? (replaces rows 1..%d)",
           (int)si_pending_n, (int)si_pending_n);
  lv_msgbox_add_text(serial_import_mbox, txt);
  lv_obj_t *btn_yes = lv_msgbox_add_footer_button(serial_import_mbox, "IMPORT");
  lv_obj_t *btn_no = lv_msgbox_add_footer_button(serial_import_mbox, "NO");
  lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0x008306), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_no, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_yes, serial_import_yes_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btn_no, serial_import_no_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_center(serial_import_mbox);
  GUI.lock_screen = true;
}
bool prev_is_cutting = false;
void handle_cut_indicator_led() {
  if (STP.flag_is_cutting == prev_is_cutting) return;
  prev_is_cutting = STP.flag_is_cutting;
  //----------------//
  (STP.flag_is_cutting) ? lv_obj_clear_flag(objects.led1_1, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(objects.led1_1, LV_OBJ_FLAG_HIDDEN);
}

static bool safe_main1_built = false;
// یک‌بار در عمر بوت؛ تماس‌های بعدی no-op (موج پایداری — بدون recreate دیسک)
static void ensure_safe_main1() {
  if (safe_main1_built) return;
  safe_main1_built = true;
  main1_safe_mode = true;
  if (objects.obj1) lv_obj_add_flag(objects.obj1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(objects.main_1, lv_color_hex(0xbfc0c3), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(objects.main_1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

  // دیسک موقعیت: +۲٪ دیگر نسبت به ۴۱۷
  const int disk_sz = 425;  // 417 * 1.02
  const int disk_x = 28;    // (480 - 425) / 2
  const int disk_y = -53;   // هم‌مرکز عمودی با صفحه

  // سایهٔ نرم چندلایه (بدون LVGL shadow_width — جلوگیری از لبهٔ تیز/قفل رندر)
  {
      // فاصله نزدیک به دیسک؛ اپاسیته بالاتر برای فید سبز/قرمز خواناتر
  }

  const int ring_d = 6;
  const int ring_sz = disk_sz + 2 * ring_d;
  lv_obj_t *ring = lv_obj_create(objects.main_1);
  main_pos_shadow_ring = ring;
  lv_obj_set_pos(ring, disk_x - ring_d, disk_y - ring_d);
  lv_obj_set_size(ring, ring_sz, ring_sz);
  lv_obj_clear_flag(ring, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_radius(ring, ring_sz / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ring, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ring, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *disk = lv_obj_create(objects.main_1);
  main_pos_disk_obj = disk;
  lv_obj_set_pos(disk, disk_x, disk_y);
  lv_obj_set_size(disk, disk_sz, disk_sz);
  lv_obj_clear_flag(disk, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_style_radius(disk, (disk_sz + 1) / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(disk, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(disk, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(disk, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(disk, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  prev_main_in_pos = -1;  // force first color sync after build

  // EEZ قبلی را حذف کن تا ویجت یتیم + هندلر تکراری لمس نماند
  {
    main_fast_destination_reset();
    lv_obj_t *old_panel = objects.panel1_2;
    lv_obj_t *old_lab = objects.lable1_1;
    objects.panel1_2 = NULL;
    objects.lable1_1 = NULL;
    if (old_panel) lv_obj_del(old_panel);
    if (old_lab) lv_obj_del(old_lab);
  }

  objects.panel1_2 = lv_obj_create(objects.main_1);
  lv_obj_set_pos(objects.panel1_2, 111, 7);
  lv_obj_set_size(objects.panel1_2, 250, 83);
  lv_obj_clear_flag(objects.panel1_2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(objects.panel1_2, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(objects.panel1_2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(objects.panel1_2, lv_color_hex(0xe2e0e3), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(objects.panel1_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  objects.lable1_1 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.lable1_1, 72, 8);
  lv_obj_set_size(objects.lable1_1, 307, 77);
  lv_obj_set_style_text_font(objects.lable1_1, &ui_font_digit80, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(objects.lable1_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.lable1_1, lv_color_hex(0x0011d1), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(objects.lable1_1, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  {
    char dest_buf[12];
    float ppmm = axisPulsesPerMm();
    if (ppmm < 1.0f) ppmm = 1.0f;
    int dest_mm = GUI.inputBuffer_sp.length()
                      ? GUI.inputBuffer_sp.toInt()
                      : (int)lroundf((float)STP.currentPosition_p / ppmm);
    snprintf(dest_buf, sizeof(dest_buf), "%d", dest_mm);
    if (!main_fast_destination_set_text(dest_buf))
      lv_label_set_text(objects.lable1_1, dest_buf);
  }
  main_bind_destination_scrub_handlers();
  ui_rebind_destination_help();

  objects.lable1_2 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.lable1_2, 9, 84);
  lv_obj_set_size(objects.lable1_2, 463, 155);
  lv_obj_set_style_text_font(objects.lable1_2, &ui_font_digit160, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(objects.lable1_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.lable1_2, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(objects.lable1_2, 35, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(objects.lable1_2, "0");
  lv_obj_add_flag(objects.lable1_2, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(objects.lable1_2, lable1_2_click_event_handler, LV_EVENT_CLICKED, NULL);

  objects.lable1_3 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.lable1_3, 112, 236);
  lv_obj_set_size(objects.lable1_3, 240, 89);
  lv_obj_set_style_text_font(objects.lable1_3, &ui_font_digit80, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(objects.lable1_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.lable1_3, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(objects.lable1_3, "---");
  lv_obj_add_flag(objects.lable1_3, LV_OBJ_FLAG_CLICKABLE);

  objects.obj4 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.obj4, 364, 56);
  lv_obj_set_size(objects.obj4, 86, 34);
  lv_obj_set_style_text_font(objects.obj4, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.obj4, lv_color_hex(0x597393), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(objects.obj4, "\xD9\x85\xDB\x8C\xD9\x84\xDB\x8C \xD9\x85\xD8\xAA\xD8\xB1");

  objects.obj3 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.obj3, 311, 260);
  lv_obj_set_size(objects.obj3, 78, 34);
  lv_obj_set_style_text_font(objects.obj3, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.obj3, lv_color_hex(0x597393), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(objects.obj3, "\xD8\xAA\xD8\xB9\xD8\xAF\xD8\xA7\xD8\xAF");

  objects.obj2 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.obj2, 141, 253);
  lv_obj_set_size(objects.obj2, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_text_font(objects.obj2, &lv_font_montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.obj2, lv_color_hex(0x597393), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(objects.obj2, "X");

  objects.btn1_1 = lv_imagebutton_create(objects.main_1);
  lv_obj_set_pos(objects.btn1_1, 1, 0);  // +۱px راست؛ دیسک سبز ثابت می‌ماند
  lv_obj_set_size(objects.btn1_1, LV_SIZE_CONTENT, 50);
  lv_imagebutton_set_src(objects.btn1_1, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_plus_silver, NULL);
  lv_imagebutton_set_src(objects.btn1_1, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_plus_silver, NULL);
  lv_obj_set_style_image_recolor(objects.btn1_1, lv_color_hex(0x16a34a), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_image_recolor_opa(objects.btn1_1, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_image_recolor(objects.btn1_1, lv_color_hex(0x0f7f3a), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_image_recolor_opa(objects.btn1_1, LV_OPA_80, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_clear_flag(objects.btn1_1, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_WITH_ARROW | LV_OBJ_FLAG_SNAPPABLE));
  lv_obj_set_style_transform_pivot_x(objects.btn1_1, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_y(objects.btn1_1, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_rotation(objects.btn1_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_x(objects.btn1_1, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_y(objects.btn1_1, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_x(objects.btn1_1, 275, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_transform_scale_y(objects.btn1_1, 275, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_event_cb(objects.btn1_1, main_icon_press_spin, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(objects.btn1_1, main_icon_press_spin, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_1, main_icon_press_spin, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_add_event_cb(objects.btn1_1, btn1_1_click_event_handler, LV_EVENT_RELEASED, NULL);

  objects.btn1_2 = lv_imagebutton_create(objects.main_1);
  lv_obj_set_pos(objects.btn1_2, 429, 1);
  lv_obj_set_size(objects.btn1_2, LV_SIZE_CONTENT, 50);
  lv_imagebutton_set_src(objects.btn1_2, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_gear, NULL);
  lv_imagebutton_set_src(objects.btn1_2, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_gear, NULL);
  lv_obj_set_style_image_recolor(objects.btn1_2, lv_color_hex(0x26384d), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_image_recolor_opa(objects.btn1_2, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_image_recolor(objects.btn1_2, lv_color_hex(0x1f2f42), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_image_recolor_opa(objects.btn1_2, LV_OPA_60, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_clear_flag(objects.btn1_2, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_WITH_ARROW | LV_OBJ_FLAG_SNAPPABLE));
  lv_obj_set_style_transform_pivot_x(objects.btn1_2, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_pivot_y(objects.btn1_2, LV_PCT(50), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_rotation(objects.btn1_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_x(objects.btn1_2, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_y(objects.btn1_2, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_transform_scale_x(objects.btn1_2, 270, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_transform_scale_y(objects.btn1_2, 270, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_event_cb(objects.btn1_2, main_icon_press_spin, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(objects.btn1_2, main_icon_press_spin, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(objects.btn1_2, main_icon_press_spin, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_add_event_cb(objects.btn1_2, btn1_2_click_event_handler, LV_EVENT_RELEASED, NULL);

  // نشانگر جهت: فقط "<" / ">" ساده — بدون transform-scale (scale متن را روی برد نامرئی می‌کرد)
  objects.anim1_1 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.anim1_1, 371, 237);  // ۱۲px پایین‌تر از ۲۲۵
  lv_obj_set_size(objects.anim1_1, 48, 48);
  lv_obj_set_style_text_font(objects.anim1_1, &lv_font_montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.anim1_1, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(objects.anim1_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(objects.anim1_1, ">");
  lv_obj_add_flag(objects.anim1_1, LV_OBJ_FLAG_HIDDEN);

  objects.anim1_2 = lv_label_create(objects.main_1);
  lv_obj_set_pos(objects.anim1_2, 71, 237);
  lv_obj_set_size(objects.anim1_2, 48, 48);
  lv_obj_set_style_text_font(objects.anim1_2, &lv_font_montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(objects.anim1_2, lv_color_hex(0xb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(objects.anim1_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(objects.anim1_2, "<");
  lv_obj_add_flag(objects.anim1_2, LV_OBJ_FLAG_HIDDEN);

  // panel1_1 / panel1_3 را از نو نساز — EEZ قبلاً با دکمه/متن ساخته؛ کپی خالی باعث بهم‌ریختگی اتمام لیست می‌شود
  if (objects.panel1_1) {
    lv_obj_add_flag(objects.panel1_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(objects.panel1_1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.panel1_1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (objects.panel1_3) {
    lv_obj_add_flag(objects.panel1_3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(objects.panel1_3, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.panel1_3, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void prcs_keyboard_enter() {
  if (!keyboard_pending_enter) return;
  keyboard_pending_enter = false;
  if (GUI.scr_id == 1 && numpad_ctx == 5) {
    numpad_commit();
    return;
  }
  if (!STP.begin_calibration && GUI.scr_id == 1) {
    numpad_close();  // استارت/Enter با نامپد باز = بستن + حرکت
    commit_main_sp(true);
  } else if (f1_num_entry_active()) {
    f1_num_entry_commit();
  } else if (STP.begin_calibration && STP.calib_stage == 2 && GUI.scr_id == 2) {
    STP.Axis_Pitch = GUI.inputBuffer_initial_L1.toFloat() / 3;
    STP.um_per_pulse = (STP.Axis_Pitch * 1000.0f) / (float)STP.drv_ppr;
    save_axis_pitch();
    save_um_ppulse();
    GUI.edit_mode_lable2_2 = false;
    STP.calib_stage = 3;
    lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L1.c_str());
    GUI.L1_GUI = GUI.inputBuffer_initial_L1.toInt();
    save_L1_GUI();
    GUI.clear_buffer_aut_flag = true;
    GUI.flag_update_lables_2 = true;
  } else if (STP.begin_calibration && STP.calib_stage == 4 && GUI.scr_id == 2) {
    STP.initial_Position_p = (int)lroundf(axisPulsesPerMm() * GUI.inputBuffer_initial_L2.toFloat());
    STP.currentPosition_p = STP.initial_Position_p;
    STP.calib_stage = 0;
    STP.begin_calibration = false;
    GUI.edit_mode_lable2_2 = false;
    GUI.edit_mode_lable2_3 = false;
    GUI.clear_buffer_aut_flag = true;
    GUI.L2_GUI = (int)lroundf(GUI.inputBuffer_initial_L2.toFloat());
    save_init_pos();
    save_L2_GUI();
    lv_label_set_text(objects.lable2_2, GUI.inputBuffer_initial_L2.c_str());
    {
      char spb[16];
      float spv = (float)STP.initial_Position_p / axisPulsesPerMm();
      if (fabsf(spv - roundf(spv)) > 0.05f) snprintf(spb, sizeof(spb), "%.1f", (double)spv);
      else snprintf(spb, sizeof(spb), "%d", (int)lroundf(spv));
      GUI.inputBuffer_sp = spb;
    }
    GUI.flag_update_lables_2 = true;
    GUI.update_lable1_1 = true;
  } else if (!STP.begin_calibration && GUI.scr_id == 2 && GUI.enable_edit_lable2_3) {
    f2_commit_start_value();
  } else if (GUI.scr_id == 3) {
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_sp[j]) { commit_sp_entry(j); return; }
    }
    for (int j = 0; j < LIST_MAX; j++) {
      if (AUT.list_edit_mode_count[j]) { commit_count_entry(j); return; }
    }
  } else if (GUI.scr_id == 2 && GUI.enable_edit_lable2_5) {
    f2_commit_end_value();
  }
}

static uint32_t ui_last_full_signal_ms = 0;
static uint32_t ui_last_motion_light_ms = 0;
static uint32_t ui_last_keyboard_signal_ms = 0;
static uint32_t ui_last_fast_signal_ms = 0;
static uint32_t ui_display_refr_period_ms = 0;
static uint32_t ui_last_jog_destination_ms = 0;

static inline void jog_destination_refresh_tick(uint32_t now) {
  if (GUI.scr_id != 1 || !GUI.NumLock ||
      !(keyboardJogActive || IOS.joystick_state_L || IOS.joystick_state_R || jog_grow_active)) {
    ui_last_jog_destination_ms = 0;
    return;
  }
  if (ui_last_jog_destination_ms != 0 &&
      (uint32_t)(now - ui_last_jog_destination_ms) < 20UL) return;
  ui_last_jog_destination_ms = now;
  GUI.update_lable1_1 = true;
}

static inline bool ui_main_fast_refresh_active() {
  return GUI.scr_id == 1 &&
         (STP.motorIs_moving || STP.jogEnabled || keyboardJogActive ||
          IOS.joystick_state_L || IOS.joystick_state_R || jog_grow_active);
}

static inline void ui_sync_display_refresh_period() {
  const uint32_t period_ms = ui_main_fast_refresh_active() ? 20UL : 20UL;
  if (period_ms == ui_display_refr_period_ms) return;
  lv_timer_t *refr = lv_display_get_refr_timer(NULL);
  if (!refr) return;
  lv_timer_set_period(refr, period_ms);
  lv_timer_ready(refr);
  ui_display_refr_period_ms = period_ms;
}

static inline bool ui_main_motion_light_mode() {
  // موج A: مسیر کامل سبک‌شدهٔ جدا (که قبلاً سیگنال‌ها را گرسنه می‌کرد) خاموش می‌ماند.
  // به‌جای آن: در process_ui_signals کار لیست/تنظیمات غیرضروری وسط حرکت اصلی skip می‌شود.
  return false;
}

static inline void process_ui_fast_signals() {
  const uint32_t now = millis();
  // Task2 loops close to 500 Hz, while the display refreshes at 40–50 Hz.
  // Five milliseconds keeps UI/keyboard latency below one frame but avoids
  // repeatedly polling the same unchanged LVGL state between two refreshes.
  if (ui_last_fast_signal_ms != 0 &&
      (uint32_t)(now - ui_last_fast_signal_ms) < 5UL) return;
  ui_last_fast_signal_ms = now;
  jog_destination_refresh_tick(now);
  // Caret refresh currently performs LVGL style/foreground work. A 10 ms gate
  // keeps keyboard response imperceptible while preventing 500 calls/second.
  if ((uint32_t)(now - ui_last_keyboard_signal_ms) >= 10UL) {
    ui_last_keyboard_signal_ms = now;
    usb_keyboard_ui_drain();
    prcs_keyboard_enter();
    vkb_poll_usb();
    prcs_keyboard_ui_requests();
  }
  qa_process_ui_action();
  f1_help_deferred_tick();
  // Consume both changing values on the fast pass. Their own 20 ms gates
  // prevent duplicate work and keep destination/current in the same frame.
  if (GUI.scr_id == 1) {
    // رشد مقصد بعد از رها→دوباره: تیک مستقل از حرکت موتور (~50Hz)
    update_lable1_2();
    update_lable1_1();
    jog_manual_badge_sync();
    // هر تیک — نه فقط لبه؛ مسیر full-signal قبلاً مثلث را از دست می‌داد
    jog_fine_badge_sync(jog_fine_held);
    prcs_dir_anim();
  }
  prcs_update_page();
  if (opt_calc_active) prcs_opt_calc_progress();
  prcs_opt_remote_open();
  if (flag_start_update_flow) {
    flag_start_update_flow = false;
    update_flow_start_from_ui();
  }
}

static inline void process_main_motion_light_signals(uint32_t now) {
  if ((uint32_t)(now - ui_last_keyboard_signal_ms) >= 10UL) {
    ui_last_keyboard_signal_ms = now;
    usb_keyboard_ui_drain();
    prcs_keyboard_enter();
    vkb_poll_usb();
    prcs_keyboard_ui_requests();
  }
  prcs_usb_mouse_extras();
  if ((uint32_t)(now - ui_last_motion_light_ms) >= 25UL) {
    ui_last_motion_light_ms = now;
    update_lable1_2();
    update_lable1_1();
  }
  prcs_main_pos_count_flash();
  prcs_lable1_2_clr();
  prcs_panel1_2_clr();
  jog_manual_badge_sync();
  blade_err_badge_sync();
  jog_fine_badge_sync(jog_fine_held);
  prcs_dir_anim();
  cast_img_emg();
  delete_img_emg();
}

static inline bool ui_full_signals_due(uint32_t now) {
  if (ui_last_full_signal_ms == 0) return true;
  if (ui_resume_has_pending_apply()) return true;
  if (GUI.flag_save_visible_raws || GUI.flag_save_all || GUI.flag_save_stock_kerf ||
      GUI.flag_save_vkb || GUI.flag_save_buzzer || GUI.flag_apply_language ||
      GUI.flag_reinit_settings || GUI.flag_update_page || GUI.flag_serial_import_dialog ||
      OPT.flag_refresh || flag_start_update_flow) {
    return true;
  }
  // Touch and LVGL timers still run continuously. This pass is maintenance and
  // flag consumption; 20 Hz cuts idle CPU without reducing touch responsiveness.
  // During main motion, live position/destination/input/safety UI runs in the
  // light pass. Keep the full maintenance pass at 5 Hz for pending work
  // without scanning every settings/list path twenty times per second.
  const uint32_t period_ms = ui_main_motion_light_mode() ? 200UL : 50UL;
  return (uint32_t)(now - ui_last_full_signal_ms) >= period_ms;
}

static inline void process_ui_signals() {
  panel_five_nav_tick();
  const uint32_t now = millis();
  jog_destination_refresh_tick(now);
  // وسط جاگ/حرکت روی اصلی: ناوبری لمس و فلگ‌های حیاتی می‌مانند؛ نگهداری لیست/F2 سبک می‌شود
  const bool main_motion = (GUI.scr_id == 1) && (STP.motorIs_moving || STP.jogEnabled);
  if (ref_failed_flag || ref_wrong_dir_flag) prcs_ref_homing_result();
  if ((uint32_t)(now - ui_last_keyboard_signal_ms) >= 10UL) {
    ui_last_keyboard_signal_ms = now;
    usb_keyboard_ui_drain();
    prcs_keyboard_enter();
    vkb_poll_usb();
    prcs_keyboard_ui_requests();
  }
  prcs_usb_mouse_extras();
  qa_process_ui_action();
  f1_help_deferred_tick();
  f4_clock_live_tick();
  oobe_wizard_check_start();
  oobe_wizard_tick();
  if (g_warranty_expired_dialog_request) {
    g_warranty_expired_dialog_request = false;
    open_warranty_expired_dialog();
  }
  if (g_warranty_screen_request) {
    g_warranty_screen_request = false;
    open_warranty_screen();
  }
  if (!main_motion) reinit_settings_widgets();
  update_lable1_2();
  update_lable1_1();
  update_lable1_3();
  if (!main_motion) {
    update_lable_2_5();
    update_lables_2();
    process_screen3_updates();
  }
  led_indicator_update();
  prcs_lable_clr();
  prcs_panel1_2_clr();
  if (GUI.scr_id == 1) {
    jog_manual_badge_sync();
    jog_fine_badge_sync(jog_fine_held);
  }
  if (!main_motion) prcs_visible_raws();
  if (GUI.flag_save_visible_raws) {
    GUI.flag_save_visible_raws = false;
    save_visible_raws();
  }
  if (GUI.flag_save_all) {
    GUI.flag_save_all = false;
    settings_flush_pending_edits();
    esp_task_wdt_reset();
    if (!g_oobe_done) {
      if (g_oobe_stage == 1) {
        save_stock_kerf();
        g_oobe_stage = 2;
        ui_toast("تنظیمات عمومی ثبت شد. اکنون در F2 سایز اول و آخر خط را تنظیم و ذخیره فرمایید",
                 "F1 settings saved. Complete Start & End sizes in F2", 3500);
        if (objects.tabview2_1) {
          lv_tabview_set_active(objects.tabview2_1, 1, LV_ANIM_OFF);
          admin_tabs_reflow();
          f1_header_title_sync();
          oobe_show_f2_guidance();
        }
        return;
      } else if (g_oobe_stage == 2) {
        save_all();
        g_oobe_stage = 0;
        save_oobe_done(true);
        ui_toast("راه‌اندازی اولیه با موفقیت انجام شد!", "Initial setup completed successfully!", 3500);
        GUI.pending_goto_scr = 1;
        return;
      }
    }
    // بدون تغییر واقعی: NVS ننویس (عمر فلش)
    if (!settings_has_unsaved_changes()) {
      panel_save_feedback_pending = false;
    } else if (f3_pinmap_commit_on_save()) {
      const bool f4_timers_changed = f4_chain_commit_pending_on_header_save();
      apply_pending();
      flush_pending_list_saves_now();
      esp_task_wdt_reset();
      save_all();
      if (f4_timers_changed) save_admin_svc_state();
      esp_task_wdt_reset();
      settings_after_save_all();
      if (panel_save_feedback_pending) {
        panel_save_feedback_pending = false;
        ui_toast("ذخیره شد", "Saved", 1500);
      }
    } else {
      panel_save_feedback_pending = false;
      settings_mark_dirty();
    }
  }
  if (GUI.flag_save_stock_kerf) {
    GUI.flag_save_stock_kerf = false;
    save_stock_kerf();
  }
  if (GUI.flag_save_vkb) {
    GUI.flag_save_vkb = false;
    pref.begin("params", false);
    pref_put_bool_if_changed(pref, "vkb_en", GUI.virtual_kb_enable);
    pref_put_uchar_if_changed(pref, "vkb_mode", GUI.vkb_mode);
    pref.end();
  }
  if (GUI.flag_save_buzzer) {
    GUI.flag_save_buzzer = false;
    save_buzzer_state();
    // بوق تأیید اینجا حذف شد — web_settings_write همان جا buzzer_cmd_short می‌زند؛
    // بوق دوم یعنی «بوق تکراری» برای یک تغییر از وب. فقط ذخیره.
  }
  if (bl_pulse_until_ms && (int32_t)(millis() - bl_pulse_until_ms) >= 0) {
    bl_pulse_until_ms = 0;
  }
  if (GUI.flag_apply_language) {
    GUI.flag_apply_language = false;
    apply_system_language_ui(GUI.pending_lang_selected);
    f1_apply_language_texts();
    main_reflow_position_disk();
    // همیشه — متن Cut List / Length / Quantity روی lablef3_* فقط اینجاست
    list_reflow_compact();
  }
  // صفر کردن NVS برای ردیف‌های تازه‌ساخته — حداکثر ۱ ردیف در هر تیک (نرم‌تر برای رندر LCD)
  {
    int done = 0;
    for (int i = 0; i < LIST_MAX && done < 1; i++) {
      uint8_t bit = (uint8_t)(1u << (i & 7));
      if (!(GUI.pending_zero_bits[i >> 3] & bit)) continue;
      GUI.pending_zero_bits[i >> 3] = (uint8_t)(GUI.pending_zero_bits[i >> 3] & ~bit);
      save_permanently_preferences_sp_val(i, 0);
      save_permanently_preferences_count_val(i, 0);
      done++;
    }
  }
  // ذخیرهٔ متراژ/تعداد لیست — حداکثر ۱ در هر تیک
  {
    int done = 0;
    for (int i = 0; i < LIST_MAX && done < 1; i++) {
      uint8_t bit = (uint8_t)(1u << (i & 7));
      if (GUI.pending_sp_save_bits[i >> 3] & bit) {
        GUI.pending_sp_save_bits[i >> 3] = (uint8_t)(GUI.pending_sp_save_bits[i >> 3] & ~bit);
        save_permanently_preferences_sp_val(i, AUT.list_sp_Val_p[i]);
        done++;
      }
      if (done >= 1) break;
      if (GUI.pending_count_save_bits[i >> 3] & bit) {
        GUI.pending_count_save_bits[i >> 3] = (uint8_t)(GUI.pending_count_save_bits[i >> 3] & ~bit);
        save_permanently_preferences_count_val(i, AUT.list_count_Val[i]);
        done++;
      }
    }
  }
  prcs_main_pos_count_flash();
  prcs_lable1_2_clr();
  numpad_sync_screen();
  update_panel3_2_x_clr();
  prcs_dir_anim();
  prcs_tabview_f_keys();
  prcs_cast_error1_1();
  prcs_remove_error1_1();
  prcs_cast_error1_3();
  prcs_remove_error1_3();
  // قفل مودال سنسور بدون UI → آزادسازی ایمن (حس هنگ لمس)
  if (sensor_alert_motion_lock && !sensor_alert_is_open()) {
    sensor_alert_close_now();
  } else if (GUI.lock_screen && sensor_alert_kind != 0 && !sensor_alert_is_open()) {
    sensor_alert_close_now();
  }
  // وقتی سنسور آزاد شد، escape را بردار تا دوباره محافظت شود
  if (limit_escape_sensor == 1 && !IOS.ref_sensor_state) limit_escape_sensor = 0;
  if (limit_escape_sensor == 2 && !IOS.up_lim_sensor_state) limit_escape_sensor = 0;
  if (sensor_stuck_escape_flag) {
    sensor_stuck_escape_flag = false;
    const int which = (sensor_stuck_which == 2) ? 2 : 1;
    sensor_alert_open(4, which, 0);
  }
  prcs_limit_recal_msg();   // قبل از wrong_dir تا در صورت هم‌زمانی، پیام مهم‌تر (جهت اشتباه) غالب شود
  prcs_wrong_dir_error();
  prcs_blade_guard_timeout_error();
  prcs_wrong_dir_timer();
  prcs_jog_soft_limit_toast();
  prcs_cut_done_toast();
  cut_safety_ui_tick();
  cut_start_wrong_screen_ui_tick();
  prcs_exit_queue_mode_prompt();
  prcs_menu_key_save();
  prcs_menu_key_cancl();
  prcs_panel_start_key();
  prcs_panel_stop_key();
  prcs_panel_nav_external_exit();
  prcs_mode_key();
  prcs_main_f_mem_keys();
  prcs_save_main_f_mem();
  if (flag_panel_stop_motion_toast) {
    flag_panel_stop_motion_toast = false;
    ui_toast("حرکت توسط کلید توقف متوقف شد", "Motion stopped by stop key", 2800);
  }
  handle_stopstate_save_request();
  uncheck_saw_output_btns();
  check_saw_output_btns();
  cast_img_emg();
  delete_img_emg();
  handle_cut_indicator_led();
  prcs_factory_reset_dialog();
  prcs_web_portal_dialog();
  prcs_web_portal_toggle();
  prcs_web_portal_ap_result();
  prcs_web_portal_chip_refresh();
  prcs_web_confirm_gate();
  prcs_angle_90_prompt();
  prcs_ui_toast();
  if (g_flag_time_synced_toast) {
    g_flag_time_synced_toast = false;
    ui_toast("ساعت دستگاه نسبت به ساعت جهانی تنظیم شد", "System clock synced with world time", 4000);
  }
  if (g_flag_update_available_toast) {
    g_flag_update_available_toast = false;
    ui_toast("نسخه جدید در سرور موجود است", "New update available on server", 5000);
  }
  prcs_list_purge_confirm();
  prcs_opt_done_modals();
  prcs_update_page();
  prcs_opt_calc_progress();
  ui_resume_apply_pending_state();
  prcs_ui_resume_opt();
  prcs_opt_refresh();
  prcs_opt_remote_open();
  prcs_serial_import_dialog();
  prcs_splash();
  prcs_temp_monitor();
  prcs_clear_list_counts();
  device_lock_ui_tick();
  pitch_cal_ui_tick();
  if (flag_start_update_flow) {
    flag_start_update_flow = false;
    update_flow_start_from_ui();
  }
  if (GUI.scr_id == 2) {
    if (objects.tabview2_1 && lv_obj_is_valid(objects.tabview2_1))
      ui_active_settings_tab = (int)lv_tabview_get_tab_act(objects.tabview2_1);
    else
      ui_active_settings_tab = -1;
    if (ui_active_settings_tab == 1) {
      if (f2_seeking_limit1 && !STP.motorIs_moving && !STP.goref_cmd) {
        f2_seeking_limit1 = false;
        if (objects.btn2_3 && lv_obj_is_valid(objects.btn2_3)) {
          lv_obj_set_style_bg_color(objects.btn2_3, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
          if (objects.lablef2_8) lv_obj_set_style_text_color(objects.lablef2_8, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
          if (objects.lablee2_8) lv_obj_set_style_text_color(objects.lablee2_8, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
      }
      if (f2_seeking_limit2 && !STP.motorIs_moving && !STP.moveStepperCMD) {
        f2_seeking_limit2 = false;
        if (objects.btn2_4 && lv_obj_is_valid(objects.btn2_4)) {
          lv_obj_set_style_bg_color(objects.btn2_4, lv_color_hex(0x0a84ff), LV_PART_MAIN | LV_STATE_DEFAULT);
          if (objects.lablef2_9) lv_obj_set_style_text_color(objects.lablef2_9, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
          if (objects.lablee2_9) lv_obj_set_style_text_color(objects.lablee2_9, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
      }
    }
    if (ui_active_settings_tab == 2) {
      f3_q012_sync_active_green();
      f3_stepper_test_ui_tick();
    }
    // ساعت F5 برای دیدن هر ثانیه در مرز درست، مستقل از رفرش سبک ۸۰۰ms نمونه می‌شود.
    if (ui_active_settings_tab == 4)
      f5_dev_runtime_tick();
  } else {
    ui_active_settings_tab = -1;
  }
}

static void prcs_pending_goto_screen() {
  // بعد از settle کامل رنگ‌آمیزی، تازه reinit/نامپد — جلوی WDT وسط paint
  if (GUI.ui_screen_settle == 0 && GUI.pending_reinit_settings &&
      GUI.pending_goto_scr < 0 && GUI.scr_id == 2) {
    GUI.pending_reinit_settings = false;
    GUI.flag_reinit_settings = true;
  }
  int scr = GUI.pending_goto_scr;
  if (scr < 0) return;
  // [UI/Task2] Never leave Main while a cut process or its confirmation is live.
  // Navigation is rejected; Task8 remains the only owner of process outputs.
  const bool cut_process_busy = STP.flag_is_cutting || IOS.saw_is_on ||
                                IOS.clamp_engaged || IOS.cut_confirm_pending;
  if (GUI.scr_id == 1 && scr != 1 && cut_process_busy) {
    GUI.pending_goto_scr = -1;
    ui_resume_mark_current(1);
    ui_toast("ابتدا برش را در صفحه اصلی کامل یا متوقف کنید",
             "Complete or stop the cut on Main first", 3200);
    return;
  }
  if (GUI.scr_id == 2 && scr != 2 && !oobe_guard_leave_screen()) {
    GUI.pending_goto_scr = -1;
    return;
  }
  // ۲–۳ ثانیهٔ اول بعد از ظاهر شدن main: صفحه‌تنظیمات را نگه دار
  // استثنا: resume بعد هنگ — فوری همان تب/صفحه
  if (scr == 2 && !ui_resume_apply_pending && ui_resume_apply_scr < 0 &&
      GUI.ui_nav_ready_ms != 0 &&
      (int32_t)(millis() - GUI.ui_nav_ready_ms) < 0) {
    return;
  }
  GUI.pending_goto_scr = -1;
  panel_five_nav_exit();
  // آیکن قارچی روی lv_layer_top است؛ با عوض شدن صفحه خودش پاک نمی‌شود
  if (scr != 0 && scr != 1) GUI.flag_del_emg = true;
  switch (scr) {
    case 0:
      numpad_close();
      GUI.pending_f1_numpad_open = false;
      if (opt_cont) {
        lv_obj_delete_async(opt_cont);
        opt_cont = NULL;
        OPT.active = false;
        OPT.waiting_move = false;
      }
      STP.motorIs_moving = false;
      STP.jogEnabled = false;
      STP.moveStepperCMD = false;
      STP.goref_cmd = false;
      GUI.lock_screen = false;
      loadScreen(SCREEN_ID_MAIN_0);
      GUI.scr_id = 0;
      GUI.ui_screen_settle = 8;
      if (IOS.stop_button_state) GUI.flag_cast_emg = true;  // آیکن استاپ اگر از قبل فعال بود
      break;
    case 1: {
      bool arm_referenced_after_main = false;
      bool opt_preserve = false;
      if (opt_cont) opt_preserve = opt_consume_home_preserve();
      numpad_close();
      GUI.pending_f1_numpad_open = false;
      if (GUI.scr_id == 2) {
        // اگر Save هم‌زمان در صف باشد برنگردان (menu save → main)
        if (!GUI.flag_save_all) settings_on_leave_settings();
      }
      if (pending_reference_confirm) {
        // ورود با «بله» بدون موقعیت‌یابی کامل — بعد از لود صفحه referenced را قطعی کن
        // (قبلاً به پنجرهٔ 700ms جوگ وابسته بود؛ ensure_safe_main1/loadScreen گاهی >700ms
        // طول می‌کشید و referenced=false می‌ماند → Enter حرکت نمی‌کرد)
        pending_reference_confirm = false;
        arm_referenced_after_main = true;
        STP.motorIs_moving = false;
        STP.jogEnabled = false;
        STP.moveStepperCMD = false;
        STP.mvm_interupt = false;
        reference_jog_inhibit_until_ms = millis() + 700;
        GUI.flag_update_lable1_2 = true;
      }
      // از صفحه سبز/رفرنس → اصلی: مقصد قبل از paint = موتور (نه ۶۰ طول اولیه)
      if (GUI.scr_id == 0 && !main_queue_mode_active())
        main_restore_destination_to_current_pos();
      {
        ensure_safe_main1();
        main_reflow_position_disk();
        char pos_buf[12];
        main_format_position_for_boot(pos_buf, sizeof(pos_buf));
        if (!main_fast_position_digits_set_text(pos_buf))
          lv_label_set_text(objects.lable1_2, pos_buf);
      }
      ui_dismiss_all_soft_keyboards();
      loadScreen(SCREEN_ID_MAIN_1);
      GUI.scr_id = 1;
      GUI.ui_screen_settle = 8;
      // فوری بعد از loadScreen — قبل از settle تا EEZ «۰/۶۰» دیده نشود
      {
        char dest_buf[12];
        main_format_mm_for_boot(dest_buf, sizeof(dest_buf), STP.Man_Sp_mm);
        if (objects.lable1_1) {
          if (!main_fast_destination_set_text(dest_buf))
            lv_label_set_text(objects.lable1_1, dest_buf);
        }
      }
      GUI.update_lable1_1 = true;
      jog_manual_badge_invalidate();
      main_reflow_position_disk();
      jog_manual_badge_sync();
      // هر ورود به اصلی = مجاز حرکت (YES / رفرنس / برگشت تنظیمات)
      STP.referenced = true;
      if (IOS.ref_sensor_state) limit_escape_sensor = 1;
      else if (IOS.up_lim_sensor_state) limit_escape_sensor = 2;
      if (sensor_alert_motion_lock && !GUI.lock_screen) {
        sensor_alert_motion_lock = false;
        STP.mvm_interupt = false;
      }
      (void)arm_referenced_after_main;
      if (IOS.stop_button_state) GUI.flag_cast_emg = true;  // ورود به اصلی با استاپ از قبل فعال
      // بعد از لود main پنل بهینه را ببند تا لیست برش لحظه‌ای دیده نشود
      if (opt_cont) {
        lv_obj_delete_async(opt_cont);
        opt_cont = NULL;
        opt_clear_panel_ptrs();
        if (!opt_preserve) {
          OPT.active = false;
          OPT.waiting_move = false;
        } else {
          opt_load_current_to_main(false, true);
        }
      } else if (OPT.active && OPT.cur < OPT.piece_count) {
        opt_load_current_to_main(false, true);
      } else if (AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX) {
        opt_main_bar_lbl_sync();
      }
      apply_system_language_ui(GUI.system_language == 2 ? 1 : 0);
      f1_apply_language_texts();
      break;
    }
    case 2:
      jog_manual_badge_invalidate();
      ui_dismiss_all_soft_keyboards();  // تمام کیبوردهای صفحه قبل بسته شوند
      loadScreen(SCREEN_ID_MAIN_2);
      GUI.scr_id = 2;
      apply_system_language_ui(GUI.system_language == 2 ? 1 : 0);
      f1_apply_language_texts();
      f1_header_title_sync();
      if (!g_oobe_done) {
        if (g_oobe_stage == 1) oobe_show_f1_guidance();
        else if (g_oobe_stage == 2) oobe_show_f2_guidance();
      }
      // resume هنگ: تب هدف را فوری بزن — بدون فلش F1
      if (ui_resume_apply_pending && objects.tabview2_1) {
        int tab = (int)ui_resume_apply_settings_tab;
        if (tab < 0) tab = 0;
        if (tab > 4) tab = 4;
        lv_tabview_set_active(objects.tabview2_1, tab, LV_ANIM_OFF);
        settings_hide_eez_clutter();
        admin_tabs_reflow();
        f1_header_title_sync();
        GUI.ui_screen_settle = 2;  // کوتاه؛ پوشش سیاه تا apply می‌ماند
      } else {
        // اولین ورود بعد از بوت: settle بیشتر
        GUI.ui_screen_settle = (GUI.ui_nav_ready_ms &&
                                (int32_t)(millis() - GUI.ui_nav_ready_ms) < 4000) ? 16 : 10;
      }
      GUI.pending_reinit_settings = true;  // بعد از settle → کروم/نامپد
      break;
    case 3:
      jog_manual_badge_invalidate();
      ui_dismiss_all_soft_keyboards();
      if (opt_cont) {
        lv_obj_delete_async(opt_cont);
        opt_cont = NULL;
        OPT.active = false;
        OPT.waiting_move = false;
      }
      loadScreen(SCREEN_ID_MAIN_3);
      GUI.scr_id = 3;
      apply_system_language_ui(GUI.system_language == 2 ? 1 : 0);
      list_reflow_compact();
      prev_visible_raws = -1;
      prev_list_page_for_vis = -999;
      // اعمال آنی ردیف‌های فعال و مقادیر قبل از رندر اولین فریم جهت حذف کامل پرش موقت ۸ ردیف
      prcs_visible_raws();
      GUI.flag_refresh_visible = false;
      GUI.flag_update_labels_3_1 = false;
      GUI.flag_update_labels_3_2 = false;
      GUI.flag_update_page = true;
      list_entry_phase = 0;
      GUI.flag_update_panel3_2_x_clr = true;
      GUI.ui_screen_settle = 4;
      break;
    default: return;
  }
  // هر تعویض صفحه → RTC تا بعد از هنگ/WDT همان صفحه برگردد
  ui_resume_mark_current(GUI.scr_id);
  Serial.printf("[UI] goto scr=%d heap=%u\n", scr, (unsigned)ESP.getFreeHeap());
  splash_release_hold_cover();
}

static inline uint32_t ui_perf_compute_busy_us(uint32_t loop_start_us, uint32_t dma_wait_start_us) {
  uint32_t elapsed = (uint32_t)(micros() - loop_start_us);
  uint32_t dma_wait = (uint32_t)(display_dma_wait_counter() - dma_wait_start_us);
  return (dma_wait < elapsed) ? (elapsed - dma_wait) : 0;
}

void Task2(void *pvParameters) {
  ets_printf("T2 enter\n");
  const uint32_t boot_wait_start = millis();
  while (!ui_bootstrap_complete && (uint32_t)(millis() - boot_wait_start) < 6000U) vTaskDelay(pdMS_TO_TICKS(1));
  ets_printf("T2 after delay\n");
  Serial.println("task2 start");
  ets_printf("T2 after println\n");
  // اگر اسپلش سریع بود/نبود — حداقل گیت از شروع Task2
  if (GUI.ui_nav_ready_ms == 0) GUI.ui_nav_ready_ms = millis() + 3500;
  // واچ‌داگ رندر: اگر Task2 هنگ کرد → TASK_WDT → همان صفحه از RTC برگردد
  {
    esp_task_wdt_config_t wdt_cfg = {
      .timeout_ms = 3500,  // ۳.۵ ثانیه — فرصت کافی برای رندر صفحات سنگین و NVS بدون ریست کاذب
      .idle_core_mask = 0,
      .trigger_panic = true,
    };
    (void)esp_task_wdt_reconfigure(&wdt_cfg);
    (void)esp_task_wdt_add(NULL);
  }
  int dbg_iter = 0;
  while (1) {
    (void)esp_task_wdt_reset();
    uint32_t perf_loop_start_us = micros();
    uint32_t perf_dma_wait_start_us = display_dma_wait_counter();
    bool perf_busy_recorded = false;
    if (dbg_iter < 3) ets_printf("T2a render %d\n", dbg_iter);
    // Close the previous asynchronous transaction before any new LVGL state
    // change, screen switch, screenshot arm, or direct LCD benchmark.
    display_dma_finish_pending();
    display_dma_try_enable_scheduled();
    // تعویض صفحه قبل از timer — هرگز داخل callback لمس
    prcs_pending_goto_screen();
    // resume هنگ: صف goto را همین حلقه بزن (منتظر full-signals نمان)
    if (splash_cont || dma_ui_resume_scr >= 0)
      prcs_splash();
    // نبض RTC: اگر روی یک صفحه ماند و هنگ شد، همان صفحه برگردد
    {
      static uint32_t ui_resume_hb_ms = 0;
      uint32_t now_ms = millis();
      if ((uint32_t)(now_ms - ui_resume_hb_ms) >= 500UL) {
        ui_resume_hb_ms = now_ms;
        ui_resume_mark_current(GUI.scr_id);
      }
    }
    // خطای موقعیت‌یابی: کارت بزرگ + بدون ضربدر روی متن (GUI_Func::prcs_ref_homing_result)
    if (ref_failed_flag || ref_wrong_dir_flag) prcs_ref_homing_result();
    shot_poll_uart0();
    if (g_remote_ui_cmd == 1) {
      g_remote_ui_cmd = 0;
      open_warranty_screen();
    } else if (g_remote_ui_cmd == 2) {
      g_remote_ui_cmd = 0;
      ui_open_volume_modal();
    } else if (g_remote_ui_cmd == 3) {
      g_remote_ui_cmd = 0;
      uf_open_manual_source();
    } else if (g_remote_ui_cmd == 4) {
      g_remote_ui_cmd = 0;
      close_warranty_screen();
      ui_close_volume_modal();
      uf_manual_back_cb(NULL);
      oobe_close();
      splash_close_manual();
    } else if (g_remote_ui_cmd == 5) {
      g_remote_ui_cmd = 0;
      update_flow_start_from_ui();
    } else if (g_remote_ui_cmd == 6) {
      g_remote_ui_cmd = 0;
      request_goto_screen(2);
      if (objects.tabview2_1) lv_tabview_set_active(objects.tabview2_1, 0, LV_ANIM_OFF);
    } else if (g_remote_ui_cmd == 7) {
      g_remote_ui_cmd = 0;
      request_goto_screen(1);
    } else if (g_remote_ui_cmd == 8) {
      g_remote_ui_cmd = 0;
      oobe_render_step(1);
    } else if (g_remote_ui_cmd == 9) {
      g_remote_ui_cmd = 0;
      oobe_render_step(2);
    } else if (g_remote_ui_cmd == 10) {
      g_remote_ui_cmd = 0;
      oobe_render_step(3);
    } else if (g_remote_ui_cmd == 11) {
      g_remote_ui_cmd = 0;
      save_oobe_done(false);
      oobe_wizard_start();
    } else if (g_remote_ui_cmd == 12) {
      g_remote_ui_cmd = 0;
      splash_manual_view(0);
    } else if (g_remote_ui_cmd == 13) {
      g_remote_ui_cmd = 0;
      splash_manual_view(1);
    } else if (g_remote_ui_cmd == 14) {
      g_remote_ui_cmd = 0;
      request_goto_screen(0);
    } else if (g_remote_ui_cmd == 15) {
      g_remote_ui_cmd = 0;
      request_goto_screen(2);
    } else if (g_remote_ui_cmd == 16) {
      g_remote_ui_cmd = 0;
      request_goto_screen(3);
    } else if (g_remote_ui_cmd == 17) {
      g_remote_ui_cmd = 0;
      extern void oobe_verify_master_pin();
      oobe_verify_master_pin();
    }
    shot_arm();
    lcd_bench_run_if_pending(display_dma_ready);
    ui_sync_display_refresh_period();
    // نمونهٔ تازه پیش از refresh: بدون تأخیر یک‌فریم و بدون redraw مقدار تکراری.
    if (ui_main_fast_refresh_active()) {
      GUI.flag_update_lable1_2 = true;
      update_lable1_2();
    }
    lv_timer_handler();
    cutter_screensaver_tick();
    update_flow_tick();   // ماشینِ حالتِ بروزرسانیِ F5-بوت (فقط وقتی g_update_mode)
    vkb_auto_open_guard_tick();
    if (GUI.ui_screen_settle > 0) {
      GUI.ui_screen_settle--;
      // فقط رندر — بدون reinit/نامپد/NVS؛ WDT فرصت فید می‌گیرد
      if (GUI.scr_id == 3 && list_entry_phase == 2 && GUI.ui_screen_settle <= 7) {
        // loadScreen متن/چیدمان EEZ را برمی‌گرداند؛ یک فریم مستقل برای reflow.
        list_reflow_compact();
        list_entry_phase = 1;
      } else if (GUI.scr_id == 3 && list_entry_phase == 1 && GUI.ui_screen_settle <= 4) {
        // این تابع خودش index و مقدار/رنگ تمام ردیف‌های قابل‌دیدن را اعمال می‌کند.
        prcs_visible_raws();
        GUI.flag_update_labels_3_1 = false;
        GUI.flag_update_labels_3_2 = false;
        list_entry_phase = 0;
      }
      perf_overlay_add_busy(ui_perf_compute_busy_us(perf_loop_start_us, perf_dma_wait_start_us));
      perf_overlay_tick();
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }
    if (GUI.pending_boot_main_ready) {
      GUI.pending_boot_main_ready = false;
      if (GUI.buzzer_enable) (void)buzzer_request_pattern(BUZZER_PATTERN_BOOT);
      // قبل از اولین نمایش اصلی: مقصد = موتور (نه اصلاح بعد از ظاهر شدن)
      if (!main_queue_mode_active())
        main_restore_destination_to_current_pos();
      ensure_safe_main1();
      main_reflow_position_disk();
      {
        char pos_buf[12];
        main_format_position_for_boot(pos_buf, sizeof(pos_buf));
        if (!main_fast_position_digits_set_text(pos_buf))
          lv_label_set_text(objects.lable1_2, pos_buf);
      }
      if (objects.lable1_1) {
        // لیبل مقصد را همین‌جا از بافر هم‌تراز با موتور بنویس (scr ممکن است هنوز ۰ باشد)
        char dest_buf[12];
        main_format_mm_for_boot(dest_buf, sizeof(dest_buf), STP.Man_Sp_mm);
        if (!main_fast_destination_set_text(dest_buf))
          lv_label_set_text(objects.lable1_1, dest_buf);
        GUI.update_lable1_1 = true;
      }
      display_dma_schedule_enable();
    }
    if (dbg_iter < 3) {
      ets_printf("T2b signals %d\n", dbg_iter);
      dbg_iter++;
    }



    // نامپد F1 جدا از reinit — یک تیک سبک‌تر
    // نامپد مجازی بدون فوکوس پیش‌فرض روی طول شاخه
    if (GUI.pending_f1_numpad_open && GUI.scr_id == 2 && GUI.virtual_kb_enable &&
        !vkb_auto_open_guard_active() &&
        !f1_help_blocks_numpad()) {
      GUI.pending_f1_numpad_open = false;
      numpad_build();
      numpad_style_f1();
      if (numpad_cont) {
        lv_obj_clear_flag(numpad_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(numpad_cont);
      }
      if (f1_ok_overlay) {
        lv_obj_clear_flag(f1_ok_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(f1_ok_overlay);
      }
      // بدون ctx=3 → نشانگر روی طول شاخه نماند
      if (numpad_ctx != 3 && numpad_ctx != 4) {
        numpad_ctx = 0;
        f1_caret_hide();
      }
      perf_overlay_add_busy(ui_perf_compute_busy_us(perf_loop_start_us, perf_dma_wait_start_us));
      perf_busy_recorded = true;
      vTaskDelay(pdMS_TO_TICKS(3));
    } else {
      const bool in_update_or_portal = (g_update_mode || uf_portal_sta_session);
      if (!in_update_or_portal) {
        uint32_t now = millis();
        const bool main_motion_light = ui_main_motion_light_mode();
        if (main_motion_light) {
          process_main_motion_light_signals(now);
        }
        if (ui_full_signals_due(now)) {
          process_ui_signals();
          ui_last_full_signal_ms = now;
        } else if (!main_motion_light) {
          process_ui_fast_signals();
        }
      }
    }
    shot_dump();
    if (!perf_busy_recorded) {
      perf_overlay_add_busy(ui_perf_compute_busy_us(perf_loop_start_us, perf_dma_wait_start_us));
    }
    perf_overlay_tick();
    // 1 ms services serial/input without polling gaps during motion or active editing.
    // In idle (no motor moving, no active edit), 3ms delay keeps CPU load ultra-low (< 15%).
    // In update/portal mode, 5ms delay saves massive CPU while retaining immediate touch response.
    uint32_t t2_delay_ms = 1;
    if (g_update_mode || uf_portal_sta_session) {
      t2_delay_ms = 5;
    } else {
      const bool busy_active = STP.motorIs_moving || STP.jogEnabled || STP.moveStepperCMD ||
                               STP.flag_is_cutting || (numpad_ctx > 0);
      if (!busy_active) {
        t2_delay_ms = 3;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(t2_delay_ms));
  }
}
