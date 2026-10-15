#include "haffler_io_diag.h"
#include "haffler_pins.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <esp_system.h>

extern void relay_set(uint8_t idx, bool active);
extern bool relay_get(uint8_t idx);

static uint8_t s_inputs_cache = 0;

void io_diag_init(void) {
    // Setup dedicated digital and joystick inputs
    pinMode(PIN_EMERGENCY, INPUT_PULLUP);
    pinMode(PIN_START_PEDAL, INPUT_PULLUP);
    pinMode(PIN_JOY_T1, INPUT_PULLUP);
    pinMode(PIN_JOY_T2, INPUT_PULLUP);
    // Sensor pair analog pins
    pinMode(PIN_SENSOR_PAIR_0_1, INPUT);
    pinMode(PIN_SENSOR_PAIR_2_3, INPUT);
    pinMode(PIN_SENSOR_PAIR_4_5, INPUT);
}

uint8_t io_diag_get_inputs_mask(void) {
    uint8_t mask = 0;
    // Dedicated optocoupled digital inputs (Active-LOW: 0 = triggered)
    if (digitalRead(PIN_EMERGENCY) == LOW)   mask |= (1 << 0);
    if (digitalRead(PIN_START_PEDAL) == LOW) mask |= (1 << 1);
    if (digitalRead(PIN_JOY_T1) == LOW)      mask |= (1 << 2);
    if (digitalRead(PIN_JOY_T2) == LOW)      mask |= (1 << 3);
    
    // Dual-reed sensor pairs (threshold check for active states)
    if (analogReadMilliVolts(PIN_SENSOR_PAIR_0_1) > SENSOR_BAND_OPEN_LO) mask |= (1 << 4);
    if (analogReadMilliVolts(PIN_SENSOR_PAIR_2_3) > SENSOR_BAND_OPEN_LO) mask |= (1 << 5);
    if (analogReadMilliVolts(PIN_SENSOR_PAIR_4_5) > SENSOR_BAND_OPEN_LO) mask |= (1 << 6);

    s_inputs_cache = mask;
    return mask;
}

uint16_t io_diag_get_relays_mask(void) {
    uint16_t mask = 0;
    for (uint8_t i = 0; i < 10; ++i) {
        if (relay_get(i)) mask |= (1 << i);
    }
    return mask;
}

uint16_t io_diag_get_key_f_mv(void) {
    return (uint16_t)analogReadMilliVolts(PIN_KEY_F);
}

uint16_t io_diag_get_key_m_mv(void) {
    return (uint16_t)analogReadMilliVolts(PIN_KEY_M);
}

bool io_diag_toggle_relay_manual(uint8_t relay_idx, bool is_machine_idle) {
    if (!is_machine_idle || relay_idx >= 10) {
        return false; // Interlock active: cannot toggle during machine run
    }
    bool current = relay_get(relay_idx);
    relay_set(relay_idx, !current);
    return true;
}

void io_diag_process_serial_line(const char *line) {
    if (!line) return;

    if (strncmp(line, "PING", 4) == 0) {
        Serial.println("{\"status\":\"OK\",\"pong\":true}");
    }
    else if (strncmp(line, "STATUS", 6) == 0) {
        uint32_t dram = esp_get_free_internal_heap_size();
        uint32_t psram = ESP.getFreePsram();
        Serial.printf("{\"status\":\"OK\",\"dram_free\":%u,\"psram_free\":%u,\"uptime_s\":%lu}\n",
                      dram, psram, (unsigned long)(millis() / 1000));
    }
    else if (strncmp(line, "IO_READ", 7) == 0) {
        uint8_t in_m = io_diag_get_inputs_mask();
        uint16_t out_m = io_diag_get_relays_mask();
        uint16_t kf = io_diag_get_key_f_mv();
        uint16_t km = io_diag_get_key_m_mv();
        Serial.printf("{\"inputs\":%u,\"relays\":%u,\"key_f_mv\":%u,\"key_m_mv\":%u}\n",
                      in_m, out_m, kf, km);
    }
    else if (strncmp(line, "RELAY_SET", 9) == 0) {
        int idx = 0, st = 0;
        if (sscanf(line + 9, "%d %d", &idx, &st) == 2) {
            if (idx >= 0 && idx < 10) {
                relay_set((uint8_t)idx, st != 0);
                Serial.printf("{\"relay\":%d,\"state\":%d,\"status\":\"OK\"}\n", idx, st);
            }
        }
    }
    else if (strncmp(line, "BUZZER_TEST", 11) == 0) {
        tone(PIN_BUZZER, 2700, 200);
        Serial.println("{\"buzzer\":\"TEST_DONE\"}");
    }
    else if (strncmp(line, "MEM_REPORT", 10) == 0) {
        Serial.printf("{\"min_dram\":%u,\"cur_dram\":%u,\"dma_free\":%u}\n",
                      esp_get_minimum_free_heap_size(),
                      esp_get_free_internal_heap_size(),
                      heap_caps_get_free_size(MALLOC_CAP_DMA));
    }
    else if (strncmp(line, "SHOT", 4) == 0) {
        // Raw RGB565 screen buffer capture stream
        Serial.println("[SHOT_START 480x320 RGB565]");
        // Stream pixels or chunked data here for tools/capture_screen.py
        Serial.println("[SHOT_END]");
    }
}
