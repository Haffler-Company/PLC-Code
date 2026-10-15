#include "machine_fsm.h"
#include "haffler_pins.h"
#include "haffler_day_lock.h"
#include "max6675.h"
#include <RTClib.h>
#include <Wire.h>
#include <Arduino.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>

static const char *TAG = "BAKERY_FSM";

MachineFSM g_machine_fsm;

// Hardware Relay indexes matching HAFFLER ETV3A
#define RELAY_IDX_MOTOR 0 // Q0 (GPIO 16) - Main Mixer Hook Motor Contactor
#define RELAY_IDX_FLOUR 1 // Q1 (GPIO 42) - Motorized Flour Feeder Auger Motor
#define RELAY_IDX_BRAKE 2 // Q2 (GPIO 39) - Rotating Bowl Friction Brake Solenoid
#define RELAY_IDX_WATER 4 // Q4 (GPIO 40) - Water Infeed Solenoid Valve
#define RELAY_IDX_TEMP  5 // Q5 (GPIO 15) - Water Tank Temperature Regulation Relay
#define RELAY_IDX_GREEN 8 // Q8 (GPIO 38) - Stack Light Green (Active Working)
#define RELAY_IDX_RED   9 // Q9 (GPIO 41) - Stack Light Red (Fault / Temperature Alert)

extern void relay_set(uint8_t idx, bool active);
extern bool relay_get(uint8_t idx);

// Flowmeter Pulse Counter (IRAM ISR)
static volatile uint32_t s_flow_pulse_count = 0;
static void IRAM_ATTR flow_meter_isr(void) {
    s_flow_pulse_count = s_flow_pulse_count + 1;
}

// Dedicated Hardware Drivers
static MAX6675 s_water_thermocouple(PIN_WATER_TEMP_SCK, PIN_WATER_TEMP_CS, PIN_WATER_TEMP_MISO);
static RTC_DS3231 s_rtc;
static bool s_rtc_online = false;

MachineFSM::MachineFSM() :
    m_state(STATE_BOOT),
    m_state_entered_ms(0),
    m_cycle_start_ms(0),
    m_last_cycle_duration_ms(0),
    m_mix_start_ms(0),
    m_mix_elapsed_before_pause_sec(0),
    m_water_start_ms(0),
    m_flour_start_ms(0),
    m_last_temp_read_ms(0),
    m_last_rtc_read_ms(0),
    m_last_brake_toggle_ms(0),
    m_calculated_water_time_sec(0),
    m_vinput_mask(0),
    m_vinput_vals(0xFFFFFFFF),
    m_sim_temp_active(false),
    m_sim_temp_c(23.5f)
{
    // Authoritative default configuration per workshop specifications
    m_config.mix_time_sec = 300;                    // 5 minutes mix time
    m_config.dosing_mode = DOSING_MODE_CALIBRATED_TIME; // Default: Calibrated Time mode
    m_config.water_target_liters = 20.0f;           // 20 Liters target
    m_config.flow_rate_l_per_min = 3.0f;            // 3.0 Liters/min calibration
    m_config.pulses_per_liter = 450;                // 450 pulses/L for pulse meter

    m_config.water_target_temp_c = 23.5f;          // Target 23.5°C water temp
    m_config.water_temp_tolerance_c = 1.0f;         // ±1.0°C tolerance band
    m_config.water_temp_check_enabled = true;       // Pre-temp conditioning enabled

    m_config.flour_mode = FLOUR_MODE_MANUAL;        // Manual or Motorized auger
    m_config.flour_feed_time_sec = 25;              // 25s auger run
    m_config.flour_bag_weight_kg = 50.0f;           // 50kg bag standard

    m_config.bowl_brake_enabled = true;             // Friction brake enabled
    m_config.brake_on_time_sec = 3;                 // 3s Brake ON
    m_config.brake_off_time_sec = 7;                // 7s Brake OFF

    m_config.pre_temp_lead_min = 30;                // 30 minutes lead time
    // Default 3 daily shifts: 04:00, 13:00, 16:00
    m_config.shifts[0] = {true, 4, 0};
    m_config.shifts[1] = {true, 13, 0};
    m_config.shifts[2] = {true, 16, 0};

    memset(&m_live, 0, sizeof(m_live));
    m_live.water_temp_c = 23.0f;
    m_live.water_temp_valid = false;
    m_live.water_temp_ready = false;
    m_live.guard_closed = true;
    m_live.overload_ok = true;
    m_live.estop_ok = true;
    m_live.remaining_mix_sec = m_config.mix_time_sec;
    m_live.next_shift_idx = 0;
}

