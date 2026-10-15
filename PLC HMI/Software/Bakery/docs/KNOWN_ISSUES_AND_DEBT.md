# Known Issues, Technical Debt & Vulnerability Analysis — Bakery HMI

This document records the vulnerabilities, race conditions, memory over-allocations, and architectural defects identified during direct source code auditing of the legacy firmware.

---

## 1. Critical Safety & Mechanical Hazards

### 1.1. Unbounded While Loops Without Sensor Timeout Watchdogs
- **Location:** `tasks.h` (Lines 169, 186, 201, 217, 248, 274, 302, 324).
- **Vulnerability:** Motion loops rely on raw sensor reads without a timeout guard:
  ```cpp
  while (digitalRead(FUNNELE_SENSOR) == LOW) {
      funneleLeft();
      vTaskDelay(pdMS_TO_TICKS(5));
      ...
  }
  ```
  If an inductive proximity sensor fails, its signal cable snaps, or flour dust obstructs detection, this loop runs indefinitely. The motor will continuously drive the mechanical assembly against physical hard stops, resulting in gearbox destruction, broken belts, or motor burnout.
- **Remediation Specification:** Implement a strict watchdog timeout (maximum 5000ms: `SENSOR_MOTION_TIMEOUT_MS`). Upon expiration without sensor confirmation, de-energize all motion outputs and trigger an emergency stop alarm (`STATE_SENSOR_FAULT`).

---

### 1.2. LVGL Multi-Core Thread-Safety Breach (Concurrent Mutation)
- **Location:** `tasks.h` (Lines 119-122, 129-132, 158-159, 246, 272, 296, 316, 337, 356-362) and `keypad_task`.
- **Vulnerability:** LVGL is not thread-safe. `app_task` and `keypad_task` on Core 0 directly invoke LVGL functions (`lv_label_set_text`, `lv_bar_set_value`, `lv_obj_add_state`) while `lvgl_task` on Core 1 concurrently executes `lv_timer_handler()`. This causes internal linked-list corruption, memory leaks, and unpredictable crashes under sustained operation.
- **Remediation Specification:** Guard all LVGL invocations across tasks using a global FreeRTOS mutex (`lv_mutex`) with timeout acquisition (`pdMS_TO_TICKS(50)`).

---

### 1.3. Spaghetti Jump Control Using `goto`
- **Location:** `tasks.h` (`BRAKE_M` and `BRAKE_A` labels at lines 166, 176, 193, 207, 225, 237, 258, 284, 309, 331).
- **Vulnerability:** The use of backward and forward `goto` jumps to interrupt continuous motion breaks modularity, violates clean state machine design, and introduces subtle edge cases where actuator states are not cleanly reset.
- **Remediation Specification:** Refactor the entire portioning sequence into a structured, non-blocking Finite State Machine class (`BakeryCycleFSM`) with distinct enum states.

---

### 1.4. FreeRTOS Stack Word Over-Allocation
- **Location:** `ChoneProject.ino` (Lines 152 to 158).
- **Vulnerability:** In ESP-IDF, `usStackDepth` represents 4-byte WORDS, not bytes. Allocating 8192 words to `keypad_task` consumes **32 KB** of internal DRAM for a task that merely samples two ADC pins. Furthermore, a diagnostic `test_task` is continuously spawned with 8192 words (32 KB), wasting precious internal memory.
- **Remediation Specification:** Remove `test_task` in production and downsize `keypad_task` stack to 2048 words (8 KB) and `buzzer_task` to 1024 words (4 KB), reclaiming over 50 KB of internal DRAM.

---

## 2. Security & Code Quality Debts

### 2.1. Hardcoded Access Passwords
- **Location:** `lvgl_function.h` (PIN `"123456"` at line 248, PIN `"654321"` at line 430).
- **Vulnerability:** Security PINs are hardcoded in source code, preventing bakery owners from setting custom security codes without recompiling firmware.
- **Remediation Specification:** Persist PIN hashes or encrypted values in NVS storage and provide an admin PIN change interface.

---

### 2.2. Redundant Label Updates with Artificial Delays
- **Location:** `tasks.h` (Lines 356 to 362):
  ```cpp
  vTaskDelay(pdMS_TO_TICKS(10));
  lv_label_set_text(ui_Menu_Counter_Lable, String(newval).c_str());
  vTaskDelay(pdMS_TO_TICKS(10));
  lv_label_set_text(ui_Menu_Counter_Lable, String(newval).c_str());
  ```
- **Vulnerability:** Unnecessary double-writes inserted as a workaround for unsynchronized display updates.
- **Remediation Specification:** Cleanly remove redundant writes once mutex-protected thread-safety is established.
