#include "panel_keys.h"
#include "panel_nav.h"
#include "warranty_mgr.h"
#include <Arduino.h>
#include "variables.h"
#include "actions.h"
#include "screens.h"
#include "ui.h"
#include "ui_toast.h"
#include "ui_resume.h"
#include "day_lock.h"
#include "a2_eeprom.h"

#define PIN_KEY_F      1
#define PIN_KEY_M      2
#define PIN_OPTO_START 7
#define PIN_OPTO_STOP  3

static uint32_t s_last_poll_ms = 0;
static panel_key_id_t s_last_key = KEY_NONE;
static uint8_t s_stable_count = 0;
static station_mode_t s_station_mode = STATION_MODE_AUTO;

station_mode_t panel_keys_get_mode(void) {
    return s_station_mode;
}

void panel_keys_set_mode(station_mode_t mode) {
    s_station_mode = mode;
}

static panel_key_id_t decode_adc_key_f(int raw) {
    if (raw > 3700) return KEY_NONE;
    if (raw >= 3100 && raw <= 3600) return KEY_F1;
    if (raw >= 2450 && raw <= 3000) return KEY_F2;
    if (raw >= 1700 && raw <= 2300) return KEY_F3;
    if (raw >= 950  && raw <= 1550) return KEY_F4;
    if (raw >= 200  && raw <= 850)  return KEY_F5;
    return KEY_NONE;
}

static panel_key_id_t decode_adc_key_m(int raw) {
    if (raw > 3700) return KEY_NONE;
    if (raw >= 3100 && raw <= 3600) return KEY_MODE;
    if (raw >= 2450 && raw <= 3000) return KEY_START;
    if (raw >= 1700 && raw <= 2300) return KEY_STOP;
    if (raw >= 950  && raw <= 1550) return KEY_SAVE;
    if (raw >= 200  && raw <= 850)  return KEY_CANCEL;
    return KEY_NONE;
}

void panel_keys_emergency_stop_all(void) {
    /* Siemens Determinism: Immediate hardware shutdown */
    HwSetValve1(false);
    HwSetValve2(false);
    HwSetBuzzer(false);
    TimerValve1 = 0;
    TimerValve2 = 0;
    ChangeBar(0);
    ChangeBar1(0);
    ui_main_show_station_status(0, UI_MAIN_STATUS_STOPPED);
    if (buzzerEnable && EEprom3_Buzzer) {
        buzzer_key;
    }
    ui_toast_show("توقف اضطراری", "فرآیند تزریق و کلیه خروجی ها متوقف شدند", 3500, TOAST_TYPE_ERROR);
}

