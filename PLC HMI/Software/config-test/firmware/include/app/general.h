#ifndef GENERAL_H
#define GENERAL_H

#include <Arduino.h>
#include <esp_system.h>
#include "defines.h"

// ===== ADC Key Ladder — GPIO1 (F1..F5) & GPIO2 (F6..F10) =====
// 5-Sample Median Filter — Siemens Industrial Standard for Noise Rejection
static inline uint16_t readMedian5(int pin)
{
    uint16_t s[5];
    for (int i = 0; i < 5; i++) {
        s[i] = (uint16_t)analogRead(pin);
    }
    for (int i = 1; i < 5; i++) {
        uint16_t key = s[i];
        int j = i - 1;
        while (j >= 0 && s[j] > key) {
            s[j + 1] = s[j];
            j--;
        }
        s[j + 1] = key;
    }
    return s[2]; // میانه ۵ نمونه جهت حذف کامل نویز ضربه‌ای کنتاکتورها
}

// نگاشت ولتاژ ADC → شماره کلید؛ بدون نمونه‌برداری در تابع (خوانده‌شده از بیرون)
static inline uint8_t getKeyFromAnalog(uint16_t value)
{
    // B-10: باندهای گسترش‌یافته — گپ بین باندها کاهش یافت (miss zone کوچک‌تر)
    // مقادیر هدف: F1/F6≈4095, F2/F7≈3320, F3/F8≈2710, F4/F9≈1955, F5/F10≈1211
    // حاشیه نویز بین باندها: ~۱۵۰ واحد ADC (کافی برای ایمنی در محیط صنعتی)
    if (value >= 3800U && value <= 4095U) return 1U; // F1 / F6 (SAVE)
    if (value >= 3050U && value <= 3650U) return 2U; // F2 / F7 (CANCEL)
    if (value >= 2450U && value <= 2950U) return 3U; // F3 / F8 (MODE)
    if (value >= 1700U && value <= 2150U) return 4U; // F4 / F9 (START)
    if (value >= 1000U && value <= 1450U) return 5U; // F5 / F10 (STOP)
    return 0U; // بدون کلید
}

// ===== Buzzer Helper =====
extern "C" int32_t get_var_buzzer(void);
extern "C" void set_var_buzzer(int32_t value);
static inline void buzzer(uint32_t count, uint32_t delay_ms_on, uint32_t delay_ms_off)
{
    int32_t vol = get_var_buzzer();
    if (vol <= 0) return;
    for (uint32_t i = 0; i < count; i++)
    {
        ledcWrite(BUZZER, (uint32_t)vol);
        vTaskDelay(pdMS_TO_TICKS(delay_ms_on));
        ledcWrite(BUZZER, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms_off));
    }
}

#endif // GENERAL_H