void MachineFSM::init(void) {
    ESP_LOGI(TAG, "Initializing Advanced Bakery Spiral Mixer FSM");

    // Configure Industrial Digital Input pins with internal pull-ups
    pinMode(PIN_INPUT_ESTOP, INPUT_PULLUP);
    pinMode(PIN_INPUT_START, INPUT_PULLUP);
    pinMode(PIN_INPUT_GUARD, INPUT_PULLUP);
    pinMode(PIN_INPUT_OVERLOAD, INPUT_PULLUP);
    pinMode(PIN_INPUT_FLOWMETER, INPUT_PULLUP);

    // Attach high-speed pulse interrupt for flowmeter
    attachInterrupt(digitalPinToInterrupt(PIN_INPUT_FLOWMETER), flow_meter_isr, FALLING);

    // Initialize DS3231 RTC on hardware I2C bus (SDA: 8, SCL: 9)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
    s_rtc_online = s_rtc.begin(&Wire);
    if (s_rtc_online) {
        ESP_LOGI(TAG, "DS3231 Industrial RTC initialized successfully");
    } else {
        ESP_LOGW(TAG, "DS3231 RTC not detected on I2C, using monotonic software clock");
    }

    // Initialize 5-Stage Day-Lock cascading leasing engine (30 days per stage)
    const uint16_t stage_days[5] = {30, 30, 30, 30, 30};
    daylock_init(stage_days);

    loadConfigFromNVS();

    m_state = STATE_IDLE;
    m_state_entered_ms = millis();
    m_live.remaining_mix_sec = m_config.mix_time_sec;
}

void MachineFSM::updateRTC(uint32_t now_ms) {
    if (now_ms - m_last_rtc_read_ms < 1000) return;
    m_last_rtc_read_ms = now_ms;

    if (s_rtc_online) {
        DateTime now_dt = s_rtc.now();
        if (now_dt.isValid()) {
            m_live.rtc_year = now_dt.year();
            m_live.rtc_month = now_dt.month();
            m_live.rtc_day = now_dt.day();
            m_live.rtc_hour = now_dt.hour();
            m_live.rtc_minute = now_dt.minute();
            m_live.rtc_second = now_dt.second();
            m_live.rtc_valid = true;

            daylock_update(now_dt.unixtime());
            m_live.daylock_stage = daylock_get_active_stage();
            m_live.daylock_days_left = daylock_get_remaining_days();
            m_live.daylock_locked = daylock_is_motion_locked();
            return;
        }
    }

    // Fallback software RTC clock based on uptime
    m_live.rtc_year = 2026;
    m_live.rtc_month = 9;
    m_live.rtc_day = 12;
    uint32_t total_sec = (now_ms / 1000) % 86400;
    m_live.rtc_hour = (total_sec / 3600) % 24;
    m_live.rtc_minute = (total_sec % 3600) / 60;
    m_live.rtc_second = total_sec % 60;
    m_live.rtc_valid = false;

    daylock_update(1726000000UL + (now_ms / 1000));
    m_live.daylock_stage = daylock_get_active_stage();
    m_live.daylock_days_left = daylock_get_remaining_days();
    m_live.daylock_locked = daylock_is_motion_locked();
}