static void handle_key_press(panel_key_id_t key) {
    if (buzzerEnable && EEprom3_Buzzer) {
        buzzer_key;
    }

    if (dev_lock_ui_is_active()) {
        if (key == KEY_OPTO_STOP) {
            panel_keys_emergency_stop_all();
            return;
        }
        if (key == KEY_STOP) {
            panel_keys_emergency_stop_all();
            day_lock_handle_panel_key((uint8_t)key);
            return;
        }
        if (key == KEY_MODE || key == KEY_START || key == KEY_CANCEL) {
            day_lock_handle_panel_key((uint8_t)key);
            return;
        }
        /* When day lock modal is active, suppress screen navigation keys */
        return;
    }

    switch (key) {
        case KEY_F1:
            if (panel_nav_is_active()) panel_nav_exit_mode();
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 0, LV_ANIM_OFF);
            ui_resume_checkpoint();
            ui_toast_show("تنظیمات عمومی", "زبانه تنظیمات عمومی باز شد [F1]", 2000, TOAST_TYPE_INFO);
            break;

        case KEY_F2:
            if (panel_nav_is_active()) panel_nav_exit_mode();
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 1, LV_ANIM_OFF);
            ui_resume_checkpoint();
            ui_toast_show("تست تجهیزات", "زبانه تست شیرها - سنسورها و سیم‌بندی [F2]", 2000, TOAST_TYPE_INFO);
            break;

        case KEY_F3:
            if (panel_nav_is_active()) panel_nav_exit_mode();
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 2, LV_ANIM_OFF);
            ui_resume_checkpoint();
            ui_toast_show("بخش مدیریت", "کنترل قفل، تاریخ، آمار و تنظیمات [F3]", 2000, TOAST_TYPE_INFO);
            break;

        case KEY_F4:
            /* Toggle context help for active screen/tab */
            ui_setting_help_open(NULL);
            break;

        case KEY_F5:
            if (panel_nav_is_active()) panel_nav_exit_mode();
            ui_close_all_modals();
            loadScreen(SCREEN_ID_MAIN);
            ui_toast_show("داشبورد اصلی", "انتقال به صفحه اصلی تزریق گاز [F5]", 2000, TOAST_TYPE_INFO);
            break;

        case KEY_MODE:
            panel_nav_queue_key(2, 3);
            break;

        case KEY_START:
            if (panel_nav_is_active()) {
                panel_nav_queue_key(2, 4);
                break;
            }
            /* Normal physical running mode start */
            if (ui_current_screen_id() == SCREEN_ID_MAIN) {
                if (s_station_mode == STATION_MODE_GLASS1) {
                    if (TimerValve2 == 0 && TimerValve2_Saved > 0) {
                        action_start_button_pressed1(NULL);
                        ui_toast_show("شروع تزریق", "تزریق گاز شیشه 1 آغاز شد", 3000, TOAST_TYPE_SUCCESS);
                    } else {
                        ui_toast_show("خطا در شروع", "ابعاد شیشه 1 معتبر نیست یا در حال تزریق است", 3500, TOAST_TYPE_WARNING);
                    }
                } else if (s_station_mode == STATION_MODE_GLASS2) {
                    if (TimerValve1 == 0 && TimerValve1_Saved > 0) {
                        action_start_button_pressed(NULL);
                        ui_toast_show("شروع تزریق", "تزریق گاز شیشه 2 آغاز شد", 3000, TOAST_TYPE_SUCCESS);
                    } else {
                        ui_toast_show("خطا در شروع", "ابعاد شیشه 2 معتبر نیست یا در حال تزریق است", 3500, TOAST_TYPE_WARNING);
                    }
                } else {
                    bool started_any = false;
                    if (TimerValve2 == 0 && TimerValve2_Saved > 0) {
                        action_start_button_pressed1(NULL);
                        started_any = true;
                    }
                    if (TimerValve1 == 0 && TimerValve1_Saved > 0) {
                        action_start_button_pressed(NULL);
                        started_any = true;
                    }
                    if (started_any) {
                        ui_toast_show("شروع تزریق", "فرآیند تزریق گاز آغاز شد", 3000, TOAST_TYPE_SUCCESS);
                    } else {
                        ui_toast_show("خطا در شروع", "ابعاد معتبر نیست یا دستگاه در حال کار است", 3500, TOAST_TYPE_WARNING);
                    }
                }
            }
            break;

        case KEY_OPTO_START:
            /* Foot pedal always starts physical machine cycle directly */
            if (ui_current_screen_id() == SCREEN_ID_MAIN) {
                if (s_station_mode == STATION_MODE_GLASS1) {
                    if (TimerValve2 == 0 && TimerValve2_Saved > 0) {
                        action_start_button_pressed1(NULL);
                        ui_toast_show("شروع تزریق", "تزریق گاز شیشه 1 آغاز شد", 3000, TOAST_TYPE_SUCCESS);
                    } else {
                        ui_toast_show("خطا در شروع", "ابعاد شیشه 1 معتبر نیست یا در حال تزریق است", 3500, TOAST_TYPE_WARNING);
                    }
                } else if (s_station_mode == STATION_MODE_GLASS2) {
                    if (TimerValve1 == 0 && TimerValve1_Saved > 0) {
                        action_start_button_pressed(NULL);
                        ui_toast_show("شروع تزریق", "تزریق گاز شیشه 2 آغاز شد", 3000, TOAST_TYPE_SUCCESS);
                    } else {
                        ui_toast_show("خطا در شروع", "ابعاد شیشه 2 معتبر نیست یا در حال تزریق است", 3500, TOAST_TYPE_WARNING);
                    }
                } else {
                    bool started_any = false;
                    if (TimerValve2 == 0 && TimerValve2_Saved > 0) {
                        action_start_button_pressed1(NULL);
                        started_any = true;
                    }
                    if (TimerValve1 == 0 && TimerValve1_Saved > 0) {
                        action_start_button_pressed(NULL);
                        started_any = true;
                    }
                    if (started_any) {
                        ui_toast_show("شروع تزریق", "فرآیند تزریق گاز آغاز شد", 3000, TOAST_TYPE_SUCCESS);
                    } else {
                        ui_toast_show("خطا در شروع", "ابعاد معتبر نیست یا دستگاه در حال کار است", 3500, TOAST_TYPE_WARNING);
                    }
                }
            }
            break;

        case KEY_STOP:
            /* Safety Red Line: If actively injecting gas, immediate physical emergency halt */
            if (TimerValve1 > 0 || TimerValve2 > 0) {
                if (panel_nav_is_active()) panel_nav_exit_mode();
                panel_keys_emergency_stop_all();
                break;
            }
            if (panel_nav_is_active()) {
                panel_nav_queue_key(2, 5);
                break;
            }
            panel_keys_emergency_stop_all();
            break;

        case KEY_OPTO_STOP:
            if (panel_nav_is_active()) panel_nav_exit_mode();
            panel_keys_emergency_stop_all();
            break;

        case KEY_SAVE:
            if (panel_nav_is_active()) {
                panel_nav_queue_key(2, 1);
                break;
            }
            action_save_button_pressed(NULL);
            ui_toast_show("ذخیره تنظیمات", "پارامترها در حافظه پایدار ثبت گردید", 2500, TOAST_TYPE_SUCCESS);
            break;

        case KEY_CANCEL:
            if (panel_nav_is_active()) {
                panel_nav_queue_key(2, 2);
            } else {
                ui_close_all_modals();
                loadScreen(SCREEN_ID_MAIN);
            }
            break;

        default:
            break;
    }
}

