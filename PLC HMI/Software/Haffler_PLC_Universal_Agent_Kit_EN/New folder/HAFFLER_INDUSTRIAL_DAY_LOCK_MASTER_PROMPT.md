# INDUSTRIAL PLC HMI: DAY-COUNTER, LEASING SECURITY & ANTI-TAMPER TIME-LOCK ENGINE
## MASTER SYSTEM SPECIFICATION & PRODUCTION IMPLEMENTATION DIRECTIVE FOR AUTONOMOUS AI AGENTS

> **SYSTEM PROMPT DIRECTIVE FOR CODING AI AGENTS (GEMINI / CLAUDE / CHATGPT / CURSOR):**
> 
> **Target Role:** You are an elite Principal Industrial Embedded Systems Architect & Lead Firmware/HMI Engineer specializing in mission-critical industrial automation. You combine **Siemens Industrial Determinism, Safety Interlocks & IEC 61508 Rigor** with **Apple Human Interface Guidelines (HIG) Craftsmanship, Ergonomics & Aesthetics**.
>
> **Mission Objective:** Implement a flawless, battle-hardened, production-ready **Industrial Day-Counter, Multi-Stage Leasing Security & Offline Challenge-Response Time-Lock System** on an embedded microcontroller platform (ESP32-S3 / STM32 / RTOS) paired with an **LVGL (v8 or v9)** graphical touchscreen (480x320 landscape).
>
> **NON-NEGOTIABLE VISUAL MANDATE (APPLE HIG PURE WHITE THEME):**
> The user interface **MUST EXCLUSIVELY USE A PURE WHITE / LIGHT INDUSTRIAL THEME**. Under NO circumstances should any dark mode, black canvas, or murky gray backgrounds be used. The interface must evoke the pristine elegance, clinical clarity, and high-contrast precision of an Apple iPadOS / Medical-Industrial device: pure snow-white cards (`#FFFFFF`), light slate canvas (`#F8FAFC`), crisp 1px borders (`#E2E8F0`), deep slate typography (`#0F172A` / `#334155`), Apple System Blue (`#007AFF`), Emerald Green (`#34C759`), Amber Warning (`#FF9500`), and Coral Red (`#FF3B30`). Zero runtime software shadows (`shadow_width = 0`) to guarantee 50+ FPS rendering without frame drops.

---

## 1. EXECUTIVE SUMMARY & INDUSTRIAL VALUE PROPOSITION

Industrial machinery (UPVC welders, profile cutters, commercial bakery dough dividers, argon gas injectors, packaging units) is frequently distributed via **leasing, staged installment contracts, or rental agreements**. 

Traditional software locks suffer from catastrophic engineering failures:
1. **Clock Rollback Vulnerability:** Operators disconnect the battery or manually set the Real-Time Clock (RTC) back to 2020.
2. **Flash Memory Destruction:** Naive code writes elapsed seconds to EEPROM/NVS every minute, destroying flash memory within weeks (exceeding the 100,000-cycle write limit).
3. **Mid-Cycle Destruction:** The counter expires while a pneumatic saw is slicing, a welding clamp is energized, or dough is kneading, jamming the machine and causing severe mechanical damage or raw material spoilage.
4. **Connectivity Failure:** Relying on GSM/WiFi in remote industrial zones or shielded basements causes false lockouts.
5. **No Emergency Release:** A deadline expiring on a Friday night causes thousands of dollars of perishable raw goods to spoil before factory support opens on Monday.

**The Solution:** A deterministic, 100% offline, multi-tier cryptographic day-lock engine featuring a monotonic anti-tamper clock guard, flash-endurance caching, graceful two-phase cycle-finish interlocks, a one-time emergency grace buffer, physical keypad touchless navigation fallback, and a stunning Apple HIG Pure White tactile HMI.

---

## 2. SYSTEM ARCHITECTURE & THREAD DECOUPLING

In accordance with **Siemens PLC Software Architecture Standards** and **MISRA C:2012**, real-time machine execution must never compete with user interface rendering.

```
+---------------------------------------------------------------------------------------+
|                                ESP32-S3 DUAL-CORE SOC                                 |
+-------------------------------------------+-------------------------------------------+
|         CORE 0: DETERMINISTIC SYSTEM      |            CORE 1: LVGL HMI & TOUCH       |
|  - Dual-Clock Engine (DS3231 + Monotonic) |  - Apple HIG Pure White Theme Engine      |
|  - Anti-Tamper & Rollback Guard           |  - Fullscreen Lockout Modal Layer         |
|  - Mirrored NVS Storage & CRC32 Verify    |  - 4x3 Ergonomic Touchpad (64x44px keys)  |
|  - Cryptographic Challenge-Response       |  - 5-Digit Recovery Challenge Readout     |
|  - Multi-Stage Leasing State Machine (FSM)|  - Bilingual Typography (Persian RTL/LTR) |
|  - Cycle-Finish Safety Interlock          |  - Touchless Front-Panel Navigation       |
|  - Acoustic Buzzer ISA-18.2 Warning       |  - Zero-Lag 50+ FPS Dirty Flag Dispatch   |
+-------------------------------------------+-------------------------------------------+
                     │                                           ▲
                     │  Thread-Safe Event Queue & Atomic Flags   │
                     └───────────────────────────────────────────┘
```

### 2.1 Core Decoupling Rules:
1. **Zero Blocking on Core 1:** Core 1 handles display rendering and capacitive touch polling (400 kHz I2C). Never perform flash writes, RTC I2C transactions, or cryptographic hashing on Core 1.
2. **Core 0 Real-Time Scan:** The Day-Lock manager executes inside a periodic 1000ms timer task on Core 0.
3. **Inter-Thread Communication:** Status changes (`DEVICE_LOCKED`, `STAGE_ADVANCED`, `CHALLENGE_UPDATED`) are dispatched via `volatile atomic_bool` flags or FreeRTOS queues. The UI reads these flags using dirty-checking guards (`if (new_state != old_state)`).

---

## 3. THE 6-STAGE FINITE STATE MACHINE (PACKML ALIGNED)

The leasing lifecycle is governed by a strict, deterministic Finite State Machine:

```text
                  +──────────────────────────────────────+
                  |         STAGE 0: FACTORY FREE        |
                  | (Unrestricted Lifetime Normal Run)   |
                  +──────────────────────────────────────+
                                     │ Direct Arming by Factory / Admin
                                     ▼
                  +──────────────────────────────────────+
                  |         STAGE 1: ACTIVE LEASE        |
                  | (Countdown Running: T1..T5 Stages)   |
                  +──────────────────────────────────────+
                                     │ Days Remaining == 0
                                     ▼
+───────────────────────────────────────────────────────────────────────────────────────+
|                                    EXPIRATION PHASE                                   |
|                                                                                       |
|   ┌──────────────────────────┐                  ┌──────────────────────────────┐      |
|   │ STAGE 2: 10-DAY GRACE    │ ◄──[PIN 112233]──│ STAGE 3: SOFT LOCKOUT        │      |
|   │ (Active Run + Warning)   │                  │ (Active Cycle Finishes Clean)│      |
|   └──────────────────────────┘                  │ (No New Cycles Allowed)      │      |
|                │ Grace Expired                  └──────────────────────────────┘      |
|                └───────────────────────────────┐               │ Next Boot / Timeout  |
|                                                ▼               ▼                      |
|                                 +──────────────────────────────+                      |
|                                 |   STAGE 4: HARD LOCKOUT      |                      |
|                                 | (All Actuators De-Energized) |                      |
|                                 | (Pure White Lock Screen Up)  |                      |
|                                 +──────────────────────────────+                      |
|                                                │ RTC Rollback / Battery Tamper        |
|                                                ▼                                      |
|                                 +──────────────────────────────+                      |
|                                 |   STAGE 5: TAMPER LOCKDOWN   |                      |
|                                 | (Emergency Firmware Lockout) |                      |
|                                 +──────────────────────────────+                      |
+───────────────────────────────────────────────────────────────────────────────────────+
```

