#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <lvgl.h>
#include <RTClib.h>
#include "storage.h"
#include "general.h"
#include "sound.h"
#include "shift_manager.h"
#include "day_lock.h"
#include "lang.h"
#include "ui_HardwareDiagnostics.h"
#include "web_portal.h"
#include "agent_hmi.h"
#include "screenshot.h"
#include "bakery_cycle_fsm.h"
#include "ui_notification.h"
#include "date_utils.h"
#include "ui_WiFiScreen.h"
#include "ui_ProjectInfoScreen.h"
#include "panel_nav.h"

// وابسته ها باید بیرون تعریف شوند (مثلا تو main)
extern ScreenMap currentScreen;
extern TaskHandle_t buzzerTaskHandle;
extern RTC_DS3231 rtc;
extern uint8_t lastS;
extern String days[];
extern ButtonState fPanel, mPanel;
extern unsigned long lastMiliSec;
extern bool processActive;
extern bool isAutoScreenActive;
extern bool cycleStarted;
extern Storage storage;
extern bool shouldBeep;
extern ShiftManager shiftManager;

extern bool isFunneleLeft;
extern bool isFunneleRight;

extern bool isPistonUp;
extern bool isPistonDown;

extern unsigned long lastMiliSecCycle;
extern bool rtc_available;

