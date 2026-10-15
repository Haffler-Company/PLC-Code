#ifndef SHIFT_MANAGER_H
#define SHIFT_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>
#include <Preferences.h>

//? ============================================================================
//? SHIFT-BASED PRODUCTION STATISTICS & FLOUR ACCOUNTING (DS3231 + NVS)
//? ============================================================================
// Shift Schedule:
// - Shift 1 (Morning):   05:00 to 13:00 (Breakfast / Lavash / Barbari rush)
// - Shift 2 (Afternoon): 13:00 to 21:00 (Evening peak)
// - Shift 3 (Night):     21:00 to 05:00 (Pre-dough batching / Special orders)

class ShiftManager {
private:
    Preferences prefs;
    uint8_t currentShift = 1;
    uint32_t shiftPortions = 0;
    uint32_t shiftActiveRuntimeSec = 0;
    uint32_t lastCheckDay = 0;
    unsigned long lastSaveMs = 0;

    // Standard 60% baker's hydration ratio: 100kg flour + 60kg water + 2kg yeast/salt = 162kg dough
    static constexpr float DOUGH_TO_FLOUR_RATIO = 1.62f;

public:
    void begin() {
        prefs.begin("bakery_shift", false);
        currentShift = prefs.getUChar("cur_shift", 1);
        shiftPortions = prefs.getULong("shift_portions", 0);
        lastCheckDay = prefs.getULong("last_day", 0);
    }

    uint8_t calculateShiftId(uint8_t hour) {
        if (hour >= 5 && hour < 13) {
            return 1; // Morning
        } else if (hour >= 13 && hour < 21) {
            return 2; // Afternoon
        } else {
            return 3; // Night
        }
    }

    const char* getShiftName(uint8_t shiftId) const {
        switch (shiftId) {
            case 1:  return "صبح (Shift 1)";
            case 2:  return "عصر (Shift 2)";
            case 3:  return "شب (Shift 3)";
            default: return "نامشخص";
        }
    }

    const char* getCurrentShiftName() const {
        return getShiftName(currentShift);
    }

    void updateTime(const DateTime &now) {
        uint8_t newShift = calculateShiftId(now.hour());
        uint32_t todayYMD = (uint32_t)now.year() * 10000UL + (uint32_t)now.month() * 100UL + (uint32_t)now.day();

        // Detect shift turnover
        if (newShift != currentShift || (lastCheckDay != 0 && todayYMD != lastCheckDay && newShift == 1)) {
            // Save archive for previous shift
            char key[16];
            snprintf(key, sizeof(key), "s%d_p", currentShift);
            prefs.putULong(key, shiftPortions);

            // Turnover to new shift
            currentShift = newShift;
            shiftPortions = 0;
            shiftActiveRuntimeSec = 0;
            lastCheckDay = todayYMD;

            prefs.putUChar("cur_shift", currentShift);
            prefs.putULong("shift_portions", 0);
            prefs.putULong("last_day", lastCheckDay);
        }
    }

    void onPortionCompleted() {
        shiftPortions++;
        // Periodic NVS save every 10 portions or every 60 seconds
        if ((shiftPortions % 10 == 0) || (millis() - lastSaveMs > 60000UL)) {
            commit();
        }
    }

    void commit() {
        prefs.putUChar("cur_shift", currentShift);
        prefs.putULong("shift_portions", shiftPortions);
        lastSaveMs = millis();
    }

    void resetShiftPortions() {
        shiftPortions = 0;
        commit();
    }

    uint8_t getCurrentShiftId() const {
        return currentShift;
    }

    uint32_t getShiftPortions() const {
        return shiftPortions;
    }

    // Flour estimation: (portions * weightGrams / 1000) / 1.62
    float getEstimatedFlourKg(int portionWeightGrams) const {
        if (portionWeightGrams <= 0) portionWeightGrams = 150; // default 150g
        float totalDoughKg = ((float)shiftPortions * (float)portionWeightGrams) / 1000.0f;
        return totalDoughKg / DOUGH_TO_FLOUR_RATIO;
    }
};

extern ShiftManager shiftManager;

#endif // SHIFT_MANAGER_H
