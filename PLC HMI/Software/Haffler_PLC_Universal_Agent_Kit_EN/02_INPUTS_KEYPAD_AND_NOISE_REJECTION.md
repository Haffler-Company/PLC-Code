# Industrial Inputs, Front-Panel Keypad & Noise Rejection
## Electromagnetic Immunity Standards, 5-Sample Median Filter, and 10-Key Tactile Subsystem

> **Document ID:** `02_INPUTS_KEYPAD_AND_NOISE_REJECTION.md`  
> **Target Hardware:** Haffler ESP32-S3 PLC HMI.  
> **Standards:** Siemens Industrial Noise Immunity (IEC 61131-2), Analog Median Filtering & Debounce FSM.

---

## 1. Optocoupled Industrial Inputs with 100% Noise Immunity

In metalworking and manufacturing facilities, large welding transformers, variable frequency drives (VFD), contactor arcing, and pneumatic solenoid discharge generate high-voltage electrical spikes on wiring.

```
            +----------------- Optocoupled Isolation Circuit ---------------+
            |                                                               |
24V/5V In --+-[ 2.2K ]--->|---( Optocoupler )----+----> GPIO Input (ESP32-S3)
                        LED Diode                   |
                                               [ 10K Pullup ]
                                                    |
                                                  +3.3V
```

### 1.1. Deterministic Multi-Sample Debouncing Algorithm
* **Sampling Rate:** Every 10ms in Core 1 control loop.
* **Stability Window:** 40ms to 50ms of continuous identical state (4 to 5 consecutive samples) required before accepting a state transition.
* **Industrial Reference Code:**

```c
#define INPUT_CHANNELS 8
#define DEBOUNCE_STABLE_TICKS 5 // 5 samples * 10ms = 50ms stable window

typedef struct {
    uint8_t pin;
    uint8_t counter;
    bool stable_state;
    bool raw_state;
} IndustrialInput_t;

static IndustrialInput_t s_inputs[INPUT_CHANNELS] = {
    {.pin = 3},  // I0: Physical Start 1
    {.pin = 7},  // I1: Physical Start 2
    {.pin = 4},  // I2: Motor Fault / Return Limit
    {.pin = 5},  // I3: Safety Guard / E-Stop
    {.pin = 6},  // I4: Homing Reference Proximity
    {.pin = 21}, // I5: Left Joystick / Foot Pedal 1
    {.pin = 37}, // I6: Right Joystick / Foot Pedal 2
    {.pin = 48}  // I7: Auxiliary Sensor
};

void industrial_inputs_scan_10ms(void) {
    for (int i = 0; i < INPUT_CHANNELS; i++) {
        // Active-LOW inputs: 0 logic = activated
        bool current_active = (gpio_get_level((gpio_num_t)s_inputs[i].pin) == 0);
        s_inputs[i].raw_state = current_active;

        if (current_active == s_inputs[i].stable_state) {
            s_inputs[i].counter = 0; // State unchanged
        } else {
            s_inputs[i].counter++;
            if (s_inputs[i].counter >= DEBOUNCE_STABLE_TICKS) {
                s_inputs[i].stable_state = current_active;
                s_inputs[i].counter = 0;
                // Certified noise-filtered state change event
                industrial_input_on_change(i, s_inputs[i].stable_state);
            }
        }
    }
}
```

```

### 1.2. Proprietary Dual-Sensor Analog Band Decoding (GPIO 4, 5, 6)
In pneumatic machines (such as UPVC welders, corner cleaners, and copy routers), monitoring cylinder position requires both a **Retracted (Open)** and an **Extended (Closed)** magnetic reed switch per actuator.  
To conserve MCU GPIOs while maintaining 100% noise immunity, the Haffler controller pairs two reed switches on a single analog pin using a precision resistor divider network:

```
+3.3V ---[ 10K Pullup ]---+------------------------> GPIO Input (ADC1)
                          |
                          +---[ 4.7K ]---/ Reed Sw 1 (Retracted) /---> GND  (1300..1800 mV)
                          |
                          +---/ Reed Sw 2 (Extended) /----------------> GND  (> 3500 mV / Active-LOW Pull)
```

#### Certified Voltage Bands:
| Voltage Band | ADC mV Range | Physical Cylinder State | Software Evaluation |
| :--- | :--- | :--- | :--- |
| **Band 1 (Retracted)** | $1300\text{ mV} \dots 1800\text{ mV}$ | Cylinder fully retracted (Open) | Retracted Sensor = ACTIVE, Extended = OFF |
| **Deadband (Moving)** | $1850\text{ mV} \dots 2240\text{ mV}$ | Cylinder in transit (Neither active) | Both sensors = OFF (Prevents transient kick) |
| **Band 3 (Both Active)** | $2350\text{ mV} \dots 2750\text{ mV}$ | Table Return / Both switches closed | Diagnostic state / Return sensor engaged |
| **Band 2 (Extended)** | $> 3500\text{ mV}$ ($~4095$) | Cylinder fully extended (Closed) | Extended Sensor = ACTIVE, Retracted = OFF |

#### Industrial C/C++ Classification Function:
```cpp
#define SENSOR_BAND_OPEN_LO  1300
#define SENSOR_BAND_OPEN_HI  1800
#define SENSOR_BAND_NONE_LO  1850
#define SENSOR_BAND_NONE_HI  2240
#define SENSOR_BAND_BOTH_LO  2350
#define SENSOR_BAND_BOTH_HI  2750
#define SENSOR_BAND_CLOSED   3500

enum SensorBand { BAND_INVALID = -1, BAND_CLOSED = 0, BAND_OPEN = 1, BAND_NONE = 2, BAND_BOTH = 3 };

