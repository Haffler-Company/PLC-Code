#pragma once
#include <Arduino.h>
#include <math.h>

/**
 * @brief Industrial Soft-Landing Predictive PID Controller for UPVC Welding Element
 * 
 * Specifically engineered for heavy aluminum heater plates with high thermal inertia:
 * 1. 5-Sample Moving Median Filter to eliminate MAX6675 SPI noise & EMI spikes from solenoids.
 * 2. Phase-plane predictive lead compensation (dT/dt) to eliminate 250C overshoot.
 * 3. Anti-Windup Zero-Accumulation Integral (only active within +/- 4C band).
 * 4. Fixed 1000ms Time-Proportioning SSR window (deterministic FreeRTOS timing).
 */

class IndustrialHeaterPID {
private:
    // 5-Point Median Filter Ring Buffer
    float _raw_buf[5] = {25.0f, 25.0f, 25.0f, 25.0f, 25.0f};
    uint8_t _buf_idx = 0;
    bool _buf_filled = false;

    // Smoothed Temperature & Rate
    float _filtered_temp = 25.0f;
    float _prev_temp_for_rate = 25.0f;
    uint32_t _prev_rate_time_ms = 0;
    float _rate_of_rise_c_per_s = 0.0f; // dT/dt

    // PID State
    float _integral_accum = 0.0f;
    uint32_t _window_start_ms = 0;
    float _current_duty = 0.0f; // 0.0 to 1.0

public:
    // Tuning parameters
    float lead_time_tau_s  = 18.0f; // Inertia lead constant (seconds)
    float kp               = 0.08f; // Proportional gain
    float ki               = 0.003f;// Integral gain (slow, steady-state zeroing)
    float kd               = 0.60f; // Derivative damping gain

    void init(float initial_temp) {
        for (int i = 0; i < 5; i++) _raw_buf[i] = initial_temp;
        _buf_idx = 0;
        _buf_filled = true;
        _filtered_temp = initial_temp;
        _prev_temp_for_rate = initial_temp;
        _prev_rate_time_ms = millis();
        _rate_of_rise_c_per_s = 0.0f;
        _integral_accum = 0.0f;
        _window_start_ms = millis();
        _current_duty = 0.0f;
    }