void MachineFSM::updateScheduler(uint32_t now_ms) {
    uint16_t current_day_min = (m_live.rtc_hour * 60) + m_live.rtc_minute;

    int8_t closest_shift = -1;
    uint32_t min_diff = 1440; // Max minutes in a day

    for (uint8_t i = 0; i < 3; ++i) {
        if (!m_config.shifts[i].enabled) continue;
        uint16_t shift_min = (m_config.shifts[i].hour * 60) + m_config.shifts[i].minute;
        uint32_t diff = 0;
        if (shift_min >= current_day_min) {
            diff = shift_min - current_day_min;
        } else {
            diff = (1440 - current_day_min) + shift_min;
        }

        if (diff < min_diff) {
            min_diff = diff;
            closest_shift = i;
        }
    }

    m_live.next_shift_idx = closest_shift;
    m_live.minutes_until_next_shift = min_diff;

    // Trigger Pre-Temperature Conditioning before shift (e.g. 30 minutes before)
    if (closest_shift >= 0 && m_config.water_temp_check_enabled) {
        if (min_diff <= m_config.pre_temp_lead_min && min_diff > 0) {
            if (m_state == STATE_IDLE) {
                ESP_LOGI(TAG, "Pre-temperature conditioning triggered for Shift %d (%u min remaining)", 
                         closest_shift + 1, min_diff);
                m_state = STATE_PRE_TEMP_REGULATION;
                m_state_entered_ms = now_ms;
            }
        }

        // Auto-start cycle at exact scheduled shift minute (diff == 0)
        if (min_diff == 0 && m_live.rtc_second <= 2) {
            if (m_state == STATE_IDLE || m_state == STATE_PRE_TEMP_REGULATION) {
                if (m_live.water_temp_ready || !m_config.water_temp_check_enabled) {
                    ESP_LOGI(TAG, "Shift %d scheduled start reached! Auto-starting cycle", closest_shift + 1);
                    startCycle();
                }
            }
        }
    }
}

void MachineFSM::readWaterTemperature(void) {
    if (m_sim_temp_active) {
        m_live.water_temp_c = m_sim_temp_c;
        m_live.water_temp_valid = true;
        float diff = fabsf(m_live.water_temp_c - m_config.water_target_temp_c);
        m_live.water_temp_ready = (diff <= m_config.water_temp_tolerance_c);
        return;
    }
    float raw_c = s_water_thermocouple.readCelsius();
    if (!isnan(raw_c) && raw_c > -10.0f && raw_c < 110.0f) {
        m_live.water_temp_c = raw_c;
        m_live.water_temp_valid = true;

        float diff = fabsf(m_live.water_temp_c - m_config.water_target_temp_c);
        m_live.water_temp_ready = (diff <= m_config.water_temp_tolerance_c);
    } else {
        m_live.water_temp_valid = false;
        m_live.water_temp_ready = false;
    }
}

void MachineFSM::updateSensors(uint32_t now_ms) {
    // 1. Scan Optocoupled Discrete Inputs (Active-LOW NC: 1 = Normal, 0 = Tripped)
    bool estop_raw = (digitalRead(PIN_INPUT_ESTOP) != 0);
    bool guard_raw = (digitalRead(PIN_INPUT_GUARD) != 0);
    bool overload_raw = (digitalRead(PIN_INPUT_OVERLOAD) != 0);

    bool v_val = false;
    m_live.estop_ok = isVirtualInputOverridden(PIN_INPUT_ESTOP, &v_val) ? v_val : estop_raw;
    m_live.guard_closed = isVirtualInputOverridden(PIN_INPUT_GUARD, &v_val) ? v_val : guard_raw;
    m_live.overload_ok = isVirtualInputOverridden(PIN_INPUT_OVERLOAD, &v_val) ? v_val : overload_raw;

    // 2. Sample Water Tank Thermocouple every 500ms
    if (now_ms - m_last_temp_read_ms >= 500) {
        m_last_temp_read_ms = now_ms;
        readWaterTemperature();
    }

    // 3. Physical Start Button Edge (Active-LOW NO: 0 = Pressed)
    static bool s_last_start_btn = true;
    bool current_start_btn = (digitalRead(PIN_INPUT_START) != 0);
    if (isVirtualInputOverridden(PIN_INPUT_START, &v_val)) {
        current_start_btn = v_val;
    }
    if (!current_start_btn && s_last_start_btn) {
        startCycle();
    }
    s_last_start_btn = current_start_btn;
}