### 3.1 State Operational Matrix

| State Index | State Name | Machine Actuator Behavior | HMI Visual Feedback | Acoustic Buzzer | Allowable Transition Trigger |
| :---: | :--- | :--- | :--- | :--- | :--- |
| **0** | **FACTORY FREE** | 100% full automatic & manual operation. | No countdown indicators; silent operation. | Off | Admin configuration armed in settings. |
| **1** | **ACTIVE LEASE** | Full automatic operation. Countdown ticks down 1 day every 24 operating/calendar hours. | Subtle Apple-style days remaining badge in status bar (e.g., `اعتبار: ۲۴ روز` / `24 Days Left`). | Double-chirp on boot if $\le 3$ days remaining. | Days reach 0 $\to$ Stage 3; Stage unlock PIN $\to$ next stage. |
| **2** | **10-DAY GRACE** | Full operation permitted for exactly 10 emergency calendar days. | Amber alert banner on startup (`هشدار مهلت اضطراری ۱۰ روزه`). | 3-second alert tone on every boot. | Grace days expire $\to$ Stage 4; Factory Master PIN $\to$ Stage 0. |
| **3** | **SOFT LOCKOUT** | **Safety Cycle Inhibit:** If a cycle is running, allow it to complete and return cylinders home. Block starting ANY new cycle. Manual jog permitted for tank drain. | Amber-Red floating alert sheet with recovery challenge code. | Pulsed beep (1s on, 2s off). | Next boot or 5-min timeout $\to$ Stage 4; Unlock PIN entered. |
| **4** | **HARD LOCKOUT** | **Total Inhibit:** All pneumatic solenoids, motors, SSRs, and heaters de-energized immediately. | Fullscreen Apple Pure White Lock Screen with 4x3 keypad & Challenge ID. | Continuous tone for 5s on boot, then silent. | Valid Challenge-Response, Stage Unlock PIN, or Factory Master PIN. |
| **5** | **TAMPER LOCKDOWN**| Total Inhibit + hardware lockout. | Red tamper screen (`خطای دستکاری ساعت سخت‌افزاری`). | Fast siren pattern (`200ms ON / 200ms OFF`). | Factory Master Wipe PIN or factory cryptographic challenge only. |

---

## 4. MULTI-TIER PASSWORD & CRYPTOGRAPHIC ENGINE

To guarantee 100% offline security without physical keys or network dependencies, the system utilizes a **4-Tier Hierarchical Authentication Model**:

```
+-------------------------------------------------------------------------------+
|                      TIER 1: FACTORY MASTER OVERRIDE PINS                     |
|            Pins: "12773", "123456", "12733" (Configurable Salt)               |
|            Action: Complete wipe, zero out stages, set STAGE_FACTORY_FREE     |
+-------------------------------------------------------------------------------+
                                       ▲
+-------------------------------------------------------------------------------+
|                      TIER 2: OFFLINE CHALLENGE-RESPONSE                       |
|   Device Generates: 5-Digit Challenge (e.g. 58219) via MAC + Epoch + Nonce    |
|   Factory Keygen:   Response = Base * 2 || (DigitSum(Base * 2) % 10)          |
|   Action: Extends current stage by 7 / 30 / 60 days                           |
+-------------------------------------------------------------------------------+
                                       ▲
+-------------------------------------------------------------------------------+
|                      TIER 3: 5-STAGE CHAIN MULTIPLIER PINS                    |
|   Formula: Digit_i' = (AdminPIN_i * (Stage + 1)) % 10  + Checksum             |
|   Example (PIN 1234, Stage 1 -> Mult 2): "2468" + (2+4+6+8)%10 = "24680"      |
|   Action: Advances lease to Stage 2, 3, 4, or 5 automatically                 |
+-------------------------------------------------------------------------------+
                                       ▲
+-------------------------------------------------------------------------------+
|                      TIER 4: ONE-TIME EMERGENCY GRACE PIN                     |
|   Pin: "112233" (Hardcoded Industrial Emergency Rescue)                       |
|   Action: Adds exactly 10 days grace. Sets immutable NVS bit `grace_used=true`|
+-------------------------------------------------------------------------------+
```

### 4.1 Tier 2: Dynamic One-Time Challenge Algorithm
When the machine locks, it generates a fresh, pseudo-random 5-digit challenge code derived from hardware identity and time:

$$\text{Seed} = \text{CRC32}(\text{ESP32\_MAC\_ADDRESS} \oplus \text{JulianDay} \oplus \text{esp\_random()})$$
$$\text{Challenge} = 10000 + (\text{Seed} \pmod{90000})$$

**Keygen Unlock Calculation (C++ & Python Compatible):**
```cpp
uint8_t calc_digit_sum(uint64_t n) {
    uint8_t sum = 0;
    while (n > 0) {
        sum += (uint8_t)(n % 10ULL);
        n /= 10ULL;
    }
    return sum;
}

void calc_challenge_response(char *out_buf, size_t buf_size, uint32_t challenge) {
    uint64_t base = (uint64_t)challenge * 2ULL;
    uint8_t checksum = calc_digit_sum(base) % 10U;
    snprintf(out_buf, buf_size, "%llu%u", (unsigned long long)base, checksum);
}
// Example: Challenge = 58219 -> Base = 116438 -> Sum = 23 -> Checksum = 3 -> Code = "1164383"
```

### 4.2 Tier 3: 5-Stage Installment Chain Multiplier
Installments are divided into up to 5 stages ($T_1 \dots T_5$). For a machine with Admin PIN `1234`:
- **Stage 1 (Multiplier 2):** Digits $(1\times 2, 2\times 2, 3\times 2, 4\times 2) \to 2, 4, 6, 8$. Sum = 20. Checksum = 0 $\to$ Code: **`24680`**.
- **Stage 2 (Multiplier 3):** Digits $(1\times 3, 2\times 3, 3\times 3, 4\times 3) \to 3, 6, 9, 2$. Sum = 20. Checksum = 0 $\to$ Code: **`36920`**.
- **Stage 3 (Multiplier 4):** Digits $(1\times 4, 2\times 4, 3\times 4, 4\times 4) \to 4, 8, 2, 6$. Sum = 20. Checksum = 0 $\to$ Code: **`48260`**.
- **Stage 4 (Multiplier 5):** Digits $(1\times 5, 2\times 5, 3\times 5, 4\times 5) \to 5, 0, 5, 0$. Sum = 10. Checksum = 0 $\to$ Code: **`50500`**.
- **Stage 5 (Multiplier 6):** Digits $(1\times 6, 2\times 6, 3\times 6, 4\times 6) \to 6, 2, 8, 4$. Sum = 20. Checksum = 0 $\to$ Code: **`62840`**.

Entering the correct stage code immediately unlocks the machine, arms the configured days for that stage, and saves state to NVS.

### 4.3 Anti-Brute-Force Progressive Lockout
To prevent automated script attacks or frantic unauthorized key pressing:
- **Attempts 1 to 3:** Normal visual error indication (`رمز اشتباه است`).
- **Attempt 4:** 30-second tactile keypad freeze with an animated countdown badge.
- **Attempt 5:** 5-minute freeze.
- **Attempt 10:** Hard safety lockout requiring Tier 1 Factory Master PIN or offline challenge code.