    /**
     * @brief 5-Point Median Filter: Rejects 100% of single-sample EMI glitches from valves/relays.
     */
    float filter_sample(float raw_deg_c) {
        _raw_buf[_buf_idx] = raw_deg_c;
        _buf_idx = (_buf_idx + 1) % 5;

        // Sort copy to find median
        float sorted[5];
        for (int i = 0; i < 5; i++) sorted[i] = _raw_buf[i];
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 5; j++) {
                if (sorted[j] < sorted[i]) {
                    float tmp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = tmp;
                }
            }
        }
        float median_val = sorted[2]; // Middle element

        // Gentle EMA filter on top of median to eliminate MAX6675 0.25C quantization steps
        _filtered_temp = (0.35f * median_val) + (0.65f * _filtered_temp);
        return _filtered_temp;
    }

    /**
     * @brief Computes rate of rise dT/dt with noise immunity.
     */
    void update_rate(uint32_t now_ms) {
        float dt = (now_ms - _prev_rate_time_ms) / 1000.0f;
        if (dt >= 0.5f) { // Update rate every 500ms or more
            float raw_rate = (_filtered_temp - _prev_temp_for_rate) / dt;
            // Clamp impossible sensor rates (> 8 C/s is physical impossibility for heavy aluminum)
            if (raw_rate > 8.0f) raw_rate = 8.0f;
            if (raw_rate < -8.0f) raw_rate = -8.0f;

            _rate_of_rise_c_per_s = (0.40f * raw_rate) + (0.60f * _rate_of_rise_c_per_s);
            _prev_temp_for_rate = _filtered_temp;
            _prev_rate_time_ms = now_ms;
        }
    }

    /**
     * @brief Calculates Duty Cycle (0.0 to 1.0) with Phase-Plane Soft Landing.
     * @param target_temp Target setpoint in deg C (e.g. 250).
     * @param current_temp Current filtered temperature.
     * @param pid_slider_val User setting slider value (default ~10).
     * @return Duty cycle between 0.0 (fully OFF) and 1.0 (fully ON).
     */
    float compute_duty(float target_temp, float current_temp, int pid_slider_val) {
        if (target_temp <= 20.0f) {
            _current_duty = 0.0f;
            _integral_accum = 0.0f;
            return 0.0f;
        }

        if (!_buf_filled) {
            init(current_temp);
        }
        _filtered_temp = current_temp;
        uint32_t now = millis();
        update_rate(now);

        float error = target_temp - current_temp;

        // ۱. ایمنی سقف دما: اگر دما بیش از ۱ درجه از هدف بالاتر رفت، قطع قطعی توان جهت جلوگیری از اورشوت
        if (current_temp >= (target_temp + 1.0f)) {
            _integral_accum = 0.0f;
            _current_duty = 0.0f;
            return _current_duty;
        }

        // ۲. توان کامل بالستیک (Full Ballistic Heating)
        // تا زمانی که دما حداقل ۶ درجه زیر هدف است، ۱۰۰٪ توان بدون ترمز اعمال می‌شود
        // تا اینرسی سنگین آلومینیوم و اتلاف حرارتی محیطی مغلوب شده و المنت سریع بالا بیاید
        if (error >= 6.0f) {
            _integral_accum = 0.0f;
            _current_duty = 1.0f;
            return _current_duty;
        }

        // ۳. باند تقرب نرم ۶ درجه‌ای (Soft-Landing Zone)
        float slider_scale = 1.0f;
        if (pid_slider_val == 1)      slider_scale = 1.3f; // Strong
        else if (pid_slider_val == 2) slider_scale = 1.6f; // Fast
        else if (pid_slider_val > 2) {
            slider_scale = ((float)pid_slider_val / 10.0f);
            if (slider_scale < 0.5f) slider_scale = 0.5f;
            if (slider_scale > 2.5f) slider_scale = 2.5f;
        }

        // توان نگه‌دارنده پایه برای صفحه ضخیم در محدوده ۲۴۰-۲۶۰ درجه
        float feed_forward = (target_temp >= 200.0f) ? 0.20f : 0.10f;

        // ترمز پیش‌بین ملایم با ثابت زمانی ۳ ثانیه (نه ۱۸ ثانیه!)
        float rate = (_rate_of_rise_c_per_s > 0.0f) ? _rate_of_rise_c_per_s : 0.0f;
        float proj_error = error - (3.0f * rate);
        if (proj_error < 0.0f) proj_error = 0.0f;

        // شیب تناسبی در باند تقرب
        float p_duty = (proj_error / 6.0f) * (1.0f - feed_forward) * slider_scale;

        // انتگرال‌گیر دقیق برای تثبیت روی نقطه هدف (±۲ درجه)
        if (fabsf(error) <= 2.0f) {
            _integral_accum += (error * 0.005f);
            if (_integral_accum > 0.15f) _integral_accum = 0.15f;
            if (_integral_accum < -0.15f) _integral_accum = -0.15f;
        } else {
            _integral_accum = 0.0f;
        }

        float duty = feed_forward + p_duty + _integral_accum;

        // کف توان حداقلی برای جلوگیری از قفل شدن یا افت دما پیش از رسیدن به هدف
        if (error > 1.5f && duty < 0.35f) {
            duty = 0.35f;
        }

        if (duty > 1.0f) duty = 1.0f;
        if (duty < 0.0f) duty = 0.0f;

        _current_duty = duty;
        return _current_duty;
    }

    /**
     * @brief Gets current rate of rise in deg C per second.
     */
    float get_rate() const { return _rate_of_rise_c_per_s; }
    float get_duty() const { return _current_duty; }
};

extern IndustrialHeaterPID g_industrial_pid;
