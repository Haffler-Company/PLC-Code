# Implementation Blueprint: Multi-Tone Melodic Buzzer Synthesizer

**Document ID:** SPEC-006  
**Target Subsystem:** Acoustic User Feedback & Audio Cues  
**Hardware Pin:** `BUZZER` (GPIO 45)  
**Peripheral:** ESP32-S3 LEDC Hardware PWM  
**Benchmarked Standard:** `Welding_single_head/firmware/include/app/app_sound.h`

---

## 1. Executive Summary & Objective

The legacy bakery firmware uses crude, blocking digital on/off pulses for the buzzer (`digitalWrite(BUZZER, HIGH); delay(100);`). This produces a harsh, monotone square wave that provides no distinction between a button click, a portion completed, cycle start, or a critical sensor jam.

This specification provides the blueprint to:
1. Transition `BUZZER` (GPIO 45) to the ESP32-S3 hardware LEDC PWM timer.
2. Implement non-blocking multi-tone melodies and harmonic audio cues.
3. Define distinct acoustic profiles:
   - Button click (subtle 20ms chirp)
   - Portion ejected (pleasant affirmative chime)
   - Cycle start (rising major arpeggio)
   - Sensor fault / E-stop (urgent alternating alarm warble).

---

## 2. Musical Note Frequencies & Audio Constants

Standard musical note frequencies (Hz):
```cpp
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
```

---

## 3. LEDC PWM Driver Implementation Blueprint

```cpp
#include <Arduino.h>

#define BUZZER_LEDC_CHANNEL 0
#define BUZZER_LEDC_RES     8  // 8-bit resolution (0..255 duty)

enum class SoundMelody {
    CLICK,
    PORTION_DONE,
    START_CYCLE,
    STOP_CYCLE,
    ALARM_FAULT
};

void initSoundHardware() {
    ledcSetup(BUZZER_LEDC_CHANNEL, 2000, BUZZER_LEDC_RES);
    ledcAttachPin(BUZZER, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0); // Silence
}

void playTone(uint32_t freqHz, uint32_t durationMs) {
    if (freqHz == 0) {
        ledcWrite(BUZZER_LEDC_CHANNEL, 0);
    } else {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, freqHz);
        ledcWrite(BUZZER_LEDC_CHANNEL, 128); // 50% duty cycle
    }
    vTaskDelay(pdMS_TO_TICKS(durationMs));
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}
```

---

## 4. Melodic Profiles & Sequences

### 4.1. Keypad Click (`SoundMelody::CLICK`):
```cpp
void playClick() {
    playTone(NOTE_C6, 25);
}
```

### 4.2. Portion Ejected (`SoundMelody::PORTION_DONE`):
```cpp
void playPortionChime() {
    playTone(NOTE_E5, 40);
    vTaskDelay(pdMS_TO_TICKS(15));
    playTone(NOTE_G5, 60);
}
```

### 4.3. Cycle Start (`SoundMelody::START_CYCLE`):
```cpp
void playStartArpeggio() {
    playTone(NOTE_C5, 60);
    playTone(NOTE_E5, 60);
    playTone(NOTE_G5, 60);
    playTone(NOTE_C6, 120);
}
```

### 4.4. Critical Alarm (`SoundMelody::ALARM_FAULT`):
```cpp
void playAlarmWarble() {
    for (int i = 0; i < 3; i++) {
        playTone(800, 150);
        vTaskDelay(pdMS_TO_TICKS(50));
        playTone(1600, 200);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

---

## 5. FreeRTOS Asynchronous Queue Integration

To ensure audio playback never blocks motion loops or UI frame rates:
- Audio requests are queued via a FreeRTOS Queue (`xQueueSend(soundQueue, &melody, 0)`).
- The dedicated Core 0 `buzzer_task` receives and plays melodies asynchronously.