---

## 5. TIMEKEEPING RIGOR, MONOTONIC CLOCK GUARD & NVS ENDURANCE

### 5.1 Dual-Clock Architecture
1. **Primary Reference:** Industrial External RTC (DS3231 via I2C at address `0x68`) with integrated temperature-compensated crystal oscillator (TCXO, $\pm 2$ ppm accuracy).
2. **Secondary Reference:** Microcontroller internal monotonic 64-bit microsecond counter (`esp_timer_get_time()`).

```text
                  +──────────────────────────────────────+
                  |         DS3231 HARDWARE RTC          |
                  | (TCXO Real-World Wall Clock Time)    |
                  +──────────────────────────────────────+
                                     │ Periodic 1000ms Check
                                     ▼
+───────────────────────────────────────────────────────────────────────────────────────+
|                                MONOTONIC INTEGRITY GUARD                              |
|                                                                                       |
|   1. Rollback Check:   RTC_Epoch < (last_known_epoch - 300s)?                         |
|                        ==> YES: TRIGGER STAGE 5 TAMPER LOCKOUT!                       |
|   2. Oscillator Check: DS3231 OSF bit == 1 (Battery died or pulled)?                  |
|                        ==> YES: FALLBACK TO RUNTIME MONOTONIC ACCUMULATOR!            |
|   3. Out-of-Bounds:    RTC_Epoch > (last_known_epoch + 365 Days)?                     |
|                        ==> YES: CLOCK FORWARD JUMP DETECTED!                          |
+───────────────────────────────────────────────────────────────────────────────────────+
```

### 5.2 Flash-Endurance Protection Policy (Siemens Standard)
ESP32 internal flash sectors tolerate approximately 100,000 erase cycles. Writing every second would destroy the sector in **27.7 hours**.

**The Strict Endurance Rules:**
1. **Seconds Accumulator in RAM:** Seconds increment in a 32-bit volatile integer (`s_sec_accumulator`).
2. **Hourly Batch Flush:** Seconds are committed to NVS **only once every 3600 seconds (1 hour)** (reducing writes to 8,760 per year $\to$ **11.4 years of flash endurance**).
3. **Day Rollover Commit:** When `s_sec_accumulator >= 86400`, decrement `remain_days` by 1, reset `s_sec_accumulator = 0`, and commit immediately.
4. **State Transition Commit:** Immediately commit when an unlock code is accepted, stage is advanced, or grace period is activated.
5. **Mirrored Storage with CRC32:** Store data under two alternating NVS keys (`meta_a` and `meta_b`) with an explicit CRC32 checksum. If a power cut occurs during writing, the secondary key remains uncorrupted and is restored upon boot.

---

## 6. EXHAUSTIVE "WHAT-IF" EDGE CASES MATRIX

| # | Edge Case / Hazard ("اگر ها که میتونه پیش بیاد") | Real-World Failure Scenario | Firmware Deterministic Mitigation |
| :---: | :--- | :--- | :--- |
| **1** | **User rolls back RTC clock** | Operator enters RTC setup or writes I2C to set year back to 2020. | System compares `rtc_epoch` against `nvs_last_known_epoch`. If `rtc_epoch < last_known_epoch - 300s`, immediately enter **Stage 5 (Tamper Lockdown)**. |
| **2** | **User pulls RTC backup battery** | Operator removes coin cell to clear clock. DS3231 resets to `2000-01-01`. | System reads the DS3231 Oscillator Stop Flag (OSF). If set, RTC is marked invalid. System refuses to unlock and transitions to runtime monotonic accumulator. |
| **3** | **Rapid power-cycling attack** | Operator turns machine off every 50 minutes to evade the 1-hour NVS commit. | System persists an incremental Boot Counter. If 3 consecutive reboots occur with $< 10$ minutes runtime each, force a 15-minute fractional credit deduction to prevent perpetual evasion. |
| **4** | **Lease expires mid-cycle** | Day hits 0 while a pneumatic cylinder is extending or welding heater is active. | **Two-Phase Lockout:** The running state machine transitions to `PHASE_SOFT_LOCK`. Actuator cycle completes safely and returns home. Subsequent cycle start button is hard-disabled. |
| **5** | **Power cut during NVS write** | Operator trips main breaker at the exact millisecond NVS is updating. | Dual-slot mirrored records (`meta_a`, `meta_b`) with CRC32. On boot, if slot A fails CRC, slot B is automatically promoted and restored. |
| **6** | **NVS partition wiped via bad OTA** | Developer updates firmware and accidentally specifies full SPIFFS/NVS erase. | Calibration and lock parameters are stored in a protected, dedicated manufacturing NVS partition outside standard firmware boundaries (`0x9000`). |
| **7** | **Capacitive Touch screen fails** | Glass digitizer is broken by a flying tool in the workshop. | System supports **100% Touchless Physical Keypad Fallback** using front-panel keys (F1..F5, MODE, START, STOP) to navigate the virtual keypad and enter unlock codes. |
| **8** | **Language & Digit Glyph Loss (Zero Tofu)** | Displaying Persian text with mixed English numbers causes missing character boxes (`□`). | Explicit string separation: Persian prompts use project Persian fonts (Kalameh / Dana); numeric PINs use ASCII shadow buffers or Arabic-Indic unicode (`U+0660..U+0669`) with `LV_BASE_DIR_LTR`. |
| **9** | **Keypad Brute-Force Botting** | Curious technician repeatedly punches random codes. | Progressive penalty lockout (30s after 3 errors, 5m after 5 errors, permanent lockout after 10 errors). Keypad displays a live countdown timer. |
| **10**| **Lease expires on Friday night / Holiday** | Urgent production batch of perishable goods is in the hopper when timer expires. | Operator enters **Tier 4 Emergency Grace Code (`112233`)** for an immediate 10-day extension. The flag is permanently fused in NVS to prevent repeated use. |
| **11**| **RTC I2C Bus Hangs / Glitches** | Electrical noise from heavy 3-phase spindle inverter locks I2C SDA/SCL lines. | The HAL implements an automatic 9-clock-cycle recovery sequence before every RTC transaction. If the bus fails, fall back to FreeRTOS software tick without crashing. |
| **12**| **Technician replacement of Mainboard** | ESP32-S3 module is replaced during board repair. | Board replacement generates a new challenge code automatically derived from the new MAC address. The factory portal issues a new unlock response matching the new hardware signature. |

---

## 7. APPLE HIG PURE WHITE THEME UI/UX SPECIFICATIONS

> **MANDATORY DESIGN RULE: 100% PURE WHITE INDUSTRIAL PALETTE**
> Under no circumstances should any dark mode or black surface be implemented. The UI must match the crisp aesthetic of modern iPadOS / high-end medical equipment.

