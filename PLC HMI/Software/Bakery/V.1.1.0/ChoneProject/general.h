#ifndef GENERAL_H
#define GENERAL_H

#include <Arduino.h>
#include <esp_system.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "sound.h"

// ===== Button Helpers =====
struct ButtonState
{
    int lastReading = 0;
    int stableKey = 0;
    unsigned long lastChangeTime = 0;
};
enum ScreenMap
{
    WARNING,
    MAIN,
    AUTOMATIC,
    MANUAL,
    SETTING,
    DIAGNOSTICS
};

//? ============================================================================
//? RTC SRAM CRASH & HANG RECOVERY (Survives SW/WDT/Panic resets with 0 Flash writes)
//? ============================================================================
RTC_NOINIT_ATTR static uint32_t ui_resume_scr_magic;
RTC_NOINIT_ATTR static uint32_t ui_resume_scr_magic_inv;
RTC_NOINIT_ATTR static uint32_t ui_resume_view_code;

static bool ui_resume_boot_latched = false;
static bool ui_resume_boot_valid = false;
static ScreenMap ui_resume_boot_scr = MAIN;

static inline bool ui_resume_reset_is_hang(esp_reset_reason_t rr)
{
    switch (rr)
    {
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
        case ESP_RST_SW:
#ifdef ESP_RST_CPU_LOCKUP
        case ESP_RST_CPU_LOCKUP:
#endif
            return true;
        default:
            return false;
    }
}

static inline void ui_resume_mark_current(ScreenMap scr)
{
    const uint32_t code = UI_RESUME_VIEW_PREFIX | ((uint32_t)scr & 0xFFu);
    if (ui_resume_scr_magic == UI_RESUME_SCR_MAGIC &&
        ui_resume_scr_magic_inv == ~UI_RESUME_SCR_MAGIC &&
        ui_resume_view_code == code)
    {
        return;
    }
    ui_resume_view_code = code;
    ui_resume_scr_magic = UI_RESUME_SCR_MAGIC;
    ui_resume_scr_magic_inv = ~UI_RESUME_SCR_MAGIC;
}

static inline void ui_resume_boot_latch_early()
{
    if (ui_resume_boot_latched) return;
    ui_resume_boot_latched = true;

    const esp_reset_reason_t rr = esp_reset_reason();
    const bool hang = ui_resume_reset_is_hang(rr);

    if (!hang)
    {
        ui_resume_scr_magic = 0;
        ui_resume_scr_magic_inv = 0;
        ui_resume_view_code = 0;
        ui_resume_boot_valid = false;
        return;
    }

    const bool ok = (ui_resume_scr_magic == UI_RESUME_SCR_MAGIC &&
                     ui_resume_scr_magic_inv == ~UI_RESUME_SCR_MAGIC &&
                     (ui_resume_view_code & 0xFF000000u) == UI_RESUME_VIEW_PREFIX);
    if (!ok)
    {
        ui_resume_scr_magic = 0;
        ui_resume_scr_magic_inv = 0;
        ui_resume_view_code = 0;
        ui_resume_boot_valid = false;
        return;
    }

    uint32_t raw_scr = ui_resume_view_code & 0xFFu;
    if (raw_scr > (uint32_t)DIAGNOSTICS) raw_scr = (uint32_t)MAIN;
    if (raw_scr == (uint32_t)WARNING) raw_scr = (uint32_t)MAIN;

    ui_resume_boot_scr = (ScreenMap)raw_scr;
    ui_resume_boot_valid = true;
}

static inline bool ui_resume_boot_get_latched(ScreenMap *out_scr)
{
    if (out_scr) *out_scr = ui_resume_boot_valid ? ui_resume_boot_scr : MAIN;
    return ui_resume_boot_latched && ui_resume_boot_valid;
}

// B-10: باندهای گسترش یافته صنعتی زیمنس — کاهش miss zone به حداقل برای نویز و افت ولتاژ کابل
// مقادیر اسمی: F1≈4095, F2≈3320, F3≈2710, F4≈1955, F5≈1211
static inline int getKeyFromAnalog(int value)
{
    if (value >= 3800 && value <= 4095) return 1; // F1
    if (value >= 3050 && value <= 3650) return 2; // F2
    if (value >= 2450 && value <= 2950) return 3; // F3
    if (value >= 1700 && value <= 2150) return 4; // F4
    if (value >= 1000 && value <= 1450) return 5; // F5
    return 0; // بدون کلید
}

int readDebouncedButton(int analogPin, ButtonState &state)
{
    int rawValue = analogRead(analogPin);
    int currentReading = getKeyFromAnalog(rawValue);

    if (currentReading != state.lastReading)
    {
        state.lastChangeTime = millis();
    }

    if ((millis() - state.lastChangeTime) > 50)
    {
        if (currentReading != state.stableKey)
        {
            state.stableKey = currentReading;
            state.lastReading = currentReading;
            return state.stableKey;
        }
    }

    state.lastReading = currentReading;
    return 0;
}