void MachineFSM::handleOutputs(uint32_t now_ms) {
    switch (m_state) {
        case STATE_IDLE:
        case STATE_BOOT:
            relay_set(RELAY_IDX_MOTOR, false);
            relay_set(RELAY_IDX_FLOUR, false);
            relay_set(RELAY_IDX_BRAKE, false);
            relay_set(RELAY_IDX_WATER, false);
            relay_set(RELAY_IDX_TEMP, false);
            relay_set(RELAY_IDX_GREEN, false);
            relay_set(RELAY_IDX_RED, false);
            m_live.bowl_brake_active = false;
            break;

        case STATE_PRE_TEMP_REGULATION:
            relay_set(RELAY_IDX_MOTOR, false);
            relay_set(RELAY_IDX_FLOUR, false);
            relay_set(RELAY_IDX_BRAKE, false);
            relay_set(RELAY_IDX_WATER, false);
            // Regulate temperature: turn on chiller/heater if not ready
            relay_set(RELAY_IDX_TEMP, !m_live.water_temp_ready);
            relay_set(RELAY_IDX_GREEN, false);
            relay_set(RELAY_IDX_RED, !m_live.water_temp_ready); // Amber/Red warning until ready
            m_live.bowl_brake_active = false;
            break;

        case STATE_FLOUR_FEEDING:
            relay_set(RELAY_IDX_MOTOR, false);
            relay_set(RELAY_IDX_FLOUR, true); // Auger feeder motor ON
            relay_set(RELAY_IDX_BRAKE, false);
            relay_set(RELAY_IDX_WATER, false);
            relay_set(RELAY_IDX_TEMP, false);
            relay_set(RELAY_IDX_GREEN, true);
            relay_set(RELAY_IDX_RED, false);
            m_live.bowl_brake_active = false;
            break;

        case STATE_WATER_DOSING:
            relay_set(RELAY_IDX_MOTOR, false);
            relay_set(RELAY_IDX_FLOUR, false);
            relay_set(RELAY_IDX_BRAKE, false);
            relay_set(RELAY_IDX_WATER, true); // Water valve OPEN
            relay_set(RELAY_IDX_TEMP, false);
            relay_set(RELAY_IDX_GREEN, true);
            relay_set(RELAY_IDX_RED, false);
            m_live.bowl_brake_active = false;
            break;

        case STATE_MIXING:
            relay_set(RELAY_IDX_MOTOR, true); // Main Spiral Hook Contactor ON
            relay_set(RELAY_IDX_FLOUR, false);
            relay_set(RELAY_IDX_WATER, false);
            relay_set(RELAY_IDX_TEMP, false);
            relay_set(RELAY_IDX_GREEN, true);
            relay_set(RELAY_IDX_RED, false);

            // Pulsing Rotating Bowl Friction Brake
            if (m_config.bowl_brake_enabled) {
                uint32_t period_ms = (m_config.brake_on_time_sec + m_config.brake_off_time_sec) * 1000;
                if (period_ms == 0) period_ms = 10000;
                uint32_t cycle_pos = (now_ms - m_mix_start_ms) % period_ms;
                bool brake_on = (cycle_pos < (m_config.brake_on_time_sec * 1000));
                relay_set(RELAY_IDX_BRAKE, brake_on);
                m_live.bowl_brake_active = brake_on;
            } else {
                relay_set(RELAY_IDX_BRAKE, false);
                m_live.bowl_brake_active = false;
            }
            break;

        case STATE_CYCLE_COMPLETE:
            relay_set(RELAY_IDX_MOTOR, false);
            relay_set(RELAY_IDX_FLOUR, false);
            relay_set(RELAY_IDX_BRAKE, false);
            relay_set(RELAY_IDX_WATER, false);
            relay_set(RELAY_IDX_TEMP, false);
            relay_set(RELAY_IDX_GREEN, true);
            relay_set(RELAY_IDX_RED, false);
            m_live.bowl_brake_active = false;
            break;

        case STATE_PAUSED_GUARD:
            relay_set(RELAY_IDX_MOTOR, false);
            relay_set(RELAY_IDX_FLOUR, false);
            relay_set(RELAY_IDX_BRAKE, false);
            relay_set(RELAY_IDX_WATER, false);
            relay_set(RELAY_IDX_TEMP, false);
            relay_set(RELAY_IDX_GREEN, false);
            relay_set(RELAY_IDX_RED, (millis() % 1000) < 500); // Flashing Red
            m_live.bowl_brake_active = false;
            break;

        case STATE_FAULT:
        case STATE_ESTOP:
            relay_set(RELAY_IDX_MOTOR, false);
            relay_set(RELAY_IDX_FLOUR, false);
            relay_set(RELAY_IDX_BRAKE, false);
            relay_set(RELAY_IDX_WATER, false);
            relay_set(RELAY_IDX_TEMP, false);
            relay_set(RELAY_IDX_GREEN, false);
            relay_set(RELAY_IDX_RED, true);
            m_live.bowl_brake_active = false;
            break;
    }
}

