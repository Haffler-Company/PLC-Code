# Implementation Blueprint: Shift Production Statistics & Flour Accounting

**Document ID:** SPEC-005  
**Target Subsystem:** Industrial Accounting, RTC Analytics & NVS Logging  
**Hardware Interface:** Maxim DS3231 I2C RTC (GPIO 8/9) & Non-Volatile Flash  
**Standard:** Enterprise Batch Tracking & Bakery Inventory Metrics

---

## 1. Executive Summary & Objective

Commercial bakeries typically operate across two or three shifts per day (Morning, Afternoon, Night). Bakery managers need automated accounting to track:
1. Portion count produced in the current active shift.
2. Estimated bulk flour and dough consumed (in kilograms).
3. Shift-by-shift historical tallies stored securely across power cuts.

This specification blueprints the shift accounting engine and data persistence schema.

---

## 2. Shift Schedule Definition & Time Boundaries

The system evaluates the real-time clock from `rtc.now()` against pre-configured shift hours:

| Shift Identifier | Shift Window | Typical Operator Role |
|---|---|---|
| **Shift 1 (Morning)** | 05:00 to 13:00 | Morning bread rush (Lavash/Barbari) |
| **Shift 2 (Afternoon)**| 13:00 to 21:00 | Afternoon/evening production |
| **Shift 3 (Night)** | 21:00 to 05:00 | Pre-dough preparation and special orders |

---

## 3. Data Structures & NVS Persistence Schema

```cpp
struct ShiftRecord {
    uint32_t dateYMD;         // e.g. 20260907
    uint8_t  shiftId;         // 1, 2, or 3
    uint32_t portionCount;    // Total dough balls portioned
    float    avgWeightGrams;  // Nominal weight per portion
    float    totalFlourKg;    // Estimated flour (calculated)
    uint32_t activeRunTimeSec;// Actual motor running duration
};
```

### 3.1. Mathematical Formula for Flour Consumption:
$$\text{Total Dough (kg)} = \frac{\text{portionCount} \times \text{nominalWeightGrams}}{1000}$$
Assuming standard 60% baker's hydration (100 kg flour + 60 kg water + 2 kg yeast/salt = 162 kg dough):
$$\text{Flour Consumption (kg)} = \frac{\text{Total Dough (kg)}}{1.62}$$

---

## 4. Shift Manager Class Implementation Blueprint

```cpp
#include <RTClib.h>
#include "storage.h"

class ShiftManager {
private:
    uint8_t currentShift = 1;
    uint32_t shiftPortions = 0;
    uint32_t lastCheckDay = 0;

public:
    void init(RTC_DS3231 &rtc);
    void onPortionCompleted();
    void updateShiftTime(const DateTime &now);
    uint32_t getShiftPortions() const { return shiftPortions; }
    uint8_t getCurrentShiftId() const { return currentShift; }
    float getEstimatedFlourKg(float portionWeightGrams) const;
};

void ShiftManager::updateShiftTime(const DateTime &now) {
    uint8_t hour = now.hour();
    uint8_t newShift = 1;
    if (hour >= 5 && hour < 13) {
        newShift = 1;
    } else if (hour >= 13 && hour < 21) {
        newShift = 2;
    } else {
        newShift = 3;
    }

    // Detect shift turnover
    if (newShift != currentShift) {
        saveShiftHistory(currentShift, shiftPortions, now);
        currentShift = newShift;
        shiftPortions = 0; // Reset tally for new shift
    }
}
```

---

## 5. UI Integration on Main Dashboard

On `ui_Menu` and `ui_AutomaticMode`:
- Display current shift indicator badge (e.g. `[شیفت صبح - ۱]`).
- Display current shift tally alongside total all-time cumulative tally.
