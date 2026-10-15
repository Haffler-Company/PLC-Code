# Implementation Blueprint: Firmware Core Hardening, FSM & LVGL Mutex

**Document ID:** SPEC-001  
**Target Subsystem:** Motion Control, Process Safety & FreeRTOS Architecture  
**Primary Files to Modify:** `tasks.h`, `general.h`, `ChoneProject.ino`, `defines.h`  
**Standard:** Siemens Industrial Determinism & IEC 61131-3 FSM Design

---

## 1. Executive Summary & Objective

This specification provides the exact, production-ready blueprint to:
1. Eliminate infinite while loops by implementing bounded sensor watchdog timeouts (`SENSOR_MOTION_TIMEOUT_MS = 5000`).
2. Replace unstructured `goto` jumps with an explicit, non-blocking Finite State Machine (`BakeryCycleFSM`).
3. Solve the multi-core LVGL concurrency hazard by introducing a thread-safe FreeRTOS Mutex (`lv_mutex`).
4. Optimize FreeRTOS task stack allocations, eliminating the diagnostic task and saving ~50 KB of internal DRAM.

---

## 2. Sensor Watchdog Timeout Architecture

### 2.1. The Problem in Legacy Code:
```cpp
// DANGEROUS: If sensor breaks, this loop never terminates
while (digitalRead(FUNNELE_SENSOR) == LOW) {
    funneleLeft();
    vTaskDelay(pdMS_TO_TICKS(5));
}
```

### 2.2. Standardized Implementation Blueprint:
Define in `defines.h`:
```cpp
#define SENSOR_MOTION_TIMEOUT_MS 5000
```

Implement in motion control loop:
```cpp
bool waitForSensorWithTimeout(uint8_t sensorPin, uint8_t targetState, uint32_t timeoutMs) {
    uint32_t start = millis();
    while (digitalRead(sensorPin) != targetState) {
        if (millis() - start >= timeoutMs) {
            // Watchdog Tripped: Fail-Safe Shutdown
            pistonStop();
            funneleStop();
            return false; // Timeout failure
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return true; // Sensor successfully reached
}
```

---

## 3. Finite State Machine (FSM) Implementation Blueprint

Create a dedicated header/source structure `bakery_cycle_fsm.h`:

```cpp
#ifndef BAKERY_CYCLE_FSM_H
#define BAKERY_CYCLE_FSM_H

#include <Arduino.h>

enum class CycleState {
    IDLE,
    FUNNEL_HOME,
    PISTON_HOME,
    FUNNEL_FEED,
    PISTON_DISCHARGE,
    CYCLE_DELAY,
    FAULT_SENSOR_TIMEOUT,
    EMERGENCY_STOP
};

class BakeryCycleFSM {
private:
    CycleState currentState = CycleState::IDLE;
    uint32_t phaseStartTime = 0;

public:
    void startCycle();
    void stopCycle();
    void updateFSM(); // Called cyclically inside app_task
    CycleState getState() const { return currentState; }
};

#endif
```

### 3.1. Non-Blocking State Transitions inside `updateFSM()`:
```cpp
void BakeryCycleFSM::updateFSM() {
    switch (currentState) {
        case CycleState::IDLE:
            pistonStop();
            funneleStop();
            break;

        case CycleState::FUNNEL_HOME:
            funneleLeft();
            if (digitalRead(FUNNELE_SENSOR) == LOW) {
                funneleStop();
                phaseStartTime = millis();
                currentState = CycleState::PISTON_HOME;
            } else if (millis() - phaseStartTime >= SENSOR_MOTION_TIMEOUT_MS) {
                pistonStop();
                funneleStop();
                currentState = CycleState::FAULT_SENSOR_TIMEOUT;
            }
            break;

        case CycleState::PISTON_HOME:
            pistonUP();
            if (digitalRead(PISTON_SENSOR) == LOW) {
                pistonStop();
                phaseStartTime = millis();
                currentState = CycleState::FUNNEL_FEED;
            } else if (millis() - phaseStartTime >= SENSOR_MOTION_TIMEOUT_MS) {
                pistonStop();
                funneleStop();
                currentState = CycleState::FAULT_SENSOR_TIMEOUT;
            }
            break;

        case CycleState::FUNNEL_FEED:
            funneleRight();
            if (millis() - phaseStartTime >= storage.get_setting_max_funnle()) {
                funneleStop();
                phaseStartTime = millis();
                currentState = CycleState::PISTON_DISCHARGE;
            }
            break;

        case CycleState::PISTON_DISCHARGE:
            pistonDown();
            if (millis() - phaseStartTime >= storage.getAutoWeight()) {
                pistonStop();
                // Complete portion, notify buzzer & increment tally
                storage.setChoneCounter(storage.getChoneCounter() + 1);
                if (storage.get_activeCounterSound()) {
                    xTaskNotifyGive(buzzerTaskHandle);
                }
                phaseStartTime = millis();
                currentState = CycleState::CYCLE_DELAY;
            }
            break;

        case CycleState::CYCLE_DELAY:
            pistonStop();
            funneleStop();
            if (millis() - phaseStartTime >= storage.getAutoSpeed()) {
                phaseStartTime = millis();
                currentState = CycleState::FUNNEL_HOME; // Next portion
            }
            break;

        case CycleState::FAULT_SENSOR_TIMEOUT:
        case CycleState::EMERGENCY_STOP:
            pistonStop();
            funneleStop();
            break;
    }
}
```

---

## 4. LVGL Thread-Safety Mutex Pattern

### 4.1. Declaration in `ChoneProject.ino`:
```cpp
SemaphoreHandle_t lv_mutex = NULL;

void setup() {
    lv_mutex = xSemaphoreCreateMutex();
    ...
}
```

### 4.2. Safe Invocation Macro:
```cpp
#define LV_SAFE_CALL(action) \
    do { \
        if (lv_mutex && xSemaphoreTake(lv_mutex, pdMS_TO_TICKS(50)) == pdTRUE) { \
            action; \
            xSemaphoreGive(lv_mutex); \
        } \
    } while (0)
```

### 4.3. Usage across Core 0 Tasks:
```cpp
// Inside app_task or keypad_task:
LV_SAFE_CALL({
    lv_label_set_text(ui_Menu_Counter_Lable, String(newval).c_str());
    lv_bar_set_value(ui_AutomaticMode_Bar2, progressPercent, LV_ANIM_ON);
});

// Inside lvgl_task on Core 1:
void lvgl_task(void *param) {
    while (true) {
        if (lv_mutex && xSemaphoreTake(lv_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(lv_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

---

## 5. FreeRTOS Task Stack Rightsizing

In `ChoneProject.ino`:
1. **Eliminate:** Delete `test_task` entirely.
2. **Right-size Stacks:**
   - `lv_tick_task`: 2048 words (Core 1)
   - `lvgl_task`: 4096 words (Core 1)
   - `app_task`: 4096 words (Core 0)
   - `keypad_task`: 2048 words (Core 0)
   - `buzzer_task`: 1024 words (Core 0)