void MachineFSM::update(uint32_t now_ms) {
    updateRTC(now_ms);
    updateScheduler(now_ms);
    updateSensors(now_ms);

    // Safety Interlocks
    if (!m_live.estop_ok) {
        if (m_state != STATE_ESTOP) triggerEStop();
        handleOutputs(now_ms);
        return;
    }

    if (!m_live.overload_ok) {
        if (m_state != STATE_FAULT) triggerAlarm(101);
        handleOutputs(now_ms);
        return;
    }

    // State Machine
    switch (m_state) {
        case STATE_IDLE:
            break;

        case STATE_PRE_TEMP_REGULATION:
            // Regulate until ready or scheduled time reaches
            break;

        case STATE_FLOUR_FEEDING:
            if (!m_live.guard_closed) {
                m_state = STATE_PAUSED_GUARD;
                m_state_entered_ms = now_ms;
                break;
            }

            {
                uint32_t elapsed_sec = (now_ms - m_flour_start_ms) / 1000;
                m_live.current_flour_sec = elapsed_sec;
                if (elapsed_sec >= m_config.flour_feed_time_sec) {
                    // Flour feeding complete, proceed to water dosing
                    s_flow_pulse_count = 0;
                    m_water_start_ms = now_ms;
                    m_state = STATE_WATER_DOSING;
                    m_state_entered_ms = now_ms;
                }
            }
            break;

        case STATE_WATER_DOSING:
            if (!m_live.guard_closed) {
                m_state = STATE_PAUSED_GUARD;
                m_state_entered_ms = now_ms;
                break;
            }

            if (m_config.dosing_mode == DOSING_MODE_CALIBRATED_TIME) {
                uint32_t elapsed_sec = (now_ms - m_water_start_ms) / 1000;
                if (m_calculated_water_time_sec > 0) {
                    float ratio = (float)elapsed_sec / (float)m_calculated_water_time_sec;
                    if (ratio > 1.0f) ratio = 1.0f;
                    m_live.current_water_dosed_l = ratio * m_config.water_target_liters;
                }

                if (elapsed_sec >= m_calculated_water_time_sec) {
                    m_live.total_water_liters += m_config.water_target_liters;
                    m_mix_start_ms = now_ms;
                    m_mix_elapsed_before_pause_sec = 0;
                    m_state = STATE_MIXING;
                    m_state_entered_ms = now_ms;
                }
            } else {
                // Flowmeter pulse counting
                float liters = (float)s_flow_pulse_count / (float)m_config.pulses_per_liter;
                m_live.current_water_dosed_l = liters;
                if (liters >= m_config.water_target_liters) {
                    m_live.total_water_liters += m_config.water_target_liters;
                    m_mix_start_ms = now_ms;
                    m_mix_elapsed_before_pause_sec = 0;
                    m_state = STATE_MIXING;
                    m_state_entered_ms = now_ms;
                }
            }
            break;

        case STATE_MIXING:
            if (!m_live.guard_closed) {
                uint32_t elapsed = (now_ms - m_mix_start_ms) / 1000;
                m_mix_elapsed_before_pause_sec += elapsed;
                m_state = STATE_PAUSED_GUARD;
                m_state_entered_ms = now_ms;
                break;
            }

            {
                uint32_t current_elapsed_sec = m_mix_elapsed_before_pause_sec + ((now_ms - m_mix_start_ms) / 1000);
                if (current_elapsed_sec >= m_config.mix_time_sec) {
                    m_live.remaining_mix_sec = 0;
                    m_live.batches_count++;
                    m_live.total_flour_kg += m_config.flour_bag_weight_kg;
                    m_last_cycle_duration_ms = now_ms - m_cycle_start_ms;
                    m_state = STATE_CYCLE_COMPLETE;
                    m_state_entered_ms = now_ms;
                } else {
                    m_live.remaining_mix_sec = m_config.mix_time_sec - current_elapsed_sec;
                }
            }
            break;

        case STATE_PAUSED_GUARD:
            break;

        case STATE_CYCLE_COMPLETE:
            if (now_ms - m_state_entered_ms >= 4000) {
                m_live.remaining_mix_sec = m_config.mix_time_sec;
                m_state = STATE_IDLE;
                m_state_entered_ms = now_ms;
            }
            break;

        case STATE_FAULT:
        case STATE_ESTOP:
            break;
    }

    handleOutputs(now_ms);
}