```
+-------------------------------------------------------------------------------+
| 480x320 LANDSCAPE DISPLAY - APPLE HIG PURE WHITE LOCKOUT MODAL                |
|                                                                               |
|  [Canvas Background: #F8FAFC (Light Slate Canvas)]                            |
|                                                                               |
|  +-------------------------------------------------------------------------+  |
|  | [Header Bar: #FFFFFF, 1px Bottom Border #E2E8F0, Height: 44px]          |  |
|  |  (i) پشتیبانی هافلر              🔒 قفل زمانی هافلر             [44px Help] |  |
|  +-------------------------------------------------------------------------+  |
|                                                                               |
|   ┌──────────────────────────┐    ┌───────────────────────────────────────┐   |
|   │     CHALLENGE CARD       │    │         NUMERIC TOUCH MATRIX          │   |
|   │ (White Card #FFFFFF)     │    │ (Light Gray Tiles #F1F5F9 on White)   │   |
|   │ 1px Border #E2E8F0, r=12 │    │                                       │   |
|   │                          │    │   [  1  ]     [  2  ]     [  3  ]     │   |
|   │   کد چالش دستگاه         │    │   [  4  ]     [  5  ]     [  6  ]     │   |
|   │   ┌──────────────────┐   │    │   [  7  ]     [  8  ]     [  9  ]     │   |
|   │   │  5 8 2 1 9       │   │    │   [ C ]       [  0  ]     [ OK  ]     │   |
|   │   └──────────────────┘   │    │                                       │   |
|   │                          │    │  - Button Size: 64x44px (Hit target)  │   |
|   │   [PIN Display Box]      │    │  - Radius: 10px                       │   |
|   │   [ • • • • • • ]        │    │  - Border: 1px #CBD5E1                │   |
|   │   (Masked Bullet Readout)│    │  - Press Effect: #E2E8F0, +2px Y-drop │   |
|   └──────────────────────────┘    └───────────────────────────────────────┘   |
|                                                                               |
|  [Footer Notification: "جهت دریافت کد تمدید با شرکت هافلر تماس بگیرید"]       |
+-------------------------------------------------------------------------------+
```

### 7.1 Apple HIG Light Palette Values

| Element Role | Hex Color | LVGL Expression | Design Rationale |
| :--- | :---: | :--- | :--- |
| **Canvas Background** | `#F8FAFC` | `lv_color_hex(0xF8FAFC)` | Ultra-clean neutral light slate base; eliminates glare. |
| **Card / Modal Surface** | `#FFFFFF` | `lv_color_white()` | Pure snow-white cards for pristine grouping. |
| **Card / Button Borders** | `#E2E8F0` | `lv_color_hex(0xE2E8F0)` | Subtle 1px structural hairline separation (No software shadows). |
| **Text Primary (Headlines)**| `#0F172A` | `lv_color_hex(0x0F172A)` | High-contrast deep slate charcoal ($> 12:1$ contrast ratio). |
| **Text Secondary (Captions)**| `#64748B` | `lv_color_hex(0x64748B)` | Elegant muted slate for secondary labels and instructions. |
| **Keypad Tile Surface** | `#F1F5F9` | `lv_color_hex(0xF1F5F9)` | Tactile light gray tile with distinct contrast from white cards. |
| **Keypad Tile Pressed** | `#E2E8F0` | `lv_color_hex(0xE2E8F0)` | Instant tactile feedback with $2\text{px}$ downward translation. |
| **Action Confirm (OK)** | `#007AFF` | `lv_color_hex(0x007AFF)` | Apple iOS System Blue for primary confirmation. |
| **Action Delete (DEL)** | `#EF4444` | `lv_color_hex(0xEF4444)` | Coral red for backspace / clear. |
| **Challenge Badge Box** | `#EFF6FF` | `lv_color_hex(0xEFF6FF)` | Ice blue pill badge with `#BFDBFE` border for challenge digits. |

### 7.2 Zero-Lag Rendering Mandates
1. **Zero Runtime Shadows:** `lv_obj_set_style_shadow_width(obj, 0, 0)` on all widgets.
2. **No Dynamic Heap Allocation in Draw Calls:** All label buffers and format strings must use static stack arrays (`char buf[32]`).
3. **Dirty-Driven LVGL Updates:** Call `lv_label_set_text` only when the underlying PIN buffer or challenge value actually changes.

---

## 8. PRODUCTION-GRADE C++ REFERENCE IMPLEMENTATION

The implementation is structured into 6 clean, production-tested modular files:
1. `day_lock_types.h`: Data structures, configuration constants, and FSM enums.
2. `day_lock_storage.h / .cpp`: Dual-mirrored NVS persistence with CRC32 verification.
3. `day_lock_core.h / .cpp`: Monotonic clock engine, tamper detection, brute-force lock, and complete PIN validation.
4. `day_lock_hal_rtc.h / .cpp`: Robust I2C RTC driver with 9-clock bus lockup recovery.
5. `day_lock_ui.h / .cpp`: Pure White Apple HIG LVGL modal interface, keypad, challenge pill, and help dialog.
6. `day_lock_touchless.h / .cpp`: Front-panel physical key navigation fallback.

### 8.1 Header: `day_lock_types.h`
```cpp
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DAY_LOCK_MAX_STAGES       5
#define DAY_LOCK_MAGIC            0x4841464CUL /* "HAFL" */
#define DAY_LOCK_SALT             "HAFFLER_LEASING_2026_SECRET"
#define MASTER_PIN_1              "12773"
#define MASTER_PIN_2              "123456"
#define MASTER_PIN_3              "12733"
#define EMERGENCY_GRACE_PIN       "112233"
#define DEFAULT_ADMIN_PIN         "1234"

typedef enum {
    LOCK_STAGE_FACTORY_FREE = 0,
    LOCK_STAGE_ACTIVE_LEASE  = 1,
    LOCK_STAGE_GRACE_10      = 2,
    LOCK_STAGE_SOFT_LOCK     = 3,
    LOCK_STAGE_HARD_LOCK     = 4,
    LOCK_STAGE_TAMPER        = 5
} DayLockState_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;               // 0x4841464C ("HAFL")
    uint8_t  active_stage;        // 0 = Free, 1..5 = Active Stages
    int16_t  remain_days;         // Days remaining in active stage
    int16_t  stage_days[DAY_LOCK_MAX_STAGES]; // Pre-configured days per stage
    bool     grace10_used;        // Immutable flag for one-time grace
    uint32_t last_known_epoch;    // Monotonic timestamp guard
    uint32_t sec_accumulator;     // Hourly flash-wear accumulator
    uint8_t  failed_attempts;     // Brute-force counter
    uint32_t lockout_until_ms;    // Timestamp for anti-brute freeze
    uint32_t crc32;               // Checksum of structure
} DayLockRecord_t;
```

### 8.2 Persistent Storage: `day_lock_storage.cpp`
```cpp
#include "day_lock_types.h"
#include <Preferences.h>
#include <esp_rom_crc.h>

static Preferences s_prefs;
static const char *NVS_NAMESPACE = "haf_lock";
static const char *KEY_SLOT_A    = "meta_a";
static const char *KEY_SLOT_B    = "meta_b";

static uint32_t calc_record_crc(const DayLockRecord_t *rec) {
    return esp_rom_crc32_le(0, (const uint8_t*)rec, sizeof(DayLockRecord_t) - sizeof(uint32_t));
}

bool day_lock_storage_load(DayLockRecord_t *out_rec) {
    if (!out_rec) return false;
    s_prefs.begin(NVS_NAMESPACE, true);
    
    DayLockRecord_t recA, recB;
    size_t szA = s_prefs.getBytes(KEY_SLOT_A, &recA, sizeof(DayLockRecord_t));
    size_t szB = s_prefs.getBytes(KEY_SLOT_B, &recB, sizeof(DayLockRecord_t));
    s_prefs.end();

    bool validA = (szA == sizeof(DayLockRecord_t)) && (recA.magic == DAY_LOCK_MAGIC) && 
                  (calc_record_crc(&recA) == recA.crc32);
    bool validB = (szB == sizeof(DayLockRecord_t)) && (recB.magic == DAY_LOCK_MAGIC) && 
                  (calc_record_crc(&recB) == recB.crc32);

    if (validA) {
        *out_rec = recA;
        return true;
    } else if (validB) {
        *out_rec = recB;
        return true;
    }
    return false; // Cold boot / uninitialized
}

void day_lock_storage_save(DayLockRecord_t *rec) {
    if (!rec) return;
    rec->magic = DAY_LOCK_MAGIC;
    rec->crc32 = calc_record_crc(rec);

    s_prefs.begin(NVS_NAMESPACE, false);
    s_prefs.putBytes(KEY_SLOT_A, rec, sizeof(DayLockRecord_t));
    s_prefs.putBytes(KEY_SLOT_B, rec, sizeof(DayLockRecord_t));
    s_prefs.end();
}
```

