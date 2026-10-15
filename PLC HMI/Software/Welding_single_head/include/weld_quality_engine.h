#pragma once
#include <Arduino.h>
#include <math.h>

/**
 * @brief Industrial Weld Quality & DVS 2207-1 Compliance Engine
 * Calculates real-time composite joint strength index (0-100%)
 * for UPVC profile corner welding on ESP32-S3.
 */

struct WeldTelemetry {
    float   score_pct          = 100.0f;
    float   temp_fidelity_pct  = 100.0f;
    float   melt_time_pct      = 100.0f;
    float   cooling_hold_pct   = 100.0f;
    float   ambient_factor_pct = 100.0f;
    char    grade[16]          = "A+";
    char    fault_reason[48]   = "OPTIMAL";
    bool    passed             = true;
    uint32_t last_calc_ms      = 0;
};

class WeldQualityEngine {
public:
    static WeldTelemetry calculate(
        float target_temp_c,
        float avg_melt_temp_c,
        float min_melt_temp_c,
        float max_melt_temp_c,
        float target_melt_s,
        float actual_melt_s,
        float target_cool_s,
        float actual_cool_s,
        float ambient_temp_c,
        bool  emergency_hit
    ) {
        WeldTelemetry res;
        res.last_calc_ms = millis();

        // 1. Critical Safety Interlocks
        if (emergency_hit) {
            res.score_pct = 0.0f;
            res.passed = false;
            strncpy(res.grade, "REJECT", sizeof(res.grade));
            strncpy(res.fault_reason, "EMERGENCY_ABORT", sizeof(res.fault_reason));
            return res;
        }

        float min_safe_temp = (target_temp_c > 150.0f) ? (target_temp_c - 15.0f) : 215.0f;
        if (min_melt_temp_c < min_safe_temp) {
            res.score_pct = 15.0f;
            res.passed = false;
            strncpy(res.grade, "REJECT", sizeof(res.grade));
            strncpy(res.fault_reason, "COLD_WELD_CRITICAL", sizeof(res.fault_reason));
            return res;
        }

        float max_safe_temp = (target_temp_c > 150.0f) ? (target_temp_c + 18.0f) : 268.0f;
        if (max_melt_temp_c > max_safe_temp || max_melt_temp_c > 275.0f) {
            res.score_pct = 20.0f;
            res.passed = false;
            strncpy(res.grade, "REJECT", sizeof(res.grade));
            strncpy(res.fault_reason, "PVC_THERMAL_BURNING", sizeof(res.fault_reason));
            return res;
        }

        // 2. Temperature Fidelity Score (Weight 40%)
        // Standard DVS 2207 tolerance: +/- 3.5 deg C
        float temp_diff = fabsf(avg_melt_temp_c - target_temp_c);
        float s_temp = expf(-(temp_diff * temp_diff) / (2.0f * 3.5f * 3.5f));
        res.temp_fidelity_pct = s_temp * 100.0f;

        // 3. Melt Duration Accuracy (Weight 30%)
        // Undermelt is punished quadratically; overmelt linearly
        float melt_diff = actual_melt_s - target_melt_s;
        float s_melt = 1.0f;
        if (melt_diff < 0.0f) {
            s_melt = expf(-powf(melt_diff / 2.5f, 2.0f));
        } else {
            s_melt = expf(-melt_diff / 6.0f);
        }
        res.melt_time_pct = s_melt * 100.0f;

        // 4. Cooling & Fusion Duration (Weight 20%)
        float cool_ratio = actual_cool_s / fmaxf(1.0f, target_cool_s);
        float s_cool = fminf(1.0f, powf(cool_ratio, 1.2f));
        res.cooling_hold_pct = s_cool * 100.0f;

        // 5. Ambient Temperature Thermal Gradient (Weight 10%)
        // Optimal workshop ambient: 20C - 26C. Winter drop causes rapid heat-sink.
        float amb_dist = fabsf(ambient_temp_c - 23.0f);
        float s_amb = fmaxf(0.5f, 1.0f - (amb_dist / 35.0f));
        res.ambient_factor_pct = s_amb * 100.0f;

        // Composite Weighted Index
        float total = (0.40f * s_temp) + (0.30f * s_melt) + (0.20f * s_cool) + (0.10f * s_amb);
        res.score_pct = fmaxf(0.0f, fminf(100.0f, total * 100.0f));

        // Grade classification
        if (res.score_pct >= 92.0f) {
            strncpy(res.grade, "A+ (Superior)", sizeof(res.grade));
            strncpy(res.fault_reason, "OPTIMAL_FUSION", sizeof(res.fault_reason));
            res.passed = true;
        } else if (res.score_pct >= 82.0f) {
            strncpy(res.grade, "A (Standard)", sizeof(res.grade));
            strncpy(res.fault_reason, "PASS_TOLERANCE", sizeof(res.fault_reason));
            res.passed = true;
        } else if (res.score_pct >= 70.0f) {
            strncpy(res.grade, "B (Warning)", sizeof(res.grade));
            strncpy(res.fault_reason, "MINOR_DEVIATION", sizeof(res.fault_reason));
            res.passed = true;
        } else {
            strncpy(res.grade, "REJECT", sizeof(res.grade));
            strncpy(res.fault_reason, "WEAK_JOINT_RISK", sizeof(res.fault_reason));
            res.passed = false;
        }

        return res;
    }
};

extern WeldTelemetry g_last_weld_telemetry;