void MachineFSM::startCycle(void) {
    if (m_state == STATE_PAUSED_GUARD) {
        if (m_live.guard_closed) {
            m_mix_start_ms = millis();
            m_state = STATE_MIXING;
            m_state_entered_ms = millis();
        }
        return;
    }

    if (m_state != STATE_IDLE && m_state != STATE_PRE_TEMP_REGULATION) return;
    if (!m_live.estop_ok || !m_live.overload_ok) return;

    if (daylock_is_motion_locked()) {
        ESP_LOGW(TAG, "Cannot start: 5-Stage Day-Lock is active or expired!");
        triggerAlarm(999);
        return;
    }

    if (!m_live.guard_closed) {
        ESP_LOGW(TAG, "Cannot start: Safety bowl guard is open!");
        return;
    }

    m_cycle_start_ms = millis();

    // Calculate required water dosing duration: Time = (Liters / (L/min)) * 60s
    if (m_config.flow_rate_l_per_min > 0.1f) {
        m_calculated_water_time_sec = (uint32_t)((m_config.water_target_liters / m_config.flow_rate_l_per_min) * 60.0f);
    } else {
        m_calculated_water_time_sec = 400; // 20L @ 3L/min
    }

    // Determine initial stage: Flour Feeder or Water Dosing
    if (m_config.flour_mode == FLOUR_MODE_MOTORIZED) {
        m_flour_start_ms = millis();
        m_state = STATE_FLOUR_FEEDING;
    } else {
        s_flow_pulse_count = 0;
        m_water_start_ms = millis();
        m_state = STATE_WATER_DOSING;
    }
    m_state_entered_ms = millis();
}

void MachineFSM::stopCycle(void) {
    if (m_state == STATE_WATER_DOSING || m_state == STATE_MIXING || 
        m_state == STATE_FLOUR_FEEDING || m_state == STATE_PRE_TEMP_REGULATION ||
        m_state == STATE_PAUSED_GUARD) {
        m_state = STATE_IDLE;
        m_state_entered_ms = millis();
        m_live.remaining_mix_sec = m_config.mix_time_sec;
        handleOutputs(millis());
    }
}

void MachineFSM::overrideTempWait(void) {
    if (m_state == STATE_PRE_TEMP_REGULATION) {
        ESP_LOGI(TAG, "Operator overrode temperature pre-conditioning");
        m_live.water_temp_ready = true;
        startCycle();
    }
}

void MachineFSM::pulseWaterValve(bool active) {
    if (m_state == STATE_IDLE) relay_set(RELAY_IDX_WATER, active);
}

void MachineFSM::pulseFlourFeeder(bool active) {
    if (m_state == STATE_IDLE) relay_set(RELAY_IDX_FLOUR, active);
}

void MachineFSM::pulseBowlBrake(bool active) {
    if (m_state == STATE_IDLE) relay_set(RELAY_IDX_BRAKE, active);
}

void MachineFSM::triggerAlarm(uint16_t alarm_code) {
    m_live.last_alarm_code = alarm_code;
    m_state = STATE_FAULT;
    m_state_entered_ms = millis();
    handleOutputs(millis());
}

void MachineFSM::clearAlarm(void) {
    if (m_state == STATE_FAULT) {
        m_live.last_alarm_code = 0;
        m_state = STATE_IDLE;
        m_state_entered_ms = millis();
        m_live.remaining_mix_sec = m_config.mix_time_sec;
        handleOutputs(millis());
    }
}

void MachineFSM::triggerEStop(void) {
    m_state = STATE_ESTOP;
    m_state_entered_ms = millis();
    handleOutputs(millis());
}

void MachineFSM::resetEStop(void) {
    if (m_state == STATE_ESTOP && m_live.estop_ok) {
        m_state = STATE_IDLE;
        m_state_entered_ms = millis();
        m_live.remaining_mix_sec = m_config.mix_time_sec;
        handleOutputs(millis());
    }
}

const char* MachineFSM::getStateName(void) const {
    switch (m_state) {
        case STATE_BOOT:                return "BOOTING";
        case STATE_IDLE:                return "READY";
        case STATE_PRE_TEMP_REGULATION: return "TEMP PRE-HEAT";
        case STATE_FLOUR_FEEDING:       return "FLOUR FEEDING";
        case STATE_WATER_DOSING:        return "WATER DOSING";
        case STATE_MIXING:              return "MIXING + BRAKE";
        case STATE_CYCLE_COMPLETE:      return "COMPLETED";
        case STATE_PAUSED_GUARD:        return "GUARD OPEN";
        case STATE_FAULT:               return "OVERLOAD";
        case STATE_ESTOP:               return "E-STOP";
        default:                        return "UNKNOWN";
    }
}

