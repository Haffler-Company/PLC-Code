#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include "defines.h"

//? ============================================================================
//? HARDWARE PWM & MELODY DEFINITIONS (ESP32-S3 LEDC)
//? ============================================================================
#define BUZZER_LEDC_CHANNEL 0
#define BUZZER_LEDC_RES     8       // 8-bit resolution (0..255)
#define BUZZER_DUTY_CYCLE   128     // 50% duty cycle for clean square/tone wave

// Musical Note Frequencies (Hz)
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_E6  1319
#define NOTE_G6  1568

enum SoundType : uint8_t {
    SOUND_NONE = 0,
    SOUND_CLICK,            // Subtle 25ms tactile click on button press
    SOUND_PORTION_DONE,     // Affirmative melodic chime on portion completion
    SOUND_START_CYCLE,      // Rising major arpeggio on cycle start
    SOUND_STOP_CYCLE,       // Descending tone on cycle stop
    SOUND_ALARM_FAULT,      // Urgent dual-tone warble on sensor timeout / emergency
    SOUND_KEY_PRESS,        // Standard keypad click
    SOUND_WARNING           // Short double beep
};

// Global queue handle for non-blocking asynchronous sound playback
extern QueueHandle_t soundQueue;

// Initialize LEDC timer peripheral on GPIO 45
static inline void sound_init() {
    ledcSetup(BUZZER_LEDC_CHANNEL, 2000, BUZZER_LEDC_RES);
    ledcAttachPin(BUZZER, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0); // Silence

    if (soundQueue == NULL) {
        soundQueue = xQueueCreate(16, sizeof(SoundType));
    }
}

// Low-level tone generator (duration in ms)
static inline void sound_tone(uint32_t freqHz, uint32_t durationMs) {
    if (freqHz == 0) {
        ledcWrite(BUZZER_LEDC_CHANNEL, 0);
    } else {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, freqHz);
        ledcWrite(BUZZER_LEDC_CHANNEL, BUZZER_DUTY_CYCLE);
    }
    vTaskDelay(pdMS_TO_TICKS(durationMs));
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

// Play distinct melodic sequences (Executed inside Core 0 buzzer_task)
static inline void sound_execute_melody(SoundType melody) {
    switch (melody) {
        case SOUND_CLICK:
        case SOUND_KEY_PRESS:
            sound_tone(NOTE_C6, 25);
            break;

        case SOUND_PORTION_DONE:
            sound_tone(NOTE_E5, 45);
            vTaskDelay(pdMS_TO_TICKS(15));
            sound_tone(NOTE_G5, 65);
            break;

        case SOUND_START_CYCLE:
            sound_tone(NOTE_C5, 50);
            vTaskDelay(pdMS_TO_TICKS(10));
            sound_tone(NOTE_E5, 50);
            vTaskDelay(pdMS_TO_TICKS(10));
            sound_tone(NOTE_G5, 50);
            vTaskDelay(pdMS_TO_TICKS(10));
            sound_tone(NOTE_C6, 120);
            break;

        case SOUND_STOP_CYCLE:
            sound_tone(NOTE_G5, 60);
            vTaskDelay(pdMS_TO_TICKS(15));
            sound_tone(NOTE_C5, 90);
            break;

        case SOUND_ALARM_FAULT:
            for (int i = 0; i < 3; i++) {
                sound_tone(880, 120);
                vTaskDelay(pdMS_TO_TICKS(30));
                sound_tone(1760, 160);
                vTaskDelay(pdMS_TO_TICKS(30));
            }
            break;

        case SOUND_WARNING:
            sound_tone(NOTE_A5, 60);
            vTaskDelay(pdMS_TO_TICKS(40));
            sound_tone(NOTE_A5, 60);
            break;

        default:
            break;
    }
}

// Asynchronous non-blocking trigger (Safe to call from any core, ISR, or UI callback)
static inline void sound_play(SoundType melody) {
    if (soundQueue != NULL) {
        xQueueSend(soundQueue, &melody, 0);
    }
}

// Backward-compatible buzzer pulse for legacy code
static inline void buzzer(int count, unsigned int delay_ms_on, unsigned int delay_ms_off) {
    for (int i = 0; i < count; i++) {
        sound_tone(2400, delay_ms_on);
        if (delay_ms_off > 0) {
            vTaskDelay(pdMS_TO_TICKS(delay_ms_off));
        }
    }
}

#ifdef __cplusplus
extern "C" {
#endif
void sound_play_c(uint8_t sound_type);
#ifdef __cplusplus
}
#endif

#endif // SOUND_H
