#ifndef PANEL_KEY_FILTER_H
#define PANEL_KEY_FILTER_H

#include <Arduino.h>
#include "general.h"

extern SemaphoreHandle_t adcMutex;

namespace panel_key_filter {

static constexpr int kKeyCenters[5] = {4050, 3350, 2700, 1950, 1250};
static constexpr int kPrimaryWindow = 360;
static constexpr int kSeparationMargin = 170;
static constexpr uint32_t kDebounceMs = 45;
static constexpr uint8_t kSamples = 5;

static inline int readAdcSafe(int analogPin) {
    // فقط وقتی mutex را واقعاً گرفتیم give کن؛ وگرنه give کردنِ mutexِ بدون‌مالکیت
    // باعثِ «assert xQueueGenericSend» و کرش/ریست می‌شد (تایم‌اوتِ ۸ms زیرِ فشار رخ می‌داد).
    bool locked = (::adcMutex && xSemaphoreTake(::adcMutex, pdMS_TO_TICKS(50)) == pdTRUE);
    int v = analogRead(analogPin);
    if (locked) xSemaphoreGive(::adcMutex);
    return v;
}

static inline int readMedian5(int analogPin) {
    int v[kSamples];
    bool locked = (::adcMutex && xSemaphoreTake(::adcMutex, pdMS_TO_TICKS(30)) == pdTRUE);
    for (uint8_t i = 0; i < kSamples; ++i) {
        v[i] = analogRead(analogPin);
        delayMicroseconds(60);
    }
    if (locked) xSemaphoreGive(::adcMutex);
    for (uint8_t i = 0; i < kSamples - 1; ++i) {
        for (uint8_t j = i + 1; j < kSamples; ++j) {
            if (v[j] < v[i]) {
                int t = v[i];
                v[i] = v[j];
                v[j] = t;
            }
        }
    }
    return v[kSamples / 2];
}

static inline int classifyKeyFromAdc(int adc) {
    int bestIdx = -1;
    int bestDist = 100000;
    int secondDist = 100000;

    for (int i = 0; i < 5; ++i) {
        int d = abs(adc - kKeyCenters[i]);
        if (d < bestDist) {
            secondDist = bestDist;
            bestDist = d;
            bestIdx = i;
        } else if (d < secondDist) {
            secondDist = d;
        }
    }

    if (bestIdx < 0) return 0;
    if (bestDist > kPrimaryWindow) return 0;
    if ((secondDist - bestDist) < kSeparationMargin) return 0;
    return bestIdx + 1;
}

static inline int readDebouncedButtonRobust(int analogPin, ButtonState &state) {
    const int adc = readMedian5(analogPin);
    const int reading = classifyKeyFromAdc(adc);
    const uint32_t now = millis();

    if (reading != state.lastReading) {
        state.lastReading = reading;
        state.lastChangeTime = now;
        return 0;
    }

    if ((now - state.lastChangeTime) < kDebounceMs) {
        return 0;
    }

    state.stableKey = reading;
    return state.stableKey;
}

static inline void resetState(ButtonState &state) {
    state.lastReading = 0;
    state.stableKey = 0;
    state.lastChangeTime = millis();
}

static inline bool readBackButtonRobust(int analogPin, bool lastStableState) {
    // برگشت روی باندِ بالای پایه‌ی میز (GPIO4): سوییچِ برگشت adc را بالا می‌برد.
    //   میزِ باز ~1550 و «هیچ‌کدوم» ~2000 = رها ؛ برگشت (~2500 «هر دو» یا ~4095) = فشرده.
    const int adc = readMedian5(analogPin);
    if (!lastStableState) {
        return adc > 2300;   // پرس
    }
    return adc > 2050;       // رها با هیسترزیس
}

} // namespace panel_key_filter

#endif // PANEL_KEY_FILTER_H