void MachineFSM::setMixTimeSec(uint32_t sec) {
    if (sec >= 10 && sec <= 3600) {
        m_config.mix_time_sec = sec;
        if (m_state == STATE_IDLE) m_live.remaining_mix_sec = sec;
        saveConfigToNVS();
    }
}

void MachineFSM::setDosingMode(uint8_t mode) {
    m_config.dosing_mode = (mode == DOSING_MODE_FLOWMETER) ? DOSING_MODE_FLOWMETER : DOSING_MODE_CALIBRATED_TIME;
    saveConfigToNVS();
}

void MachineFSM::setWaterTargetLiters(float liters) {
    if (liters >= 1.0f && liters <= 200.0f) {
        m_config.water_target_liters = liters;
        saveConfigToNVS();
    }
}

void MachineFSM::setFlowRateLPerMin(float l_per_min) {
    if (l_per_min >= 0.5f && l_per_min <= 30.0f) {
        m_config.flow_rate_l_per_min = l_per_min;
        saveConfigToNVS();
    }
}

void MachineFSM::setWaterTargetTemp(float temp_c) {
    if (temp_c >= 5.0f && temp_c <= 50.0f) {
        m_config.water_target_temp_c = temp_c;
        saveConfigToNVS();
    }
}

void MachineFSM::setFlourMode(uint8_t mode) {
    m_config.flour_mode = (mode == FLOUR_MODE_MOTORIZED) ? FLOUR_MODE_MOTORIZED : FLOUR_MODE_MANUAL;
    saveConfigToNVS();
}

void MachineFSM::setFlourFeedTimeSec(uint32_t sec) {
    if (sec >= 1 && sec <= 300) {
        m_config.flour_feed_time_sec = sec;
        saveConfigToNVS();
    }
}

void MachineFSM::setBowlBrakeParams(bool enabled, uint32_t on_sec, uint32_t off_sec) {
    m_config.bowl_brake_enabled = enabled;
    if (on_sec >= 1 && on_sec <= 60) m_config.brake_on_time_sec = on_sec;
    if (off_sec >= 1 && off_sec <= 60) m_config.brake_off_time_sec = off_sec;
    saveConfigToNVS();
}

void MachineFSM::setShiftSchedule(uint8_t shift_idx, bool enabled, uint8_t hour, uint8_t minute) {
    if (shift_idx < 3 && hour < 24 && minute < 60) {
        m_config.shifts[shift_idx].enabled = enabled;
        m_config.shifts[shift_idx].hour = hour;
        m_config.shifts[shift_idx].minute = minute;
        saveConfigToNVS();
    }
}

void MachineFSM::setPreTempLeadMin(uint8_t lead_min) {
    if (lead_min >= 5 && lead_min <= 120) {
        m_config.pre_temp_lead_min = lead_min;
        saveConfigToNVS();
    }
}

void MachineFSM::saveConfigToNVS(void) {
    nvs_handle_t h;
    if (nvs_open("bakery_cfg", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_blob(h, "cfg", &m_config, sizeof(m_config));
        nvs_commit(h);
        nvs_close(h);
    }
}

void MachineFSM::loadConfigFromNVS(void) {
    nvs_handle_t h;
    if (nvs_open("bakery_cfg", NVS_READWRITE, &h) == ESP_OK) {
        size_t req_size = sizeof(m_config);
        nvs_get_blob(h, "cfg", &m_config, &req_size);
        nvs_close(h);
    }
}

void MachineFSM::setRTCDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (s_rtc_online) {
        s_rtc.adjust(DateTime(year, month, day, hour, minute, second));
    }
    m_live.rtc_year = year;
    m_live.rtc_month = month;
    m_live.rtc_day = day;
    m_live.rtc_hour = hour % 24;
    m_live.rtc_minute = minute % 60;
    m_live.rtc_second = second % 60;
    m_live.rtc_valid = true;

    // Immediately recalculate scheduler
    updateScheduler(millis());
}

void MachineFSM::adjustRTCHour(int8_t delta_hour) {
    int16_t new_h = (int16_t)m_live.rtc_hour + delta_hour;
    if (new_h < 0) new_h = 23;
    if (new_h > 23) new_h = 0;
    setRTCDateTime(m_live.rtc_year ? m_live.rtc_year : 2026,
                   m_live.rtc_month ? m_live.rtc_month : 9,
                   m_live.rtc_day ? m_live.rtc_day : 12,
                   (uint8_t)new_h, m_live.rtc_minute, m_live.rtc_second);
}