static bool s_f5_boot_active = false;

bool is_f5_boot_active(void) {
    return s_f5_boot_active;
}

void panel_keys_init(void) {
    pinMode(PIN_OPTO_START, INPUT_PULLUP);
    pinMode(PIN_OPTO_STOP,  INPUT_PULLUP);
    analogSetPinAttenuation(PIN_KEY_F, ADC_11db);
    analogSetPinAttenuation(PIN_KEY_M, ADC_11db);
    int raw_boot = analogRead(PIN_KEY_F);
    if (raw_boot >= 200 && raw_boot <= 850) {
        s_f5_boot_active = true;
    }
    s_last_poll_ms = millis();
    s_last_key = KEY_NONE;
    s_stable_count = 0;
}

void panel_keys_poll(void) {
    uint32_t now = millis();
    if (now - s_last_poll_ms < 25) return; // 40 Hz scanning
    s_last_poll_ms = now;

    /* Check digital opto-isolated inputs first (highest priority) */
    /* Siemens Rigor: 2-cycle (50ms) debounce on stop input to eliminate contactor EMI spikes */
    static uint8_t s_opto_stop_count = 0;
    if (stop_input_is_active()) {
        if (s_opto_stop_count < 255) s_opto_stop_count++;
        if (s_opto_stop_count >= 2) {
            if (s_last_key != KEY_OPTO_STOP) {
                s_last_key = KEY_OPTO_STOP;
                handle_key_press(KEY_OPTO_STOP);
            }
        }
        return;
    } else {
        s_opto_stop_count = 0;
        if (s_last_key == KEY_OPTO_STOP) {
            s_last_key = KEY_NONE;
        }
    }

    /* Siemens Rigor: 2-cycle (50ms) debounce on foot pedal to eliminate EMI noise spikes */
    static uint8_t s_opto_start_count = 0;
    if (digitalRead(PIN_OPTO_START) == LOW) {
        if (s_opto_start_count < 255) s_opto_start_count++;
        if (s_opto_start_count >= 2) {
            if (s_last_key != KEY_OPTO_START) {
                s_last_key = KEY_OPTO_START;
                handle_key_press(KEY_OPTO_START);
            }
        }
        return;
    } else {
        s_opto_start_count = 0;
        if (s_last_key == KEY_OPTO_START) {
            s_last_key = KEY_NONE;
        }
    }

    /* Read ADC ladder keypads */
    int raw_f = analogRead(PIN_KEY_F);
    panel_key_id_t key_f = decode_adc_key_f(raw_f);

    int raw_m = analogRead(PIN_KEY_M);
    panel_key_id_t key_m = decode_adc_key_m(raw_m);

    panel_key_id_t detected = KEY_NONE;
    if (key_m != KEY_NONE) detected = key_m;
    else if (key_f != KEY_NONE) detected = key_f;

    /* Siemens Rigor: 2-cycle (50ms) contact bounce filter for ADC resistor ladders */
    static panel_key_id_t s_cand_key = KEY_NONE;
    static uint8_t s_cand_count = 0;
    static uint32_t s_press_start_ms = 0;
    static uint32_t s_last_repeat_ms = 0;

    if (detected != KEY_NONE) {
        if (detected == s_cand_key) {
            if (s_cand_count < 255) s_cand_count++;
            if (s_cand_count >= 2 && s_last_key != detected && s_last_key == KEY_NONE) {
                s_last_key = detected;
                s_press_start_ms = now;
                s_last_repeat_ms = now;
                handle_key_press(detected);
            } else if (s_last_key == detected && panel_nav_is_active()) {
                /* Continuous hold auto-repeat (>1000ms, ticks every 150ms) */
                if (detected == KEY_START || detected == KEY_STOP) {
                    if ((now - s_press_start_ms) >= 1000U) {
                        if ((now - s_last_repeat_ms) >= 150U) {
                            s_last_repeat_ms = now;
                            panel_nav_auto_repeat(detected == KEY_START ? +1 : -1);
                        }
                    }
                }
            }
        } else {
            s_cand_key = detected;
            s_cand_count = 1;
        }
    } else {
        s_cand_key = KEY_NONE;
        s_cand_count = 0;
        s_last_key = KEY_NONE;
        s_press_start_ms = 0;
        s_last_repeat_ms = 0;
    }
}

bool panel_keys_is_start_pressed(void) {
    return (digitalRead(PIN_OPTO_START) == LOW);
}

void panel_keys_trigger_key(panel_key_id_t key) {
    handle_key_press(key);
}

