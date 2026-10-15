#ifndef GENERAL_H
#define GENERAL_H

#include <Arduino.h>
#include <esp_system.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "driver/periph_ctrl.h"   // برای ری‌ستِ پریفرالِ SARADC (رفعِ قفلِ ADC بینِ ری‌ست‌ها)

// قفلِ مشترکِ ADC را Codex با نامِ adcMutex مدیریت می‌کند (در panel_key_filter.h / main.cpp)
extern "C" void set_f5_boot_active(bool active);

// ===== Button Helpers =====
struct ButtonState
{
    int lastReading = 0;
    int stableKey = 0;
    unsigned long lastChangeTime = 0;
};
enum ScreenMap
{
    WARNING,
    MAIN,
    AUTOMATIC,
    MANUAL,
    SETTING
};

static unsigned long lastUpdateTime = 0;
static int band_low_check = 0;
static int band_high_check = 0;
static int value_to_display = 0;
static int last_target_temp = -1;

// extern Storage storage; // این خط کامنت شد چون تعریف Storage موجود نیست
// اگر از آن استفاده می‌کنید، باید تعریف آن را اضافه کنید

static inline int getKeyFromAnalog(int value)
{
    if (value >= 4000 && value <= 5000)
        return 1; // F1
    else if (value >= 3100 && value <= 3600)
        return 2; // F2
    else if (value >= 2500 && value <= 2900)
        return 3; // F3
    else if (value >= 1800 && value <= 2100)
        return 4; // F4
    else if (value >= 1100 && value <= 1400)
        return 5; // F5
    else
        return 0; // بدون کلید
    return 0;
}

static inline int readDebouncedButton(int analogPin, ButtonState &state)
{
    int rawValue = analogRead(analogPin);
    int currentReading = getKeyFromAnalog(rawValue);

    // Serial.println(rawValue);

    if (currentReading != state.lastReading)
    {
        state.lastChangeTime = millis();
    }

    if ((millis() - state.lastChangeTime) > 50) // زمان دیبانس 50 میلی‌ثانیه
    {
        // if (currentReading != state.stableKey)
        // {
        state.stableKey = currentReading;
        state.lastReading = currentReading;
        // Serial.println(state.stableKey);
        return state.stableKey;
            
        // }
    }

    //state.lastReading = currentReading;
    // Serial.println("222222");
    return 0;
    
    // return 0; // هیچ کلید پایداری فشرده نشده است

}

// ===== Buzzer Helper =====
extern volatile uint8_t g_buzzer_volume;

static inline void buzzer(int count, unsigned int delay_ms_on, unsigned int delay_ms_off)
{
    if (BUZZER == PIN_NC) return;
    uint8_t vol = g_buzzer_volume;
    if (vol == 0) return;

    for (int i = 0; i < count; i++)
    {
        digitalWrite(BUZZER, HIGH);
        if (delay_ms_on <= 35) {
            delayMicroseconds(delay_ms_on * 1000);
        } else {
            vTaskDelay(pdMS_TO_TICKS(delay_ms_on));
        }
        digitalWrite(BUZZER, LOW);
        if (delay_ms_off > 0)
        {
            vTaskDelay(pdMS_TO_TICKS(delay_ms_off));
        }
    }
}