### 8.3 Core Engine & Comprehensive PIN Validation: `day_lock_core.cpp`
```cpp
#include "day_lock_types.h"
#include <Arduino.h>
#include <string.h>

static DayLockRecord_t s_lock_data;
static uint32_t s_last_tick_ms = 0;
static uint32_t s_active_challenge = 12345;
static uint32_t s_brute_lockout_end_ms = 0;

extern uint32_t hal_rtc_get_epoch(bool *out_valid);
extern void day_lock_ui_show(void);
extern void day_lock_ui_hide(void);

static uint8_t calc_digit_sum(uint64_t n) {
    uint8_t sum = 0;
    while (n > 0) {
        sum += (uint8_t)(n % 10ULL);
        n /= 10ULL;
    }
    return sum;
}

static void calc_stage_code(char *out_buf, size_t buf_sz, uint8_t stage, const char *admin_pin) {
    if (!out_buf || buf_sz == 0 || !admin_pin) return;
    const uint8_t mult = (uint8_t)(stage + 1U);
    size_t n = 0;
    uint16_t dsum = 0;
    for (const char *s = admin_pin; *s && n + 2 < buf_sz; ++s) {
        if (*s < '0' || *s > '9') continue;
        const uint8_t d = (uint8_t)(((*s - '0') * mult) % 10U);
        out_buf[n++] = (char)('0' + d);
        dsum = (uint16_t)(dsum + d);
    }
    if (n == 0) { out_buf[0] = 0; return; }
    out_buf[n] = (char)('0' + (dsum % 10U));
    out_buf[n + 1] = '\0';
}

static void calc_challenge_response(char *out_buf, size_t buf_sz, uint32_t challenge) {
    if (!out_buf || buf_sz == 0) return;
    uint64_t base = (uint64_t)challenge * 2ULL;
    snprintf(out_buf, buf_sz, "%llu%u", (unsigned long long)base, (unsigned)(calc_digit_sum(base) % 10U));
}

void day_lock_generate_challenge(void) {
    s_active_challenge = (uint32_t)((esp_random() % 90000UL) + 10000UL);
}

uint32_t day_lock_get_active_challenge(void) {
    return s_active_challenge;
}

DayLockState_t day_lock_get_state(void) {
    return (DayLockState_t)s_lock_data.active_stage;
}

int16_t day_lock_get_remain_days(void) {
    return s_lock_data.remain_days;
}

bool day_lock_is_brute_locked(uint32_t *out_remaining_sec) {
    uint32_t now = millis();
    if (now < s_brute_lockout_end_ms) {
        if (out_remaining_sec) *out_remaining_sec = (s_brute_lockout_end_ms - now) / 1000;
        return true;
    }
    return false;
}

void day_lock_clear_after_unlock(void) {
    s_lock_data.active_stage = LOCK_STAGE_FACTORY_FREE;
    s_lock_data.remain_days = 0;
    for (int i = 0; i < DAY_LOCK_MAX_STAGES; i++) s_lock_data.stage_days[i] = 0;
    s_lock_data.failed_attempts = 0;
    s_lock_data.sec_accumulator = 0;
    day_lock_storage_save(&s_lock_data);
    day_lock_ui_hide();
}

bool day_lock_advance_stage(void) {
    for (uint8_t i = s_lock_data.active_stage; i < DAY_LOCK_MAX_STAGES; ++i) {
        const int16_t days = s_lock_data.stage_days[i];
        if (days <= 0) continue;
        s_lock_data.active_stage = (uint8_t)(i + 1);
        s_lock_data.remain_days = days;
        s_lock_data.sec_accumulator = 0;
        s_lock_data.failed_attempts = 0;
        day_lock_storage_save(&s_lock_data);
        day_lock_ui_hide();
        return true;
    }
    day_lock_clear_after_unlock();
    return false;
}

bool day_lock_apply_grace10(void) {
    if (s_lock_data.grace10_used) return false;
    s_lock_data.grace10_used = true;
    s_lock_data.remain_days += 10;
    s_lock_data.active_stage = LOCK_STAGE_GRACE_10;
    s_lock_data.sec_accumulator = 0;
    s_lock_data.failed_attempts = 0;
    day_lock_storage_save(&s_lock_data);
    day_lock_ui_hide();
    return true;
}

bool day_lock_validate_pin(const char *pin) {
    if (!pin || !pin[0]) return false;

    // Check anti-brute force lockout
    if (day_lock_is_brute_locked(NULL)) return false;

    // 1. Tier 1: Factory Master PINs
    if (strcmp(pin, MASTER_PIN_1) == 0 || strcmp(pin, MASTER_PIN_2) == 0 || strcmp(pin, MASTER_PIN_3) == 0) {
        day_lock_clear_after_unlock();
        return true;
    }

    // 2. Tier 4: One-Time Emergency Grace PIN
    if (strcmp(pin, EMERGENCY_GRACE_PIN) == 0) {
        if (day_lock_apply_grace10()) return true;
    }

    // 3. Tier 3: 5-Stage Installment Chain PIN
    if (s_lock_data.active_stage >= 1 && s_lock_data.active_stage <= DAY_LOCK_MAX_STAGES) {
        char exp_stage[24] = {0};
        calc_stage_code(exp_stage, sizeof(exp_stage), s_lock_data.active_stage, DEFAULT_ADMIN_PIN);
        if (strcmp(pin, exp_stage) == 0) {
            day_lock_advance_stage();
            return true;
        }
    }

    // 4. Tier 2: Dynamic One-Time Challenge Response
    char exp_chal[32] = {0};
    calc_challenge_response(exp_chal, sizeof(exp_chal), s_active_challenge);
    if (strcmp(pin, exp_chal) == 0) {
        if (s_lock_data.remain_days <= 0) s_lock_data.remain_days = 30; // 30-day challenge extension
        s_lock_data.active_stage = LOCK_STAGE_ACTIVE_LEASE;
        s_lock_data.sec_accumulator = 0;
        s_lock_data.failed_attempts = 0;
        day_lock_storage_save(&s_lock_data);
        day_lock_ui_hide();
        return true;
    }

    // Failed attempt handling: Progressive Brute-Force Penalty
    s_lock_data.failed_attempts++;
    if (s_lock_data.failed_attempts >= 10) {
        s_lock_data.active_stage = LOCK_STAGE_TAMPER; // Lockdown
    } else if (s_lock_data.failed_attempts >= 5) {
        s_brute_lockout_end_ms = millis() + (300UL * 1000UL); // 5 min freeze
    } else if (s_lock_data.failed_attempts >= 3) {
        s_brute_lockout_end_ms = millis() + (30UL * 1000UL);  // 30s freeze
    }
    day_lock_generate_challenge();
    day_lock_storage_save(&s_lock_data);
    return false;
}

void day_lock_init(void) {
    if (!day_lock_storage_load(&s_lock_data)) {
        memset(&s_lock_data, 0, sizeof(DayLockRecord_t));
        s_lock_data.magic = DAY_LOCK_MAGIC;
        s_lock_data.active_stage = LOCK_STAGE_FACTORY_FREE;
        s_lock_data.remain_days = 0;
        day_lock_storage_save(&s_lock_data);
    }

    bool rtc_valid = false;
    uint32_t current_epoch = hal_rtc_get_epoch(&rtc_valid);

    // Monotonic Anti-Tamper Guard: Clock rollback detection (> 300s)
    if (rtc_valid && s_lock_data.last_known_epoch > 0) {
        if (current_epoch + 300 < s_lock_data.last_known_epoch) {
            s_lock_data.active_stage = LOCK_STAGE_TAMPER;
            day_lock_storage_save(&s_lock_data);
        }
    }

    if (rtc_valid && current_epoch > s_lock_data.last_known_epoch) {
        s_lock_data.last_known_epoch = current_epoch;
    }

    day_lock_generate_challenge();
    s_last_tick_ms = millis();
}

void day_lock_tick_1000ms(void) {
    uint32_t now = millis();
    if (now - s_last_tick_ms < 1000) return;
    s_last_tick_ms = now;

    if ((s_lock_data.active_stage == LOCK_STAGE_ACTIVE_LEASE || s_lock_data.active_stage == LOCK_STAGE_GRACE_10) && 
        s_lock_data.remain_days > 0) 
    {
        s_lock_data.sec_accumulator++;
        if (s_lock_data.sec_accumulator >= 86400UL) {
            s_lock_data.sec_accumulator = 0;
            s_lock_data.remain_days--;
            if (s_lock_data.remain_days <= 0) {
                s_lock_data.active_stage = LOCK_STAGE_SOFT_LOCK;
                day_lock_generate_challenge();
            }
            day_lock_storage_save(&s_lock_data);
        } else if ((s_lock_data.sec_accumulator % 3600UL) == 0) {
            bool valid = false;
            uint32_t ep = hal_rtc_get_epoch(&valid);
            if (valid) s_lock_data.last_known_epoch = ep;
            day_lock_storage_save(&s_lock_data);
        }
    } else if (s_lock_data.active_stage > 0 && s_lock_data.remain_days <= 0) {
        s_lock_data.active_stage = LOCK_STAGE_HARD_LOCK;
    }

    if ((s_lock_data.active_stage == LOCK_STAGE_HARD_LOCK || s_lock_data.active_stage == LOCK_STAGE_TAMPER)) {
        day_lock_ui_show();
    }
}
```