// ===== Memory Report Helper =====
void printMemoryReport()
{
    size_t flashSize = ESP.getFlashChipSize();
    size_t sketchSize = ESP.getSketchSize();
    size_t freeSketch = ESP.getFreeSketchSpace();
    float flashUsedPercent = (float)sketchSize / flashSize * 100;
    Serial.println("📊 Memory Usage Report:\n");

    Serial.printf("🔹 Flash Size: %u KB\n", flashSize / 1024);
    Serial.printf("🔹 Sketch Size: %u KB (%.2f%% used)\n", sketchSize / 1024, flashUsedPercent);
    Serial.printf("🔹 Free Sketch Space: %u KB\n\n", freeSketch / 1024);

    size_t heapTotal = ESP.getHeapSize();
    size_t heapFree = ESP.getFreeHeap();
    size_t heapUsed = heapTotal - heapFree;
    float heapUsedPercent = (float)heapUsed / heapTotal * 100;

    Serial.printf("🧠 Internal RAM (DRAM):\n");
    Serial.printf("🔹 Total: %u KB\n", heapTotal / 1024);
    Serial.printf("🔹 Used : %u KB (%.2f%% used)\n", heapUsed / 1024, heapUsedPercent);
    Serial.printf("🔹 Free : %u KB\n\n", heapFree / 1024);

    if (psramFound())
    {
        size_t psramTotal = ESP.getPsramSize();
        size_t psramFree = ESP.getFreePsram();
        size_t psramUsed = psramTotal - psramFree;
        float psramUsedPercent = (float)psramUsed / psramTotal * 100;

        Serial.printf("🧠 PSRAM:\n");
        Serial.printf("🔹 Total: %u KB\n", psramTotal / 1024);
        Serial.printf("🔹 Used : %u KB (%.2f%% used)\n", psramUsed / 1024, psramUsedPercent);
        Serial.printf("🔹 Free : %u KB\n", psramFree / 1024);
    }
    else
    {
        Serial.println("❌ PSRAM not detected.");
    }
}

// ===== Industrial Siemens Fail-Safe Pin Setup =====
void setupPin()
{
    // Buzzer (Active HIGH transistor drive: guarantee silence during boot)
    digitalWrite(BUZZER, LOW);
    pinMode(BUZZER, OUTPUT);

    // Relay Outputs (Active LOW: ON=LOW, OFF=HIGH)
    // Siemens Safe Boot: Drive pins HIGH before setting as OUTPUT to prevent spurious relay triggers
    digitalWrite(INVERTER_FORWARD, OFF);
    pinMode(INVERTER_FORWARD, OUTPUT);

    digitalWrite(INVERTER_REVERSE, OFF);
    pinMode(INVERTER_REVERSE, OUTPUT);

    digitalWrite(FUNNELE_MOTOR, OFF);
    pinMode(FUNNELE_MOTOR, OUTPUT);

    digitalWrite(PISTON_MOTOR, OFF);
    pinMode(PISTON_MOTOR, OUTPUT);

    // Inductive sensor inputs
    pinMode(PISTON_SENSOR, INPUT);
    pinMode(FUNNELE_SENSOR, INPUT);
}

//? ============================================================================
//? INVERTER INTERLOCK & 100ms DEAD-TIME PROTECTION (Siemens Safety Standard)
//? ============================================================================
static int current_inverter_dir = 0; // 0 = STOP, 1 = FWD, 2 = REV

static inline void setInverterDirection(int target_dir)
{
    if (target_dir == current_inverter_dir) return;

    // If changing active direction between FWD and REV, enforce mandatory dead-time
    if (current_inverter_dir != 0 && target_dir != 0)
    {
        digitalWrite(INVERTER_FORWARD, OFF);
        digitalWrite(INVERTER_REVERSE, OFF);
        vTaskDelay(pdMS_TO_TICKS(INVERTER_DEADTIME_MS));
    }

    if (target_dir == 1) // FWD
    {
        digitalWrite(INVERTER_REVERSE, OFF);
        digitalWrite(INVERTER_FORWARD, ON);
    }
    else if (target_dir == 2) // REV
    {
        digitalWrite(INVERTER_FORWARD, OFF);
        digitalWrite(INVERTER_REVERSE, ON);
    }
    else // STOP
    {
        digitalWrite(INVERTER_FORWARD, OFF);
        digitalWrite(INVERTER_REVERSE, OFF);
    }
    current_inverter_dir = target_dir;
}

void pistonDown()
{
    digitalWrite(FUNNELE_MOTOR, OFF);
    setInverterDirection(1); // FWD
    digitalWrite(PISTON_MOTOR, ON);
}

void pistonUP()
{
    digitalWrite(FUNNELE_MOTOR, OFF);
    setInverterDirection(2); // REV
    digitalWrite(PISTON_MOTOR, ON);
}

void pistonStop()
{
    digitalWrite(PISTON_MOTOR, OFF);
    setInverterDirection(0); // STOP
}

void funneleRight()
{
    digitalWrite(PISTON_MOTOR, OFF);
    setInverterDirection(1); // FWD
    digitalWrite(FUNNELE_MOTOR, ON);
}

void funneleLeft()
{
    digitalWrite(PISTON_MOTOR, OFF);
    setInverterDirection(2); // REV
    digitalWrite(FUNNELE_MOTOR, ON);
}

void funneleStop()
{
    digitalWrite(FUNNELE_MOTOR, OFF);
    setInverterDirection(0); // STOP
}

void checkBoot()
{
    delay(100);
    int read = analogRead(KEY_M);

    if (getKeyFromAnalog(read) == 5)
    {

        const esp_partition_t *first_app_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);

        if (first_app_partition != NULL)
        {
            esp_err_t err = esp_ota_set_boot_partition(first_app_partition);

            if (err == ESP_OK)
            {
                esp_restart();
            }
            else
            {
                Serial.print("Error BIOS  : ");
                Serial.println(esp_err_to_name(err));
            }
        }
        else
        {
            Serial.println("BIOS OTA 404!");
        }
    }
}

#endif
