# 24. Shift Statistics, Material Accounting & Audio Melodic Synthesizer
## Multi-Shift Production Tracking, Thermodynamic / Flow Math, and Non-Blocking LEDC PWM Audio

> **Document ID:** `24_SHIFT_STATISTICS_MATERIAL_ACCOUNTING_AND_AUDIO.md`  
> **Target Platform:** Haffler Universal Integrated PLC + HMI Controller (ESP32-S3 N16R8).  
> **Standard:** Industrial Shift Accounting, DS3231 Monotonic Time Tracking, ISA Sound Ergonomics.  
> **Audience:** Autonomous AI Agents and Senior Industrial Systems Engineers.

---

## Executive Summary
In commercial and industrial production (bakeries, extrusion cutting, profile welding, gas filling), machine operators and factory managers require:
1. **Automated Multi-Shift Accounting:** Automatic segmentation of production counts and operating hours across factory working shifts without manual resetting.
2. **Material Consumption Physics Math:** Live estimation of raw material usage (flour weight in kilograms, inert gas volume in liters, profile lengths in meters) based on portion/cycle count and calibrated process parameters.
3. **Ergonomic Audio Feedback:** Distinct acoustic signatures generated via hardware LEDC PWM timers over a non-blocking FreeRTOS queue, providing audible cues for button presses, cycle starts, completions, and ISA-18.2 fault conditions without CPU delays.

---

## 1. Automated 3-Shift Factory Management (DS3231 RTC)

### 1.1. Industrial Shift Schedule Matrix
Haffler controllers segment 24-hour operation into 3 standardized production shifts:

| Shift Identifier | Shift Name | Standard Hours | Typical Crew / Function |
| :---: | :--- | :---: | :--- |
| **SHIFT_MORNING** | Morning Shift (صبح) | `05:00` – `13:00` | High-volume continuous production |
| **SHIFT_AFTERNOON** | Afternoon Shift (عصر) | `13:00` – `21:00` | Secondary production & batch processing |
| **SHIFT_NIGHT** | Night Shift (شب) | `21:00` – `05:00` | Heavy baking, CIP cleaning & maintenance |

### 1.2. Shift Rollover Logic & NVS Preservation
A background task on Core 0 inspects the DS3231 hardware RTC every 10 seconds. When the clock transitions across a shift boundary:
1. Current shift counters are archived into NVS structure array `shift_records[3]`.
2. Current shift cycle count is reset to 0.
3. Lifetime machine odometer (`odometer_total_cycles`) continues incrementing monotonically.
4. Total flour or material consumption for the shift is finalized and stored.

```cpp
#include "haffler_pins.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SHIFT_MORNING = 0,
    SHIFT_AFTERNOON = 1,
    SHIFT_NIGHT = 2,
    SHIFT_COUNT = 3
} shift_id_t;

typedef struct {
    uint32_t total_cycles;
    uint32_t good_cycles;
    uint32_t rejected_cycles;
    float    material_consumed_kg;
    uint32_t operating_seconds;
    uint32_t start_epoch;
} shift_record_t;

static shift_record_t s_active_shift_record;
static shift_id_t     s_current_shift = SHIFT_MORNING;

shift_id_t shift_calculate_active(uint8_t hour) {
    if (hour >= 5 && hour < 13) {
        return SHIFT_MORNING;
    } else if (hour >= 13 && hour < 21) {
        return SHIFT_AFTERNOON;
    } else {
        return SHIFT_NIGHT;
    }
}

void shift_manager_update(uint8_t hour, uint8_t minute, uint32_t epoch) {
    shift_id_t calculated = shift_calculate_active(hour);
    
    // Detect Shift Boundary Transition
    if (calculated != s_current_shift) {
        // 1. Archive previous shift metrics to NVS
        // nvs_save_shift_record(s_current_shift, &s_active_shift_record);
        
        // 2. Initialize new shift record
        s_current_shift = calculated;
        s_active_shift_record.total_cycles = 0;
        s_active_shift_record.good_cycles = 0;
        s_active_shift_record.rejected_cycles = 0;
        s_active_shift_record.material_consumed_kg = 0.0f;
        s_active_shift_record.operating_seconds = 0;
        s_active_shift_record.start_epoch = epoch;
    }
}
```

---

## 2. Material Consumption Formulas & Physics Engines

### 2.1. Bakery Dough Divider (Flour Consumption)
In dough portioning machines, the operator configures target dough piece weight $W$ (grams). Since standard bakery dough consists of flour, water, yeast, and salt with an average hydration ratio of $62\%$, the mass of dry flour consumed per dough piece is:

$$\text{Flour Mass (kg)} = \frac{\text{Portion Count} \times \text{Piece Weight (g)}}{1000 \times 1.62}$$

```cpp
float bakery_calculate_flour_consumed_kg(uint32_t portion_count, float piece_weight_grams) {
    if (piece_weight_grams <= 0.0f) return 0.0f;
    return (float)portion_count * piece_weight_grams / (1000.0f * 1.62f);
}
```

### 2.2. Gas Filling Spacer Injection (Argon Volume)
For double-glazing window argon injection, gas volume per window unit is computed from glass dimensions ($W \times H \times T_{\text{spacer}}$) and flow sensor integration:

$$\text{Volume (Liters)} = \frac{W (\text{mm}) \times H (\text{mm}) \times T (\text{mm})}{1\,000\,000} \times \text{Overfill Factor } (1.15)$$

