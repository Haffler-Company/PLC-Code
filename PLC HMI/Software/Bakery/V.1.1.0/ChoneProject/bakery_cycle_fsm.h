#ifndef BAKERY_CYCLE_FSM_H
#define BAKERY_CYCLE_FSM_H

#include <Arduino.h>
#include "defines.h"
#include "storage.h"
#include "general.h"
#include "sound.h"
#include "shift_manager.h"
#include "ui.h"
#include "ui_notification.h"
#include "day_lock.h"

//? ============================================================================
//? SIEMENS DETERMINISTIC FINITE STATE MACHINE (PackML / IEC 61131-3)
//? ============================================================================

enum class CycleState : uint8_t {
    IDLE = 0,
    FUNNEL_HOMING,
    FUNNEL_HOMING_PAUSE,
    PISTON_HOMING,
    FUNNEL_FEEDING,
    PISTON_DISCHARGING,
    CYCLE_DELAY,
    FAULT_SENSOR_TIMEOUT,
    EMERGENCY_STOP
};

class BakeryCycleFSM {
private:
    CycleState currentState = CycleState::IDLE;
    uint32_t phaseStartTime = 0;
    uint32_t lastBarValue = 0;
    const char* lastStatusText = "";

    void updateUIProgress(uint32_t progressPercent) {
        if (progressPercent != lastBarValue) {
            lastBarValue = progressPercent;
            if (isAutoScreenActive && lv_lock()) {
                lv_bar_set_value(ui_AutomaticMode_Bar2, progressPercent, LV_ANIM_ON);
                lv_unlock();
            }
        }
    }

