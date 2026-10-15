#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file machine_fsm.h
 * @brief Industrial Deterministic State Machine for Commercial Bakery Spiral Mixer
 * @details Conforms to Siemens PackML principles on FreeRTOS Core 1 (50ms scan).
 * Features:
 *   - Multi-shift calendar scheduler (DS3231 RTC) with pre-temp conditioning
 *   - Water dosing in Liters (Flowmeter or Calibrated Flowrate: L/min to seconds)
 *   - Manual or Motorized Flour Infeed (Auger screw)
 *   - Main Mixer Motor with Periodic Rotating Bowl Friction Brake
 *   - Safety interlocks (Guard, Overload, E-Stop)
 */

typedef enum {
    DOSING_MODE_FLOWMETER       = 0, // Dosing water by pulse flowmeter (liters)
    DOSING_MODE_CALIBRATED_TIME = 1  // Dosing water by calibrated rate: Time = (L / Rate) * 60s
} water_dosing_mode_t;

typedef enum {
    FLOUR_MODE_MANUAL    = 0, // Operator adds flour manually before cycle start
    FLOUR_MODE_MOTORIZED = 1  // Automated flour hopper auger motor Q1
} flour_feed_mode_t;

typedef struct {
    bool    enabled;
    uint8_t hour;   // 0..23
    uint8_t minute; // 0..59
} shift_schedule_t;

typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE,                 // Ready, waiting for operator start or scheduled shift trigger
    STATE_PRE_TEMP_REGULATION,  // Pre-conditioning water tank temperature (e.g. 30m before shift)
    STATE_FLOUR_FEEDING,        // Running motorized flour feeder auger Q1
    STATE_WATER_DOSING,         // Injecting water Q4 (measured in Liters via pulses or calibrated rate)
    STATE_MIXING,               // Spiral hook motor Q0 running + Bowl Brake Q2 pulsing
    STATE_CYCLE_COMPLETE,       // Mix completed, audible chime, stats updated
    STATE_PAUSED_GUARD,         // Bowl safety guard opened (safe halt of motor & countdown)
    STATE_FAULT,                // Motor overload bimetal or system trip
    STATE_ESTOP                 // Emergency stop mushroom engaged
} MachineState;

typedef struct {
    uint32_t mix_time_sec;             // Primary mix duration in seconds (e.g. 300s = 5m, 600s = 10m)
    uint8_t  dosing_mode;              // 0 = Flowmeter, 1 = Calibrated Timer
    float    water_target_liters;      // Target water volume in Liters (e.g. 20.0 L)
    float    flow_rate_l_per_min;      // Calibrated water flow rate (e.g. 3.0 L/min)
    uint32_t pulses_per_liter;         // Flowmeter pulse calibration factor (e.g. 450)

    float    water_target_temp_c;      // Desired water temp (e.g. 23.5°C)
    float    water_temp_tolerance_c;   // Temperature tolerance band (e.g. ±1.0°C)
    bool     water_temp_check_enabled; // Enable temperature regulation & pre-conditioning

    uint8_t  flour_mode;               // 0 = Manual, 1 = Motorized Auger
    uint32_t flour_feed_time_sec;      // Motorized flour run duration (e.g. 25s)
    float    flour_bag_weight_kg;      // Standard bag weight for shift accounting (50.0kg)

    bool     bowl_brake_enabled;       // Enable friction brake pulse during mix
    uint32_t brake_on_time_sec;        // Brake energized time (e.g. 3s)
    uint32_t brake_off_time_sec;       // Brake released time (e.g. 7s)

    uint8_t  pre_temp_lead_min;        // Lead time before shift to regulate water temp (e.g. 30m)
    shift_schedule_t shifts[3];        // 3 Programmable Daily Shifts (e.g. 04:00, 13:00, 16:00)
} bakery_config_t;