// LVGL Task Handle
void lv_tick_task(void *arg)
{
    while (true)
    {
        lv_tick_inc(1); // 1ms tick
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// LVGL Render Task (Protected by global FreeRTOS mutex)
void lvgl_task(void *param)
{
    while (true)
    {
        if (lv_lock())
        {
            agent_hmi_poll();
            shot_poll();
            ui_WiFiScreen_update_tick();
            ui_ProjectInfoScreen_update_tick();
            panel_nav_poll_tick();
            lv_timer_handler();
            lv_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void buzzer_task(void *param)
{
    while (true)
    {
        SoundType s = SOUND_NONE;
        if (soundQueue != NULL && xQueueReceive(soundQueue, &s, pdMS_TO_TICKS(50)) == pdPASS)
        {
            sound_execute_melody(s);
        }
        else if (ulTaskNotifyTake(pdTRUE, 0) > 0)
        {
            sound_execute_melody(SOUND_CLICK);
        }
    }
}

void keypad_task(void *param)
{
    while (true)
    {
        int keyF = readDebouncedButton(KEY_F, fPanel);
        int keyM = readDebouncedButton(KEY_M, mPanel);

        if (keyF != 0)
        {
            sound_play(SOUND_KEY_PRESS);
            panel_nav_queue_key(1, (uint8_t)keyF);
        }
        if (keyM != 0)
        {
            sound_play(SOUND_KEY_PRESS);

            // Dual personality: When navigation is IDLE, M4/M5 control physical machine
            if (!panel_nav_is_active())
            {
                // Siemens Industrial Safety: Key M5 (Stop) is an unconditional Emergency Stop across all screens
                if (keyM == 5)
                {
                    processActive = false;
                    cycleStarted = false;
                    funneleStop();
                    pistonStop();
                    sound_play(SOUND_STOP_CYCLE);

                    if (lv_lock())
                    {
                        if (ui_AutomaticMode_StartStopBTT != NULL && lv_obj_is_valid(ui_AutomaticMode_StartStopBTT))
                        {
                            lv_label_set_text(ui_AutomaticMode_StartStopBTT, "شروع");
                            lv_obj_clear_state(ui_AutomaticMode_StartStopButton, LV_STATE_USER_1);
                            lv_obj_clear_state(ui_AutomaticMode_BackButton, LV_STATE_DISABLED);
                            lv_obj_clear_state(ui_AutomaticMode_SetupHolder, LV_STATE_USER_1);
                        }
                        ui_show_notification(NOTIF_WARNING, "توقف اضطراری شاسی M5", "تمام عملگرها سریعا قطع و فرآیند متوقف شد");
                        lv_unlock();
                    }
                }
                else if (keyM == 4 && currentScreen == AUTOMATIC)
                {
                    if (day_lock_is_locked())
                    {
                        sound_play(SOUND_ALARM_FAULT);
                        if (lv_lock())
                        {
                            ui_show_notification(NOTIF_ERROR, "خطای قفل دستگاه", "مهلت کارکرد به پایان رسیده است");
                            dev_lock_ui_show();
                            lv_unlock();
                        }
                    }
                    else
                    {
                        processActive = true;
                        cycleStarted = true;
                        sound_play(SOUND_START_CYCLE);

                        if (lv_lock())
                        {
                            lv_label_set_text(ui_AutomaticMode_StartStopBTT, "توقف");
                            lv_obj_add_state(ui_AutomaticMode_StartStopButton, LV_STATE_USER_1);
                            lv_obj_add_state(ui_AutomaticMode_BackButton, LV_STATE_DISABLED);
                            lv_obj_add_state(ui_AutomaticMode_SetupHolder, LV_STATE_USER_1);
                            ui_show_notification(NOTIF_SUCCESS, "شروع فرآیند شاسی M4", "تولید اتوماتیک چانه آغاز گردید");
                            lv_unlock();
                        }
                    }
                }
            }

            // Forward to panel_nav (handles navigation mode, focus cycle, and active adjustments)
            panel_nav_queue_key(2, (uint8_t)keyM);
        }

        // Auto-repeat for START (M4) and STOP (M5) when held down > 1000ms in navigation mode
        static uint32_t hold_start_ms = 0;
        static uint32_t last_repeat_ms = 0;
        static int held_key = 0;

        if (panel_nav_is_active() && (mPanel.stableKey == 4 || mPanel.stableKey == 5))
        {
            if (held_key != mPanel.stableKey)
            {
                held_key = mPanel.stableKey;
                hold_start_ms = millis();
                last_repeat_ms = millis();
            }
            else
            {
                uint32_t now = millis();
                if ((now - hold_start_ms) >= 1000U && (now - last_repeat_ms) >= 150U)
                {
                    last_repeat_ms = now;
                    panel_nav_queue_key(2, (uint8_t)held_key);
                }
            }
        }
        else
        {
            held_key = 0;
            hold_start_ms = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
// Main App
void app_task(void *param)
{
    while (true)
    {

        if (rtc_available)
        {
            DateTime timeNow = rtc.now();
            shiftManager.updateTime(timeNow);
            if (lastS != timeNow.second())
            {
                lastS = timeNow.second();
                day_lock_tick_1s();
                if (currentScreen == MAIN && lv_lock())
                {
                    char timeBuf[16];
                    char dateBuf[16];
                    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", timeNow.hour(), timeNow.minute());
                    if (g_lang == LANG_FA)
                    {
                        int jy = 1404, jm = 1, jd = 1;
                        gregorian_to_jalali(timeNow.year(), timeNow.month(), timeNow.day(), &jy, &jm, &jd);
                        snprintf(dateBuf, sizeof(dateBuf), "%04d/%02d/%02d", jy, jm, jd);
                    }
                    else
                    {
                        snprintf(dateBuf, sizeof(dateBuf), "%04d/%02d/%02d", timeNow.year(), timeNow.month(), timeNow.day());
                    }
                    lv_label_set_text(ui_Menu_TimeLable, timeBuf);
                    lv_label_set_text(ui_Menu_DateLable, dateBuf);
                    lv_unlock();
                }
            }
        }
        else
        {
            unsigned long sec = millis() / 1000UL;
            if (lastS != (uint8_t)(sec % 60))
            {
                lastS = (uint8_t)(sec % 60);
                day_lock_tick_1s();
                if (currentScreen == MAIN && lv_lock())
                {
                    char timeBuf[16];
                    snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", (sec / 3600UL) % 24, (sec / 60UL) % 60);
                    lv_label_set_text(ui_Menu_TimeLable, timeBuf);
                    lv_label_set_text(ui_Menu_DateLable, (g_lang == LANG_FA) ? "1404/01/01" : "2025/01/01");
                    lv_unlock();
                }
            }
        }

        if (currentScreen == DIAGNOSTICS)
        {
            int rawF = analogRead(KEY_F);
            int rawM = analogRead(KEY_M);
            bool fHome = (digitalRead(FUNNELE_SENSOR) == LOW);
            bool pHome = (digitalRead(PISTON_SENSOR) == LOW);
            if (lv_lock())
            {
                ui_HardwareDiagnostics_update(rawF, rawM, fHome, pHome);
                lv_unlock();
            }
        }

        //? ====================================================================
        //? MANUAL MOTIONS (Non-blocking with sensor watchdog timeout)
        //? ====================================================================
        static uint32_t manualMotionStartMs = 0;

        if (isFunneleLeft)
        {
            if (manualMotionStartMs == 0) manualMotionStartMs = millis();
            if (digitalRead(FUNNELE_SENSOR) != LOW)
            {
                funneleStop();
                isFunneleLeft = false;
                manualMotionStartMs = 0;
            }
            else if (millis() - manualMotionStartMs > SENSOR_MOTION_TIMEOUT_MS)
            {
                Serial.println("ERR: Funnel Left manual watchdog timeout!");
                sound_play(SOUND_ALARM_FAULT);
                funneleStop();
                isFunneleLeft = false;
                manualMotionStartMs = 0;
                if (lv_lock())
                {
                    ui_show_notification(NOTIF_ERROR, "خطای تایم اوت سنسور قیف", "قیف در ۵ ثانیه به سنسور هوم نرسید - موتور متوقف شد");
                    lv_unlock();
                }
            }
            else
            {
                funneleLeft();
            }
        }
        else if (isFunneleRight)
        {
            if (manualMotionStartMs == 0) manualMotionStartMs = millis();
            if (millis() - manualMotionStartMs >= (uint32_t)storage.get_setting_max_funnle())
            {
                funneleStop();
                isFunneleRight = false;
                manualMotionStartMs = 0;
            }
            else
            {
                funneleRight();
            }
        }
        else if (isPistonUp)
        {
            if (manualMotionStartMs == 0) manualMotionStartMs = millis();
            if (digitalRead(PISTON_SENSOR) != LOW)
            {
                pistonStop();
                isPistonUp = false;
                manualMotionStartMs = 0;
            }
            else if (millis() - manualMotionStartMs > SENSOR_MOTION_TIMEOUT_MS)
            {
                Serial.println("ERR: Piston Up manual watchdog timeout!");
                sound_play(SOUND_ALARM_FAULT);
                pistonStop();
                isPistonUp = false;
                manualMotionStartMs = 0;
                if (lv_lock())
                {
                    ui_show_notification(NOTIF_ERROR, "خطای تایم اوت سنسور پیستون", "پیستون در ۵ ثانیه به سنسور هوم نرسید - موتور متوقف شد");
                    lv_unlock();
                }
            }
            else
            {
                pistonUP();
            }
        }
        else if (isPistonDown)
        {
            if (manualMotionStartMs == 0) manualMotionStartMs = millis();
            if (millis() - manualMotionStartMs >= (uint32_t)storage.getAutoWeight())
            {
                pistonStop();
                isPistonDown = false;
                manualMotionStartMs = 0;
            }
            else
            {
                pistonDown();
            }
        }
        else
        {
            manualMotionStartMs = 0;
            //? ================================================================
            //? DETERMINISTIC AUTOMATIC CYCLE FSM (Siemens IEC 61131-3 Standard)
            //? ================================================================
            static BakeryCycleFSM cycleFSM;
            cycleFSM.update();
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Dedicated background web portal task (Core 0, non-blocking)
void web_task(void *param)
{
    while (true)
    {
        webPortal.handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#endif