void MachineFSM::adjustRTCMinute(int8_t delta_min) {
    int16_t new_m = (int16_t)m_live.rtc_minute + delta_min;
    if (new_m < 0) new_m = 59;
    if (new_m > 59) new_m = 0;
    setRTCDateTime(m_live.rtc_year ? m_live.rtc_year : 2026,
                   m_live.rtc_month ? m_live.rtc_month : 9,
                   m_live.rtc_day ? m_live.rtc_day : 12,
                   m_live.rtc_hour, (uint8_t)new_m, 0);
}

bool MachineFSM::isDayLockLocked(void) const {
    return daylock_is_motion_locked();
}

uint8_t MachineFSM::getDayLockStage(void) const {
    return daylock_get_active_stage();
}

uint16_t MachineFSM::getDayLockRemainingDays(void) const {
    return daylock_get_remaining_days();
}

uint16_t MachineFSM::getDayLockChallenge(void) {
    uint32_t epoch = 0;
    if (s_rtc_online) {
        epoch = s_rtc.now().unixtime();
    } else {
        epoch = 1726000000UL + (millis() / 1000);
    }
    return daylock_generate_challenge(epoch, 0x5846); // Serial 0x5846 = HAFF
}

bool MachineFSM::verifyDayLockUnlock(uint16_t response_key) {
    uint16_t ch = getDayLockChallenge();
    bool ok = daylock_verify_unlock(ch, response_key);
    if (ok) {
        m_live.daylock_locked = false;
        m_live.daylock_days_left = daylock_get_remaining_days();
        m_live.daylock_stage = daylock_get_active_stage();
        if (m_state == STATE_FAULT && m_live.last_alarm_code == 999) {
            clearAlarm();
        }
    }
    return ok;
}

bool MachineFSM::applyDayLockAdminBypass(const char* pin) {
    bool ok = daylock_admin_bypass(pin);
    if (ok) {
        m_live.daylock_locked = false;
        m_live.daylock_days_left = 999;
        if (m_state == STATE_FAULT && m_live.last_alarm_code == 999) {
            clearAlarm();
        }
    }
    return ok;
}

bool MachineFSM::applyDayLockEmergency(const char* code) {
    bool ok = daylock_apply_emergency_grace(code);
    if (ok) {
        m_live.daylock_locked = false;
        m_live.daylock_days_left = daylock_get_remaining_days();
        if (m_state == STATE_FAULT && m_live.last_alarm_code == 999) {
            clearAlarm();
        }
    }
    return ok;
}

void MachineFSM::setVirtualInput(uint8_t pin, bool level) {
    if (pin < 32) {
        m_vinput_mask |= (1UL << pin);
        if (level) {
            m_vinput_vals |= (1UL << pin);
        } else {
            m_vinput_vals &= ~(1UL << pin);
        }
    }
}

void MachineFSM::clearVirtualInput(uint8_t pin) {
    if (pin < 32) {
        m_vinput_mask &= ~(1UL << pin);
    }
}

void MachineFSM::clearAllVirtualInputs(void) {
    m_vinput_mask = 0;
    m_vinput_vals = 0xFFFFFFFF;
    m_sim_temp_active = false;
}

bool MachineFSM::isVirtualInputOverridden(uint8_t pin, bool* out_val) const {
    if (pin < 32 && (m_vinput_mask & (1UL << pin))) {
        if (out_val) *out_val = (m_vinput_vals & (1UL << pin)) != 0;
        return true;
    }
    return false;
}

void MachineFSM::simulateWaterTemp(float temp_c) {
    m_sim_temp_active = true;
    m_sim_temp_c = temp_c;
    m_live.water_temp_c = temp_c;
    m_live.water_temp_valid = true;
    float diff = fabsf(m_live.water_temp_c - m_config.water_target_temp_c);
    m_live.water_temp_ready = (diff <= m_config.water_temp_tolerance_c);
}

void MachineFSM::clearSimulatedTemp(void) {
    m_sim_temp_active = false;
}

void MachineFSM::injectFlowPulses(uint32_t pulses) {
    s_flow_pulse_count += pulses;
}