typedef struct {
    uint16_t rtc_year;
    uint8_t  rtc_month;
    uint8_t  rtc_day;
    uint8_t  rtc_hour;
    uint8_t  rtc_minute;
    uint8_t  rtc_second;
    bool     rtc_valid;

    float    water_temp_c;
    bool     water_temp_valid;
    bool     water_temp_ready;         // True when within target ± tolerance

    uint32_t remaining_mix_sec;
    float    current_water_dosed_l;    // Live water delivered in Liters
    uint32_t current_flour_sec;        // Live flour feeder elapsed sec

    bool     bowl_brake_active;        // Live state of bowl brake Q2
    bool     guard_closed;             // Safety bowl guard status (true = closed safe)
    bool     overload_ok;              // Motor thermal overload status (true = normal)
    bool     estop_ok;                 // Emergency stop status (true = normal / released)

    uint32_t batches_count;            // Total batches mixed during current shift
    float    total_water_liters;       // Total water dosed in liters
    float    total_flour_kg;           // Total flour processed in kg

    int8_t   next_shift_idx;           // Index of next scheduled shift (0..2 or -1)
    uint32_t minutes_until_next_shift; // Countdown minutes to next shift
    uint16_t last_alarm_code;          // Active alarm code

    uint8_t  daylock_stage;            // Active leasing stage (0..4)
    uint16_t daylock_days_left;        // Remaining days in stage
    bool     daylock_locked;           // True if motion is locked due to expired stage/tamper
} bakery_live_t;

class MachineFSM {
private:
    MachineState    m_state;
    uint32_t        m_state_entered_ms;
    uint32_t        m_cycle_start_ms;
    uint32_t        m_last_cycle_duration_ms;
    uint32_t        m_mix_start_ms;
    uint32_t        m_mix_elapsed_before_pause_sec;
    uint32_t        m_water_start_ms;
    uint32_t        m_flour_start_ms;
    uint32_t        m_last_temp_read_ms;
    uint32_t        m_last_rtc_read_ms;
    uint32_t        m_last_brake_toggle_ms;
    uint32_t        m_calculated_water_time_sec;
    uint32_t        m_vinput_mask;
    uint32_t        m_vinput_vals;
    bool            m_sim_temp_active;
    float           m_sim_temp_c;

    bakery_config_t m_config;
    bakery_live_t   m_live;

    void updateSensors(uint32_t now_ms);
    void updateRTC(uint32_t now_ms);
    void updateScheduler(uint32_t now_ms);
    void handleOutputs(uint32_t now_ms);
    void readWaterTemperature(void);

public:
    MachineFSM();

    void init(void);
    void update(uint32_t now_ms);

    // Operator Controls
    void startCycle(void);
    void stopCycle(void);
    void overrideTempWait(void);
    void pulseWaterValve(bool active);
    void pulseFlourFeeder(bool active);
    void pulseBowlBrake(bool active);

    // Safety & Alarms
    void triggerAlarm(uint16_t alarm_code);
    void clearAlarm(void);
    void triggerEStop(void);
    void resetEStop(void);

    // Getters
    MachineState getState(void) const { return m_state; }
    const char* getStateName(void) const;
    const bakery_config_t& getConfig(void) const { return m_config; }
    bakery_config_t* getConfigPtr(void) { return &m_config; }
    const bakery_live_t& getLive(void) const { return m_live; }
    
    // Setters & Adjusters
    void setMixTimeSec(uint32_t sec);
    void setDosingMode(uint8_t mode);
    void setWaterTargetLiters(float liters);
    void setFlowRateLPerMin(float l_per_min);
    void setWaterTargetTemp(float temp_c);
    void setFlourMode(uint8_t mode);
    void setFlourFeedTimeSec(uint32_t sec);
    void setBowlBrakeParams(bool enabled, uint32_t on_sec, uint32_t off_sec);
    void setShiftSchedule(uint8_t shift_idx, bool enabled, uint8_t hour, uint8_t minute);
    void setPreTempLeadMin(uint8_t lead_min);
    
    // RTC Clock Adjustments
    void setRTCDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    void adjustRTCHour(int8_t delta_hour);
    void adjustRTCMinute(int8_t delta_min);

    // 5-Stage Day-Lock Management
    bool isDayLockLocked(void) const;
    uint8_t getDayLockStage(void) const;
    uint16_t getDayLockRemainingDays(void) const;
    uint16_t getDayLockChallenge(void);
    bool verifyDayLockUnlock(uint16_t response_key);
    bool applyDayLockAdminBypass(const char* pin);
    bool applyDayLockEmergency(const char* code);

    // Virtual Input Testing & Remote Simulation
    void setVirtualInput(uint8_t pin, bool level);
    void clearVirtualInput(uint8_t pin);
    void clearAllVirtualInputs(void);
    bool isVirtualInputOverridden(uint8_t pin, bool* out_val) const;
    void simulateWaterTemp(float temp_c);
    void clearSimulatedTemp(void);
    void injectFlowPulses(uint32_t pulses);

    void saveConfigToNVS(void);
    void loadConfigFromNVS(void);
};

extern MachineFSM g_machine_fsm;