    void updateUIStatus(const char* text, lv_color_t color) {
        if (text != lastStatusText) {
            lastStatusText = text;
            if (isAutoScreenActive && lv_lock()) {
                if (ui_AutomaticMode_StatusLabel != NULL && lv_obj_is_valid(ui_AutomaticMode_StatusLabel)) {
                    lv_label_set_text(ui_AutomaticMode_StatusLabel, text);
                    lv_obj_set_style_text_color(ui_AutomaticMode_StatusLabel, color, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                lv_unlock();
            }
        }
    }

    void syncUIStopState() {
        if (lv_lock()) {
            lv_label_set_text(ui_AutomaticMode_StartStopBTT, "شروع");
            lv_obj_clear_state(ui_AutomaticMode_StartStopButton, LV_STATE_USER_1);
            lv_obj_clear_state(ui_AutomaticMode_BackButton, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_AutomaticMode_SetupHolder, LV_STATE_USER_1);
            lv_unlock();
        }
    }

public:
    void reset() {
        currentState = CycleState::IDLE;
        phaseStartTime = 0;
        updateUIProgress(0);
        updateUIStatus("وضعیت: آماده شروع چرخه", lv_color_hex(0x546E7A));
        funneleStop();
        pistonStop();
    }

    void abortFault(const char* reason) {
        Serial.printf("[FSM FAULT] %s\n", reason);
        sound_play(SOUND_ALARM_FAULT);
        currentState = CycleState::FAULT_SENSOR_TIMEOUT;
        cycleStarted = false;
        processActive = false;
        funneleStop();
        pistonStop();
        storage.commitCounter();
        updateUIProgress(0);
        updateUIStatus("خطا: توقف اضطراری سنسور", lv_color_hex(0xD32F2F));
        syncUIStopState();

        if (lv_lock()) {
            ui_show_notification(NOTIF_ERROR, "خطای توقف اضطراری سنسور", reason);
            lv_unlock();
        }
    }

    void stopCycle() {
        if (currentState != CycleState::IDLE) {
            Serial.println("[FSM] Cycle stopped by operator");
            sound_play(SOUND_STOP_CYCLE);
            reset();
            cycleStarted = false;
            processActive = false;
            storage.commitCounter();
            updateUIStatus("وضعیت: متوقف شده توسط اپراتور", lv_color_hex(0x546E7A));
            syncUIStopState();

            if (lv_lock()) {
                ui_show_notification(NOTIF_INFO, "توقف فرآیند", "چرخه کارکرد توسط اپراتور متوقف گردید");
                lv_unlock();
            }
        }
    }

    void startCycle() {
        if (device_locked) {
            Serial.println("[FSM] Machine is locked by Day-Lock license");
            cycleStarted = false;
            processActive = false;
            syncUIStopState();
            if (lv_lock()) {
                ui_show_notification(NOTIF_ERROR, "خطای قفل دستگاه", "مهلت کارکرد به پایان رسیده است");
                dev_lock_ui_show();
                lv_unlock();
            }
            return;
        }

        Serial.println("[FSM] Cycle started");
        currentState = CycleState::FUNNEL_HOMING;
        phaseStartTime = millis();
        cycleStarted = true;
        processActive = true;
        updateUIProgress(25);
        updateUIStatus("در حال بازگشت قیف به موقعیت اولیه...", lv_color_hex(0x0288D1));

        if (lv_lock()) {
            ui_show_notification(NOTIF_SUCCESS, "شروع فرآیند اتوماتیک", "چرخه اتوماتیک چانه گیری آغاز شد");
            lv_unlock();
        }
    }

    CycleState getState() const { return currentState; }

    void update() {
        uint32_t now = millis();

        // If cycle was commanded to start but FSM is currently idle
        if (cycleStarted && currentState == CycleState::IDLE) {
            startCycle();
            return;
        }

        // If cycle was cancelled or stop was pressed externally
        if (!cycleStarted && currentState != CycleState::IDLE && currentState != CycleState::FAULT_SENSOR_TIMEOUT) {
            stopCycle();
            return;
        }

        switch (currentState) {
            case CycleState::IDLE:
                // Steady idle state: outputs de-energized
                break;

            case CycleState::FUNNEL_HOMING:
                updateUIStatus("در حال بازگشت قیف به موقعیت هوم...", lv_color_hex(0x0288D1));
                // Sensor returns HIGH when home position is reached
                if (digitalRead(FUNNELE_SENSOR) != LOW) {
                    funneleStop();
                    phaseStartTime = now;
                    currentState = CycleState::FUNNEL_HOMING_PAUSE;
                    updateUIStatus("مکث استقرار مکانیکی قیف...", lv_color_hex(0x0288D1));
                } else if (now - phaseStartTime > SENSOR_MOTION_TIMEOUT_MS) {
                    abortFault("سنسور هوم قیف بعد از ۵ ثانیه تحریک نشد - موتور متوقف شد");
                } else {
                    funneleLeft();
                }
                break;

            case CycleState::FUNNEL_HOMING_PAUSE:
                funneleStop();
                // Mechanical settling pause between funnel and piston motions
                if (now - phaseStartTime >= 300) {
                    phaseStartTime = now;
                    updateUIProgress(59);
                    currentState = CycleState::PISTON_HOMING;
                    updateUIStatus("در حال بالا رفتن پیستون به هوم...", lv_color_hex(0x0288D1));
                }
                break;

            case CycleState::PISTON_HOMING:
                // Sensor returns HIGH when home position is reached
                if (digitalRead(PISTON_SENSOR) != LOW) {
                    pistonStop();
                    phaseStartTime = now;
                    updateUIProgress(75);
                    currentState = CycleState::FUNNEL_FEEDING;
                    updateUIStatus("در حال تغذیه خمیر به محفظه...", lv_color_hex(0x2E7D32));
                } else if (now - phaseStartTime > SENSOR_MOTION_TIMEOUT_MS) {
                    abortFault("سنسور هوم پیستون بعد از ۵ ثانیه تحریک نشد - موتور متوقف شد");
                } else {
                    pistonUP();
                }
                break;

            case CycleState::FUNNEL_FEEDING:
                if (now - phaseStartTime >= (uint32_t)storage.get_setting_max_funnle()) {
                    funneleStop();
                    phaseStartTime = now;
                    updateUIProgress(85);
                    currentState = CycleState::PISTON_DISCHARGING;
                    updateUIStatus("در حال برش و تخلیه چانه خمیر...", lv_color_hex(0xE65100));
                } else {
                    funneleRight();
                }
                break;

            case CycleState::PISTON_DISCHARGING:
                if (now - phaseStartTime >= (uint32_t)storage.getAutoWeight()) {
                    pistonStop();
                    updateUIProgress(100);

                    // Portion completed successfully!
                    if (storage.get_activeCounterSound()) {
                        sound_play(SOUND_PORTION_DONE);
                    }
                    shiftManager.onPortionCompleted();

                    int newCount = storage.getChoneCounter() + 1;
                    storage.setChoneCounter(newCount);

                    if (lv_lock()) {
                        char valBuf[16];
                        snprintf(valBuf, sizeof(valBuf), "%d", newCount);
                        lv_label_set_text(ui_Menu_Counter_Lable, valBuf);
                        if (isAutoScreenActive) {
                            lv_label_set_text(ui_AutomaticMode_SpeedValue1, valBuf);
                        }
                        lv_unlock();
                    }

                    phaseStartTime = now;
                    currentState = CycleState::CYCLE_DELAY;
                    updateUIStatus("وقفه تنظیمی بین دو چانه...", lv_color_hex(0x546E7A));
                } else {
                    pistonDown();
                }
                break;

            case CycleState::CYCLE_DELAY:
                pistonStop();
                funneleStop();
                if (now - phaseStartTime >= (uint32_t)storage.getAutoSpeed()) {
                    // Two-Phase Soft Lock: Safe cycle completion guard
                    if (day_lock_is_locked() || day_lock_is_soft_locked()) {
                        stopCycle();
                        if (lv_lock()) {
                            ui_show_notification(NOTIF_WARNING, "قفل نرم دستگاه", "اعتبار روزشمار به پایان رسید. چرخه جاری تکمیل و دستگاه متوقف شد.");
                            dev_lock_ui_show();
                            lv_unlock();
                        }
                        break;
                    }

                    updateUIProgress(0);
                    // Cycle next portion
                    currentState = CycleState::FUNNEL_HOMING;
                    phaseStartTime = now;
                    updateUIProgress(25);
                    updateUIStatus("در حال بازگشت قیف به موقعیت هوم...", lv_color_hex(0x0288D1));
                }
                break;

            case CycleState::FAULT_SENSOR_TIMEOUT:
            case CycleState::EMERGENCY_STOP:
                pistonStop();
                funneleStop();
                break;
        }
    }
};

#endif // BAKERY_CYCLE_FSM_H