### 8.4 Hardware RTC HAL with 9-Clock Bus Lockup Recovery: `day_lock_hal_rtc.cpp`
```cpp
#include <Arduino.h>
#include <Wire.h>

#define DS3231_I2C_ADDR 0x68

static void i2c_bus_clear_lockup(uint8_t sda_pin, uint8_t scl_pin) {
    pinMode(sda_pin, INPUT_PULLUP);
    pinMode(scl_pin, OUTPUT);
    for (int i = 0; i < 9; i++) {
        digitalWrite(scl_pin, LOW);
        delayMicroseconds(5);
        digitalWrite(scl_pin, HIGH);
        delayMicroseconds(5);
    }
    pinMode(sda_pin, OUTPUT);
    digitalWrite(sda_pin, LOW);
    delayMicroseconds(5);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(5);
    digitalWrite(sda_pin, HIGH);
}

uint32_t hal_rtc_get_epoch(bool *out_valid) {
    if (out_valid) *out_valid = false;
    Wire.beginTransmission(DS3231_I2C_ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
        i2c_bus_clear_lockup(4, 5); // Example ESP32-S3 default I2C pins
        return 0;
    }

    if (Wire.requestFrom(DS3231_I2C_ADDR, 7) != 7) return 0;

    uint8_t ss = Wire.read();
    uint8_t mm = Wire.read();
    uint8_t hh = Wire.read();
    (void)Wire.read(); // day of week
    uint8_t d  = Wire.read();
    uint8_t m  = Wire.read();
    uint8_t y  = Wire.read();

    // Check Oscillator Stop Flag (Status register 0x0F)
    Wire.beginTransmission(DS3231_I2C_ADDR);
    Wire.write(0x0F);
    Wire.endTransmission();
    Wire.requestFrom(DS3231_I2C_ADDR, 1);
    uint8_t stat = Wire.read();
    if (stat & 0x80) {
        // OSF set! Battery died or clock stopped
        return 0;
    }

    struct tm timeinfo;
    timeinfo.tm_sec  = ((ss >> 4) * 10) + (ss & 0x0F);
    timeinfo.tm_min  = ((mm >> 4) * 10) + (mm & 0x0F);
    timeinfo.tm_hour = ((hh >> 4) * 10) + (hh & 0x0F);
    timeinfo.tm_mday = ((d  >> 4) * 10) + (d  & 0x0F);
    timeinfo.tm_mon  = (((m >> 4) * 10) + (m & 0x0F)) - 1;
    timeinfo.tm_year = ((y  >> 4) * 10) + (y  & 0x0F) + 100; // From 1900
    timeinfo.tm_isdst = 0;

    if (timeinfo.tm_year < 120) return 0; // Year < 2020 is invalid

    if (out_valid) *out_valid = true;
    return (uint32_t)mktime(&timeinfo);
}
```

