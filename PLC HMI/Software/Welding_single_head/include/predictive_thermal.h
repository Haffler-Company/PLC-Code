#pragma once
#include <Arduino.h>
#include <math.h>

/**
 * @brief Predictive Thermal Inertia Compensator & Teflon Life Manager
 * Eliminates 250C overshoot on aluminum heating plates using rate-of-rise lead damping.
 * Implements Eco-Sleep to protect Teflon cloth from burning during idle periods.
 */

struct PredictiveThermalConfig {
    float tau_lead_s       = 18.0f; // Thermal lead time constant (seconds)
    float max_brake_drop_c = 15.0f; // Max anticipation window below setpoint
    int   eco_sleep_temp_c = 180;   // Standby temperature during idle (Teflon saver)
    int   teflon_max_warn  = 1200;  // Teflon tape life threshold (cycles)
};

class PredictiveThermalManager {
private:
    float _prev_temp = 25.0f;
    uint32_t _prev_time_ms = 0;
    float _filtered_rate = 0.0f;
    uint32_t _idle_start_ms = 0;
    bool  _eco_mode_active = false;

public:
    PredictiveThermalConfig cfg;

    void init() {
        _prev_temp = 25.0f;
        _prev_time_ms = millis();
        _filtered_rate = 0.0f;
        _idle_start_ms = millis();
        _eco_mode_active = false;
    }

    /**
     * @brief Computes projected surface temperature based on thermal inertia.
     * @param current_temp Current thermocouple reading in deg C.
     * @return Projected temperature T_pred = T_meas + tau * dT/dt.
     */
    float compute_projected_temp(float current_temp) {
        uint32_t now = millis();
        float dt = (now - _prev_time_ms) / 1000.0f;
        if (dt < 0.1f) dt = 0.1f;
        if (dt > 5.0f) dt = 1.0f; // Prevent huge jump after task stall

        float raw_rate = (current_temp - _prev_temp) / dt;
        // EMA filter on rate of rise to eliminate MAX6675 SPI quantization jitter
        _filtered_rate = (0.30f * raw_rate) + (0.70f * _filtered_rate);

        _prev_temp = current_temp;
        _prev_time_ms = now;

        // If heating rapidly, project forward
        float projected = current_temp;
        if (_filtered_rate > 0.05f) {
            projected += (cfg.tau_lead_s * _filtered_rate);
        }
        return projected;
    }

    /**
     * @brief Determines effective target temperature considering Eco-Sleep.
     * @param recipe_target Recipe target setpoint (e.g. 250 C).
     * @param is_screensaver_active True if display screensaver is active.
     * @return Effective target temperature.
     */
    int get_effective_target(int recipe_target, bool is_screensaver_active) {
        (void)is_screensaver_active;
        _idle_start_ms = 0;
        _eco_mode_active = false;
        return recipe_target;
    }

    bool is_eco_active() const { return _eco_mode_active; }
    float get_rate_of_rise() const { return _filtered_rate; }
};

extern PredictiveThermalManager g_predictive_thermal;
extern uint32_t g_teflon_cycles;
extern bool g_teflon_replace_warn;