// ===== Memory Report Helper =====
static inline void printMemoryReport()
{
    size_t flashSize = ESP.getFlashChipSize();
    size_t sketchSize = ESP.getSketchSize();
    size_t freeSketch = ESP.getFreeSketchSpace();
    float flashUsedPercent = (float)sketchSize / flashSize * 100;
    Serial.println("📊 Memory Usage Report:\n");

    Serial.printf("🔹 Flash Size: %u KB\n", flashSize / 1024);
    Serial.printf("🔹 Sketch Size: %u KB (%.2f%% used)\n", sketchSize / 1024, flashUsedPercent);
    Serial.printf("🔹 Free Sketch Space: %u KB\n\n", freeSketch / 1024);

    size_t heapTotal = ESP.getHeapSize();
    size_t heapFree = ESP.getFreeHeap();
    size_t heapUsed = heapTotal - heapFree;
    float heapUsedPercent = (float)heapUsed / heapTotal * 100;

    Serial.printf("🧠 Internal RAM (DRAM):\n");
    Serial.printf("🔹 Total: %u KB\n", heapTotal / 1024);
    Serial.printf("🔹 Used : %u KB (%.2f%% used)\n", heapUsed / 1024, heapUsedPercent);
    Serial.printf("🔹 Free : %u KB\n\n", heapFree / 1024);

    if (psramFound())
    {
        size_t psramTotal = ESP.getPsramSize();
        size_t psramFree = ESP.getFreePsram();
        size_t psramUsed = psramTotal - psramFree;
        float psramUsedPercent = (float)psramUsed / psramTotal * 100;

        Serial.printf("🧠 PSRAM:\n");
        Serial.printf("🔹 Total: %u KB\n", psramTotal / 1024);
        Serial.printf("🔹 Used : %u KB (%.2f%% used)\n", psramUsed / 1024, psramUsedPercent);
        Serial.printf("🔹 Free : %u KB\n\n", psramFree / 1024);
    }
    else
    {
        Serial.println("❌ PSRAM not detected.");
    }
}

// ===== Dummy Setup Placeholder =====
static inline void setupPin()
{
    // رفعِ قفلِ احتمالیِ ADC که از کرشِ قبلی در دامنهٔ RTC باقی مانده (وگرنه analogRead هنگ می‌کند → حلقهٔ ری‌ست)
    periph_module_reset(PERIPH_SARADC_MODULE);
    analogReadResolution(12);
    analogSetPinAttenuation(KEY_F, ADC_11db);
    analogSetPinAttenuation(KEY_M, ADC_11db);
    analogSetPinAttenuation(BACK, ADC_11db);

    digitalWrite(BUZZER, LOW);
    pinMode(BUZZER, OUTPUT);

    // ایمنی: پیکربندیِ HW روی مجموعهٔ ثابتِ ۸ پایهٔ *فیزیکیِ* کاندیدِ خروجی و ۵ پایهٔ کاندیدِ ورودی
    // انجام می‌شود — نه فقط پایه‌ای که همین الان صاحب دارد. چون کاربر می‌تواند از منوی اطلاعات هر
    // پایه را زمانِ اجرا N.C./جابجا کند، اگر اینجا فقط رویِ متغیرهای *فعلی* (TABLE/SSR_1/...) کار
    // می‌کردیم، پایه‌ای که همین الان بی‌صاحب است هرگز OUTPUT+OFF نمی‌گرفت (می‌توانست شناور/نامعین
    // بماند یا بعداً بدونِ ری‌استارت به نامِ دیگری اختصاص یابد بدونِ اینکه کسی pinMode رویش زده باشد).
    // آیتم: DEFAULT_SPARE_Q8/Q9 هم اضافه شد — این دو ترمینالِ اضافیِ روی برد اگر از دیالوگِ جابجایی
    // به نامی اختصاص یابند، باید از همین سرِ بوت OUTPUT+OFF باشند (نه فقط لحظهٔ تخصیص در رم).
    static const int outCandidates[10] = {
        DEFAULT_TABLE, DEFAULT_PRESS_LEFT, DEFAULT_PRESS_RIGHT, DEFAULT_ELEMENT,
        DEFAULT_SHABLON, DEFAULT_SSR_1, DEFAULT_ZARBEH1, DEFAULT_ZARBEH2,
        DEFAULT_SPARE_Q8, DEFAULT_SPARE_Q9
    };
    for (int i = 0; i < 10; i++) {
        // استاندارد زیمنس: ابتدا نوشتن سطح OFF (HIGH) روی ثبات پین پیش از فعال‌سازی خروجی جهت ممانعت از شوک میکروثانیه‌ای بوبین
        digitalWrite(outCandidates[i], OFF);
        pinMode(outCandidates[i], OUTPUT);
    }

    pinMode(DEFAULT_EMERGENCY, INPUT_PULLUP);
    pinMode(DEFAULT_PEDAL, INPUT_PULLUP);
    // پیکربندی صریح و پایدار پین‌های باس SPI سنسور ترموکوپل MAX6675 (همراه با پول‌آپ روی MISO برای جلوگیری از خوانش شناور 0x0000)
    pinMode(THERMO_1_CS, OUTPUT);
    digitalWrite(THERMO_1_CS, HIGH);
    pinMode(THERMO_2_CS, OUTPUT);
    digitalWrite(THERMO_2_CS, HIGH);
    pinMode(THERMO_SCK, OUTPUT);
    digitalWrite(THERMO_SCK, LOW);
    pinMode(THERMO_MISO, INPUT_PULLUP);

    if (SSR_1 != PIN_NC && SSR_1 >= 0) {
        digitalWrite(SSR_1, OFF);
        pinMode(SSR_1, OUTPUT);
    }

    // سنسورهای جک — هر سه پایه آنالوگ (المنت=GPIO6، شابلون=GPIO5، میز/برگشت=GPIO4)، رنج کامل 0..4095
    analogSetPinAttenuation(DEFAULT_SENSOR_ELEMENT, ADC_11db);
    analogSetPinAttenuation(DEFAULT_SENSOR_SHABLON, ADC_11db);
    analogSetPinAttenuation(DEFAULT_SENSOR_TABLE, ADC_11db);
}