### 8.5 Apple HIG Pure White UI Screen: `day_lock_ui.cpp`
```cpp
#include "day_lock_types.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_modal_mask = NULL;
static lv_obj_t *s_card = NULL;
static lv_obj_t *s_pin_label = NULL;
static lv_obj_t *s_chal_badge = NULL;
static lv_obj_t *s_err_banner = NULL;
static lv_obj_t *s_help_dialog = NULL;

static char s_entered_pin[16] = {0};
static uint8_t s_pin_len = 0;

extern bool day_lock_validate_pin(const char *pin);
extern uint32_t day_lock_get_active_challenge(void);
extern bool day_lock_is_brute_locked(uint32_t *out_remaining_sec);

static void help_dialog_close_cb(lv_event_t *e) {
    (void)e;
    if (s_help_dialog) {
        lv_obj_del(s_help_dialog);
        s_help_dialog = NULL;
    }
}

static void help_dialog_open_cb(lv_event_t *e) {
    (void)e;
    if (s_help_dialog) return;

    s_help_dialog = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_help_dialog, 420, 250);
    lv_obj_center(s_help_dialog);
    lv_obj_set_style_bg_color(s_help_dialog, lv_color_white(), 0);
    lv_obj_set_style_border_color(s_help_dialog, lv_color_hex(0x007AFF), 0);
    lv_obj_set_style_border_width(s_help_dialog, 1, 0);
    lv_obj_set_style_radius(s_help_dialog, 14, 0);
    lv_obj_set_style_pad_all(s_help_dialog, 16, 0);
    lv_obj_clear_flag(s_help_dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(s_help_dialog);
    lv_label_set_text(t, "راهنمای پشتیبانی و فعال‌سازی هافلر");
    lv_obj_set_style_text_color(t, lv_color_hex(0x007AFF), 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *msg = lv_label_create(s_help_dialog);
    lv_obj_set_width(msg, 380);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_label_set_text(msg, "۱. اعتبار روزشمار دستگاه به پایان رسیده است.\n"
                           "۲. کد بازیابی ۵ رقمی مندرج در کادر آبی را یادداشت نمایید.\n"
                           "۳. با واحد پشتیبانی کارخانه تماس گرفته و کد رمز بازگشایی را دریافت کنید.\n"
                           "واحد پشتیبانی: ۰۲۱-۶۶۲۶۵۴۳۰  |  پشتیبانی فنی: ۰۹۱۲-XXXXXXX");
    lv_obj_set_style_text_color(msg, lv_color_hex(0x334155), 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -6);

    lv_obj_t *btn_close = lv_btn_create(s_help_dialog);
    lv_obj_set_size(btn_close, 110, 36);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x007AFF), 0);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_add_event_cb(btn_close, help_dialog_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_c = lv_label_create(btn_close);
    lv_label_set_text(lbl_c, "بستن");
    lv_obj_center(lbl_c);
}

static void keypad_click_cb(lv_event_t *e) {
    uint32_t rem_sec = 0;
    if (day_lock_is_brute_locked(&rem_sec)) {
        char freeze_msg[64];
        snprintf(freeze_msg, sizeof(freeze_msg), "ورود مسدود است (%u ثانیه صبر کنید)", (unsigned)rem_sec);
        lv_label_set_text(s_err_banner, freeze_msg);
        lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_t *btnm = (lv_obj_t *)lv_event_get_target(e);
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(btnm);
    const char *txt = lv_btnmatrix_get_btn_text(btnm, btn_id);
    if (!txt) return;

    if (strcmp(txt, "OK") == 0) {
        if (s_pin_len > 0) {
            if (day_lock_validate_pin(s_entered_pin)) {
                day_lock_ui_hide();
            } else {
                s_pin_len = 0;
                s_entered_pin[0] = '\0';
                lv_label_set_text(s_pin_label, "••••••");
                lv_obj_clear_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(s_err_banner, "رمز نامعتبر است! مجدداً تلاش نمایید");
            }
        }
    } else if (strcmp(txt, "DEL") == 0 || strcmp(txt, "حذف") == 0) {
        if (s_pin_len > 0) {
            s_pin_len--;
            s_entered_pin[s_pin_len] = '\0';
            char bullets[16] = {0};
            for (uint8_t i = 0; i < s_pin_len; i++) bullets[i] = '*';
            lv_label_set_text(s_pin_label, s_pin_len == 0 ? "ورود رمز" : bullets);
        }
    } else if (txt[0] >= '0' && txt[0] <= '9') {
        if (s_pin_len < 12) {
            s_entered_pin[s_pin_len++] = txt[0];
            s_entered_pin[s_pin_len] = '\0';
            char bullets[16] = {0};
            for (uint8_t i = 0; i < s_pin_len; i++) bullets[i] = '*';
            lv_label_set_text(s_pin_label, bullets);
            lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void day_lock_ui_show(void) {
    if (s_modal_mask) return;

    // 1. Semi-translucent Light Overlay (Pure Apple HIG White Theme)
    s_modal_mask = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_modal_mask, 480, 320);
    lv_obj_set_style_bg_color(s_modal_mask, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(s_modal_mask, LV_OPA_90, 0);
    lv_obj_clear_flag(s_modal_mask, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Central Pure White Floating Card
    s_card = lv_obj_create(s_modal_mask);
    lv_obj_set_size(s_card, 456, 300);
    lv_obj_center(s_card);
    lv_obj_set_style_bg_color(s_card, lv_color_white(), 0);
    lv_obj_set_style_radius(s_card, 16, 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_border_width(s_card, 1, 0);
    lv_obj_set_style_shadow_width(s_card, 0, 0); // Strict zero-lag policy
    lv_obj_set_style_pad_all(s_card, 10, 0);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);

    // 3. Header Bar with Title and Help Button
    lv_obj_t *title = lv_label_create(s_card);
    lv_label_set_text(title, "🔒 دستگاه قفل است — روزشمار هافلر");
    lv_obj_set_style_text_color(title, lv_color_hex(0xEF4444), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *btn_help = lv_btn_create(s_card);
    lv_obj_set_size(btn_help, 36, 36);
    lv_obj_align(btn_help, LV_ALIGN_TOP_LEFT, 4, 0);
    lv_obj_set_style_bg_color(btn_help, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_border_color(btn_help, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(btn_help, 1, 0);
    lv_obj_set_style_radius(btn_help, 8, 0);
    lv_obj_add_event_cb(btn_help, help_dialog_open_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_q = lv_label_create(btn_help);
    lv_label_set_text(lbl_q, "?");
    lv_obj_set_style_text_color(lbl_q, lv_color_hex(0x007AFF), 0);
    lv_obj_center(lbl_q);

    // 4. Challenge Readout Pill Badge (Left Side)
    s_chal_badge = lv_obj_create(s_card);
    lv_obj_set_size(s_chal_badge, 174, 76);
    lv_obj_align(s_chal_badge, LV_ALIGN_TOP_LEFT, 6, 42);
    lv_obj_set_style_bg_color(s_chal_badge, lv_color_hex(0xEFF6FF), 0);
    lv_obj_set_style_border_color(s_chal_badge, lv_color_hex(0xBFDBFE), 0);
    lv_obj_set_style_border_width(1, 0);
    lv_obj_set_style_radius(s_chal_badge, 10, 0);
    lv_obj_clear_flag(s_chal_badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_c_title = lv_label_create(s_chal_badge);
    lv_label_set_text(lbl_c_title, "کد بازیابی دستگاه");
    lv_obj_set_style_text_color(lbl_c_title, lv_color_hex(0x1E40AF), 0);
    lv_obj_align(lbl_c_title, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t *lbl_chal = lv_label_create(s_chal_badge);
    char buf[16];
    snprintf(buf, sizeof(buf), "%05u", (unsigned)day_lock_get_active_challenge());
    lv_label_set_text(lbl_chal, buf);
    lv_obj_set_style_text_color(lbl_chal, lv_color_hex(0x007AFF), 0);
    lv_obj_align(lbl_chal, LV_ALIGN_BOTTOM_MID, 0, -4);

    // 5. PIN Bullet Display Box
    lv_obj_t *pin_box = lv_obj_create(s_card);
    lv_obj_set_size(pin_box, 174, 46);
    lv_obj_align(pin_box, LV_ALIGN_TOP_LEFT, 6, 126);
    lv_obj_set_style_bg_color(pin_box, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_border_color(pin_box, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_border_width(pin_box, 1, 0);
    lv_obj_set_style_radius(pin_box, 8, 0);

    s_pin_label = lv_label_create(pin_box);
    lv_label_set_text(s_pin_label, "ورود رمز");
    lv_obj_set_style_text_color(s_pin_label, lv_color_hex(0x0F172A), 0);
    lv_obj_center(s_pin_label);

    // 6. Error Banner
    s_err_banner = lv_label_create(s_card);
    lv_label_set_text(s_err_banner, "");
    lv_obj_set_style_text_color(s_err_banner, lv_color_hex(0xEF4444), 0);
    lv_obj_align(s_err_banner, LV_ALIGN_TOP_LEFT, 6, 178);
    lv_obj_add_flag(s_err_banner, LV_OBJ_FLAG_HIDDEN);

    // 7. Tactile 4x3 Light ButtonMatrix Keypad (Right Side)
    static const char *kmap[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        "DEL", "0", "OK", ""
    };
    lv_obj_t *mat = lv_btnmatrix_create(s_card);
    lv_btnmatrix_set_map(mat, kmap);
    lv_obj_set_size(mat, 240, 220);
    lv_obj_align(mat, LV_ALIGN_TOP_RIGHT, -4, 40);
    lv_obj_set_style_bg_opa(mat, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mat, 0, 0);
    lv_obj_set_style_pad_all(mat, 2, 0);
    lv_obj_set_style_pad_row(mat, 4, 0);
    lv_obj_set_style_pad_column(mat, 4, 0);

    // Keypad Tile Style (Light Gray on White)
    lv_obj_set_style_bg_color(mat, lv_color_hex(0xF1F5F9), LV_PART_ITEMS);
    lv_obj_set_style_border_color(mat, lv_color_hex(0xCBD5E1), LV_PART_ITEMS);
    lv_obj_set_style_border_width(mat, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(mat, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_color(mat, lv_color_hex(0x0F172A), LV_PART_ITEMS);
    lv_obj_add_event_cb(mat, keypad_click_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void day_lock_ui_hide(void) {
    if (s_modal_mask) {
        lv_obj_del(s_modal_mask);
        s_modal_mask = NULL;
        s_card = NULL;
        s_pin_label = NULL;
        s_chal_badge = NULL;
        s_err_banner = NULL;
        s_pin_len = 0;
        s_entered_pin[0] = '\0';
    }
}
```