### 2.3. Profile Cutting Saw (Material Linear Meters)
For UPVC and aluminum saws, cut profile length $L$ (mm) is accumulated per piece:

$$\text{Total Extrusion Cut (Meters)} = \sum_{i=1}^{N} \frac{L_i + K_{\text{blade}}}{1000}$$
*(Where $K_{\text{blade}}$ is the saw blade kerf width, typically $4.2\text{ mm}$).*

---

## 3. LEDC Hardware PWM Audio Melodic Synthesizer

### 3.1. Acoustic Hardware Configuration
The onboard piezo buzzer is connected to `PIN_BUZZER` (`GPIO 45`). Rather than blocking the CPU with crude `delay()` tones, the audio engine runs on the ESP32-S3 hardware **LEDC Timer**:
- **LEDC Low-Speed Channel:** `LEDC_CHANNEL_0`
- **LEDC Timer Resolution:** `LEDC_TIMER_10_BIT` (1024 steps)
- **Duty Cycle:** $50\%$ when active ($512$), $0\%$ when silent.

### 3.2. Standard Melodic Sound Signatures (ISA Ergonomics)
The system synthesizes 6 discrete multi-tone signatures:

| Sound Identifier | Acoustic Profile | Musical Notes (Hz) | Duration (ms) | Operator Meaning |
| :--- | :--- | :--- | :---: | :--- |
| `SOUND_KEY_PRESS` | Subtle high click | $2400\text{ Hz}$ | $20\text{ ms}$ | Tactile key feedback |
| `SOUND_START_CYCLE`| Ascending major arpeggio | $1046 \rightarrow 1318 \rightarrow 1568$ (C6-E6-G6) | $60\text{ ms each}$ | Automatic cycle starting |
| `SOUND_PORTION_DONE`| Pleasant double chime | $2093 \rightarrow 2637$ (C7-E7) | $40\text{ ms each}$ | Piece completed |
| `SOUND_STOP_CYCLE` | Descending cadence | $1568 \rightarrow 1046$ (G6-C6) | $80\text{ ms each}$ | Normal cycle stop |
| `SOUND_ALARM_FAULT`| Piercing urgent pulse | $2800 \leftrightarrow 800\text{ Hz}$ alternating | $250\text{ ms each}$ | Emergency / sensor fault |
| `SOUND_CONFIRM` | Crisp high chime | $1760 \rightarrow 3520$ (A6-A7) | $50\text{ ms each}$ | Parameter saved to NVS |

### 3.3. Non-Blocking FreeRTOS Audio Queue Driver

```cpp
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/ledc.h"

#define PIN_BUZZER_PWM  45
#define LEDC_BUZZER_CH  LEDC_CHANNEL_0
#define LEDC_BUZZER_TMR LEDC_TIMER_0

typedef enum {
    SOUND_KEY_PRESS,
    SOUND_START_CYCLE,
    SOUND_PORTION_DONE,
    SOUND_STOP_CYCLE,
    SOUND_ALARM_FAULT,
    SOUND_CONFIRM
} sound_event_t;

static QueueHandle_t s_sound_queue = NULL;

void sound_init(void) {
    ledc_timer_config_t timer_conf = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_BUZZER_TMR,
        .freq_hz          = 2000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {
        .gpio_num   = PIN_BUZZER_PWM,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_BUZZER_CH,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_BUZZER_TMR,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&ch_conf);

    s_sound_queue = xQueueCreate(8, sizeof(sound_event_t));
    xTaskCreatePinnedToCore(sound_task, "sound_task", 1536, NULL, 3, NULL, 0);
}

void sound_play(sound_event_t evt) {
    if (s_sound_queue) {
        xQueueSend(s_sound_queue, &evt, 0); // Non-blocking
    }
}

static void tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CH, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CH);
    } else {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_TMR, freq_hz);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CH, 512); // 50% duty
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CH);
    }
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CH, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CH);
}

static void sound_task(void *pvParameters) {
    sound_event_t evt;
    while (1) {
        if (xQueueReceive(s_sound_queue, &evt, portMAX_DELAY) == pdTRUE) {
            switch (evt) {
                case SOUND_KEY_PRESS:
                    tone(2400, 20);
                    break;
                case SOUND_START_CYCLE:
                    tone(1046, 60); tone(1318, 60); tone(1568, 80);
                    break;
                case SOUND_PORTION_DONE:
                    tone(2093, 40); tone(2637, 50);
                    break;
                case SOUND_STOP_CYCLE:
                    tone(1568, 70); tone(1046, 90);
                    break;
                case SOUND_ALARM_FAULT:
                    for (int i = 0; i < 3; ++i) {
                        tone(2800, 180); tone(800, 180);
                    }
                    break;
                case SOUND_CONFIRM:
                    tone(1760, 50); tone(3520, 60);
                    break;
            }
        }
    }
}
```

---

## 4. Integration Verification Checklist

- [ ] Shift transitions trigger exactly on hour thresholds without cycle interruption.
- [ ] Material consumption variables use 32-bit floats with bounded limits ($0.0$ to $99999.9$).
- [ ] LEDC hardware buzzer driver releases the output to 0V when silent (no continuous DC current through piezo).
- [ ] Audio task stack does not exceed 1536 words on Core 0.