static inline void checkBoot()
{
    extern bool g_is_crash_recovery_boot;
    if (!g_is_crash_recovery_boot) delay(100);
    int read = analogRead(KEY_F);
    Serial.print(read);
    if (read > 1000 && read < 1400)
    {
        set_var_ota_enable(true);
        set_f5_boot_active(true);
        // const esp_partition_t *first_app_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);

        // if (first_app_partition != NULL)
        // {
        //     esp_err_t err = esp_ota_set_boot_partition(first_app_partition);

        //     if (err == ESP_OK)
        //     {
        //         esp_restart();
        //     }
        //     else
        //     {
        //         Serial.print("Error BIOS  : ");
        //         Serial.println(esp_err_to_name(err));
        //     }
        // }
        // else
        // {
        //     Serial.println("BIOS OTA 404!");
        // }
    }
}

// bool manageHeater(int targetTemp, int currentTemp, int pidValue, bool isTypeJ, int heaterPin, int telorance)
// {
//     bool needsReset = false;
//     int targetTemp_adj;
//     int currentTemp_adj = currentTemp * 4;

//     if (isTypeJ)
//     {
//         targetTemp_adj = (targetTemp * 4) * 1.35;
//     }
//     else
//     {
//         targetTemp_adj = (targetTemp * 4);
//     }

//     if (((21 + currentTemp) > targetTemp) && (currentTemp < targetTemp + 7))
//     {
//         int ti = (currentTemp_adj + telorance - (currentTemp_adj / 2)) * pidValue;
//         ti = ti / 10;
//         int fast_ti = (targetTemp_adj + 15 - (targetTemp_adj / 2)) * pidValue;
//         fast_ti = fast_ti / 10;
//         int set_time = 0;
//         if (ti > 0)
//         {
//             digitalWrite(heaterPin, ON);
//             if (currentTemp < targetTemp + 1)
//             {
//                 if (currentTemp + 5 > targetTemp)
//                 {
//                     set_time = ti;
//                     if (currentTemp_adj > targetTemp_adj + 4)
//                         set_time = ti / 1.05;
//                     if (currentTemp_adj > targetTemp_adj + 5)
//                         set_time = ti / 1.10;
//                     if (currentTemp_adj > targetTemp_adj + 6)
//                         set_time = ti / 1.17;
//                 }
//                 else
//                 {
//                     set_time = fast_ti;
//                 }
//             }
//             else if (currentTemp < targetTemp + 6)
//             {
//                 if (currentTemp < targetTemp + 5)
//                 {
//                     if (currentTemp < targetTemp + 3)
//                     {
//                         if (currentTemp < targetTemp + 2)
//                         {
//                             set_time = ti / 1.35;
//                             if (currentTemp_adj > targetTemp_adj + 9)
//                                 set_time = ti / 1.5;
//                             if (currentTemp_adj > targetTemp_adj + 10)
//                                 set_time = ti / 1.65;
//                         }
//                         else
//                         {
//                             set_time = ti / 1.8;
//                             if (currentTemp_adj > targetTemp_adj + 13)
//                                 set_time = ti / 2.5;
//                         }
//                     }
//                     else
//                     {
//                         set_time = ti / 3.0;
//                     }
//                 }
//                 else
//                 {
//                     set_time = ti / 6.0;
//                 }
//             }
//             else
//             {
//                 set_time = ti / 12.0;
//             }
//             // delay(set_time);
//             vTaskDelay(pdMS_TO_TICKS(set_time));