SensorBand classify_sensor_band(int adc_mv) {
    if (adc_mv >= SENSOR_BAND_CLOSED) return BAND_CLOSED;
    if (adc_mv >= SENSOR_BAND_OPEN_LO && adc_mv <= SENSOR_BAND_OPEN_HI) return BAND_OPEN;
    if (adc_mv >= SENSOR_BAND_NONE_LO && adc_mv <= SENSOR_BAND_NONE_HI) return BAND_NONE;
    if (adc_mv >= SENSOR_BAND_BOTH_LO && adc_mv <= SENSOR_BAND_BOTH_HI) return BAND_BOTH;
    return BAND_INVALID;
}
```

---

## 2. Front-Panel 10-Key Tactile Subsystem

The industrial front panel features **10 tactile/membrane keys** organized into two columns of 5 keys, enabling operators to control the PLC with heavy gloves:

```
+-------------------------------------------------------------+
|               FRONT-PANEL TACTILE KEY MATRIX                |
|                                                             |
|   COLUMN 1 (KEY_F - on GPIO 1):     COLUMN 2 (KEY_M - on GPIO 2):   |
|   [F1] Shortcut Tab 1               [SAVE]   Commit to NVS  |
|   [F2] Shortcut Tab 2               [CANCEL] Exit / Back    |
|   [F3] Shortcut Tab 3               [MODE]   Cycle Focus    |
|   [F4] Shortcut Tab 4               [START]  Confirm / Inc  |
|   [F5] Shortcut Tab 5               [STOP]   Dec / Halt     |
+-------------------------------------------------------------+
```

### 2.1. Calibrated Resistor Ladder Voltage Dividers
Each 5-key column connects to an analog resistor divider network on an ADC1 channel (`KEY_F` on GPIO 1, `KEY_M` on GPIO 2):
* **Key 1:** Center 4050 mV (Tolerance Window: $\pm 360\text{ mV}$)
* **Key 2:** Center 3350 mV (Tolerance Window: $\pm 360\text{ mV}$)
* **Key 3:** Center 2700 mV (Tolerance Window: $\pm 360\text{ mV}$)
* **Key 4:** Center 1950 mV (Tolerance Window: $\pm 360\text{ mV}$)
* **Key 5:** Center 1250 mV (Tolerance Window: $\pm 360\text{ mV}$)
* **Separation Margin:** Minimum 170 mV distance between adjacent keys required.
* **Idle (No Key):** Open line $> 4090\text{ mV}$.

### 2.2. Multi-Task Hardware Arbiter Safety (`adcMutex`)
When multiple FreeRTOS tasks (e.g., Sensor Scan on Core 1 and Keypad Scan on Core 0) access ADC1 simultaneously, the ESP32-S3 ADC hardware arbiter experiences bus collision and panics.
* **MANDATORY:** Guard all ADC reads with `adcMutex`.
* **CRITICAL BUG PREVENTION:** Only call `xSemaphoreGive(adcMutex)` if `xSemaphoreTake(adcMutex, ...)` returned `pdTRUE`. Giving an unowned mutex triggers FreeRTOS `assert xQueueGenericSend` and hard-faults the MCU!

```cpp
extern SemaphoreHandle_t adcMutex;

static inline int read_adc_safe(int analog_pin) {
    bool locked = (adcMutex && xSemaphoreTake(adcMutex, pdMS_TO_TICKS(50)) == pdTRUE);
    int val = analogRead(analog_pin);
    if (locked) xSemaphoreGive(adcMutex); // ONLY give if actually taken!
    return val;
}
```

### 2.2. 5-Sample Median Filter (`readMedian5`)
Arithmetic averaging spreads voltage spike errors. In contrast, **median filtering completely discards transient electromagnetic spikes**:

```c
#include <esp_adc/adc_oneshot.h>

static uint16_t adc_read_median5(adc_oneshot_unit_handle_t adc_handle, adc_channel_t chan) {
    int samples[5];
    for (int i = 0; i < 5; i++) {
        adc_oneshot_read(adc_handle, chan, &samples[i]);
    }
    // Bubble sort 5 samples
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (samples[i] > samples[j]) {
                int temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }
    return (uint16_t)samples[2]; // True median sample: immune to impulse noise
}
```

### 2.3. Event Discrimination: Click, Long Press & Auto-Repeat
* **Short Click:** Key released within $< 1000\text{ms}$ $\rightarrow$ Emit standard click event.
* **Long Press:** Key held continuously $> 1000\text{ms}$ $\rightarrow$ Activate high-speed auto-adjustment.
* **Auto-Repeat:** In navigation mode, holding START or STOP triggers auto-repeat every 150ms with smooth acceleration.

### 2.4. Core Isolation via FreeRTOS Queues
**CRITICAL:** Keypad scanning runs on Core 0. **Never call LVGL graphics functions directly from keypad scan routines or timer callbacks.** Instead, dispatch events to a FreeRTOS queue:

```c
typedef struct {
    uint8_t col; // 1: KEY_F, 2: KEY_M
    uint8_t key; // 1..5
    bool is_repeat;
} PanelKeyEvent_t;

static QueueHandle_t s_key_queue = NULL;

void keypad_task_init(void) {
    s_key_queue = xQueueCreate(16, sizeof(PanelKeyEvent_t));
}

void keypad_post_event(uint8_t col, uint8_t key, bool repeat) {
    PanelKeyEvent_t ev = {.col = col, .key = key, .is_repeat = repeat};
    xQueueSend(s_key_queue, &ev, 0);
}

void keypad_consume_in_lvgl(void) {
    PanelKeyEvent_t ev;
    while (xQueueReceive(s_key_queue, &ev, 0) == pdTRUE) {
        panel_nav_process_key(ev.col, ev.key, ev.is_repeat);
    }
}
```