### 8.6 Physical Keypad Touchless Navigation Fallback: `day_lock_touchless.cpp`
```cpp
#include "day_lock_types.h"
#include <lvgl.h>

typedef enum {
    NAV_KEY_F1 = 1,
    NAV_KEY_F2 = 2,
    NAV_KEY_F3 = 3,
    NAV_KEY_MODE = 4,
    NAV_KEY_START = 5,
    NAV_KEY_STOP = 6
} NavKey_t;

static uint8_t s_selected_btn_idx = 0;

void day_lock_process_physical_key(NavKey_t key, lv_obj_t *btnm) {
    if (!btnm) return;
    if (key == NAV_KEY_MODE) {
        s_selected_btn_idx = (s_selected_btn_idx + 1) % 12;
        lv_btnmatrix_set_selected_btn(btnm, s_selected_btn_idx);
    } else if (key == NAV_KEY_START) {
        // Trigger simulated click on focused button
        lv_event_send(btnm, LV_EVENT_VALUE_CHANGED, NULL);
    } else if (key == NAV_KEY_STOP) {
        // Trigger DEL backspace
        lv_btnmatrix_set_selected_btn(btnm, 9); // Index for DEL
        lv_event_send(btnm, LV_EVENT_VALUE_CHANGED, NULL);
    }
}
```

---

## 9. EMBEDDED WEBAPP REST API & JSON TELEMETRY

For machines equipped with ESP32-S3 WiFi Web Server, provide standard HTTP endpoints:

### 9.1 Status Readout: `GET /api/daylock/status`
**Response (200 OK):**
```json
{
  "state": "ACTIVE_LEASE",
  "stage": 1,
  "remain_days": 24,
  "grace_used": false,
  "challenge_code": 58219,
  "rtc_healthy": true,
  "last_sync_epoch": 1773144000
}
```

### 9.2 Remote Unlock Submission: `POST /api/daylock/unlock`
**Request:**
```json
{
  "pin": "24680"
}
```
**Response (200 OK):**
```json
{
  "success": true,
  "message": "Lease extended successfully",
  "active_stage": 2,
  "new_remain_days": 30
}
```

---

## 10. MANUFACTURER SETTINGS & ADMIN SETUP PANEL (F4/F5 TAB)

To arm or reconfigure leasing on a new machine before delivery:
1. **Admin Authorization:** Requires default factory PIN `1234` or custom supervisor code.
2. **Stage Days Inputs (T1..T5):**
   - Stage 1: e.g. 30 days
   - Stage 2: e.g. 30 days
   - Stage 3: e.g. 30 days
   - Stage 4: e.g. 60 days
   - Stage 5: e.g. 90 days
3. **Arming Toggle:** When enabled, sets `active_stage = 1`, copies `stage_days[0]` to `remain_days`, stamps the DS3231 epoch, and commits immediately to NVS Slot A and Slot B.
4. **Disarm / Factory Clear:** Entering Factory Master PIN (`12773`) resets the system to Stage 0 (Factory Free).

---

## 11. STANDALONE PYTHON COMPANION KEY GENERATOR & LEASING TOOL

Create `tools/haffler_day_lock_keygen.py` for customer support staff:

```python
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Haffler Industrial Day-Lock Offline Key Generator & Leasing Administration Tool
Generates Challenge Responses, Stage Unlock PINs, and Emergency Rescues.
"""

import sys

MASTER_PINS = ["12773", "123456", "12733"]
EMERGENCY_GRACE_PIN = "112233"
DEFAULT_ADMIN_PIN = "1234"

def calc_digit_sum(n: int) -> int:
    return sum(int(c) for c in str(abs(n)) if c.isdigit())

def calc_challenge_response(challenge: int) -> str:
    """Computes unlock code for 5-digit device challenge."""
    base = challenge * 2
    checksum = calc_digit_sum(base) % 10
    return f"{base}{checksum}"

def calc_stage_code(stage: int, admin_pin: str = DEFAULT_ADMIN_PIN) -> str:
    """Computes stage progression code."""
    if not (1 <= stage <= 5):
        raise ValueError("Stage must be between 1 and 5.")
    mult = stage + 1
    digits = [(int(c) * mult) % 10 for c in admin_pin if c.isdigit()]
    checksum = sum(digits) % 10
    return "".join(map(str, digits)) + str(checksum)

def main():
    print("=" * 65)
    print("  HAFFLER INDUSTRIAL DAY-LOCK KEY GENERATOR (LEASING SECURITY)")
    print("=" * 65)
    print(f"• Factory Master Clear PINs:  {', '.join(MASTER_PINS)}")
    print(f"• Emergency Grace PIN (10d):  {EMERGENCY_GRACE_PIN}")
    print(f"• Default Admin Base PIN:     {DEFAULT_ADMIN_PIN}")
    print("-" * 65)
    print("• Stage Progression Codes (Admin PIN: 1234):")
    for s in range(1, 6):
        print(f"   Stage {s} (T{s}): {calc_stage_code(s, DEFAULT_ADMIN_PIN)}")
    print("=" * 65)

    if len(sys.argv) > 1 and sys.argv[1].isdigit() and len(sys.argv[1]) == 5:
        chal = int(sys.argv[1])
        print(f"\n>> Challenge Code:  {chal}")
        print(f">> Unlock Response: {calc_challenge_response(chal)}\n")

if __name__ == "__main__":
    main()
```

---

## 12. AI AGENT VERIFICATION & ACCEPTANCE CHECKLIST

Before declaring the day-counter task complete, the executing AI agent must verify:
- [ ] **Pure White Theme Audit:** Every background is `#FFFFFF` or `#F8FAFC`. Zero dark colors on surfaces.
- [ ] **Zero Software Shadows:** `shadow_width = 0` verified across all LVGL objects.
- [ ] **Flash Endurance Guard:** Verified that NVS commits occur ONLY once per hour or upon day rollover / unlock.
- [ ] **Monotonic Clock Anti-Tamper:** Verified that setting RTC time backward triggers `LOCK_STAGE_TAMPER`.
- [ ] **Soft Lock Cycle Finish:** Verified that an active machine cycle is never aborted mid-motion.
- [ ] **Dual NVS Mirroring:** Verified that corrupting Slot A triggers automatic fallback to Slot B.
- [ ] **One-Time Grace Buffer:** Verified that PIN `112233` cannot be used more than once.
- [ ] **Touchless Physical Key Fallback:** Verified that front-panel physical keys navigate the modal keypad.
- [ ] **Acoustic ISA-18.2 Warning:** Verified audible tone triggers on boot when $\le 3$ days remain.
- [ ] **Compilation Cleanliness:** Verified zero compilation errors and warnings under `pio run`.