//             digitalWrite(heaterPin, OFF);
//         }
//         if (targetTemp + 3 < currentTemp)
//         {
//             needsReset = true;
//         }
//     }
//     else if (currentTemp < targetTemp)
//     {
//         digitalWrite(heaterPin, ON);
//     }
//     else
//     {
//         digitalWrite(heaterPin, OFF);
//     }
//     return needsReset;
// }

bool manageHeater(int targetTemp, int currentTemp, int pidValue, bool isTypeJ, int heaterPin, int telorance)
{
    bool needsReset = false;

    // Static variables to maintain state like in heater_contorl
    static bool stat_down = 0;
    static unsigned long copy_time = 0;

    // Adjusted temperature values
    int targetTemp_adj;
    int currentTemp_adj = currentTemp * 4;

    // --- START OF UPDATED LOGIC ---

    // Temperature adjustment logic from heater_contorl
    // Note: The factor is 1.38 and is applied when isTypeJ is FALSE, mimicking the 'else' block in heater_contorl
    if (isTypeJ)
    {
        targetTemp_adj = (targetTemp * 4);
    }
    else
    {
        targetTemp_adj = (targetTemp * 4) * 1.38;
    }

    // Main control window (condition from heater_contorl)
    if (((27 + currentTemp) > targetTemp) && (currentTemp < targetTemp + 9))
    {
        // Calculate base 'on-time' (ti) using the formula from heater_contorl
        int ti = (currentTemp_adj + telorance - (currentTemp_adj / 2.3)) * pidValue;
        ti = (ti / 10);

        if (ti > 0)
        {
            // Logic for 'stat_down' from heater_contorl
            if (currentTemp + 20 < targetTemp && stat_down == 0)
            {
                stat_down = 1;
                copy_time = millis();
            }
            if ((copy_time + 20000) < millis())
            {
                digitalWrite(heaterPin, ON); // Turn heater ON
            }

            // Calculate 'set_time' based on the difference between target and current temp
            int result_val = targetTemp_adj - currentTemp_adj;
            int set_time = 0;

            if (result_val >= 0)
            {
                set_time = ti + ti * result_val * 0.015;
            }
            else if (result_val + 10 >= 0)
            {
                set_time = ti + ti * result_val * 0.030;
            }
            else
            {
                set_time = ti + ti * result_val * 0.050;
            }

            // Use vTaskDelay as in the original manageHeater function
            if (set_time > 0)
            {
                vTaskDelay(pdMS_TO_TICKS(set_time));
            }

            digitalWrite(heaterPin, OFF); // Turn heater OFF
        }

        // Keep the original reset logic of the function
        if (targetTemp + 3 < currentTemp)
        {
            needsReset = true;
        }
    }
    // If temperature is too low, turn heater on constantly
    else if (currentTemp < targetTemp)
    {
        // Logic for 'stat_down' from heater_contorl
        if (currentTemp + 40 < targetTemp)
            stat_down = 0;
        digitalWrite(heaterPin, ON);
    }
    // If temperature is too high, turn heater off
    else
    {
        digitalWrite(heaterPin, OFF);
    }

    // --- END OF UPDATED LOGIC ---

    return needsReset;
}

static inline String getCorrectedDisplayString(int currentTemp, int targetTemp)
{

    if (targetTemp != last_target_temp)
    {
        last_target_temp = targetTemp;
        band_low_check = targetTemp;
        band_high_check = targetTemp;
        value_to_display = currentTemp;
        lastUpdateTime = millis();
    }

    if (currentTemp >= 1000)
    {
        return "---";
    }

    if ((5 + currentTemp >= band_low_check) && (currentTemp <= band_high_check + 3))
    {
        value_to_display = targetTemp;

        band_low_check = targetTemp;
        band_high_check = targetTemp;
    }
    else if (millis() - lastUpdateTime > 3000)
    {
        value_to_display = currentTemp;

        lastUpdateTime = millis();
        band_low_check = targetTemp + 5;
        band_high_check = targetTemp - 1;
    }
    return String(value_to_display);
}

#endif
