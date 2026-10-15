# 11. Pre-Flight Industrial Verification Checklist (30 Gates)

## Executive Summary
Prior to testing on physical machinery, flashing production boards, or committing software updates, every developer and automated agent must validate their work against the **30 Haffler Industrial Gates**. Passing these 30 gates guarantees zero electrical glitches, zero memory leaks, and deterministic machine safety.

---

## The 30 Pre-Flight Verification Gates

### Category 1: Electrical & Boot Safety (Gates 1–5)
- [ ] **Gate 01: Instant Pin Clamping:** Are all pneumatic relay outputs ($Q_0..Q_9$) explicitly driven HIGH ($1 = \text{OFF}$) in the first line of `setup()` before any driver initialization?
- [ ] **Gate 02: Bidirectional Pin Mode:** Are relay GPIOs configured as `GPIO_MODE_INPUT_OUTPUT` with internal pull-up enabled to prevent floating states during boot?
- [ ] **Gate 03: Buzzer Zero-State:** Is the buzzer pin (`GPIO 45`) held at LOW ($0\text{V}$) during power-on to eliminate audible chirps during reboot?
- [ ] **Gate 04: Brownout Detection Level:** Is the ESP32-S3 brownout detector enabled and configured for Level 7 ($2.8\text{V}$) to prevent flash corruption during voltage sags?
- [ ] **Gate 05: Output Status Mask:** Are all output state reads performed using `s_active_relay_mask` rather than `gpio_get_level()`?

---

### Category 2: Real-Time Scanning & Core Isolation (Gates 6–10)
- [ ] **Gate 06: Dual-Core Segregation:** Is the machine control state machine pinned to Core 1 and the LVGL/networking loop pinned to Core 0?
- [ ] **Gate 07: Scan Determinism:** Does the Core 1 scan loop run on a deterministic timer ($50\text{ ms} \pm 1\text{ ms}$) without blocking delays (`delay()` or `vTaskDelay` over $5\text{ms}$ prohibited)?
- [ ] **Gate 08: Mutex Timeouts:** Are all LVGL mutex acquisitions bounded by a strict $\le 50\text{ ms}$ timeout to eliminate deadlocks?
- [ ] **Gate 09: Stack High-Water Marks:** Have all FreeRTOS task stacks been audited via `uxTaskGetStackHighWaterMark()` to ensure $>1\text{ KB}$ headroom?
- [ ] **Gate 10: Zero-Allocation in Loops:** Are periodic scan routines and render callbacks free of `new`, `malloc`, `free`, and `String` allocations?

---

### Category 3: Front-Panel Keypad & Input Debounce (Gates 11–15)
- [ ] **Gate 11: Median Filtering:** Does the ADC keypad driver execute a 5-sample median filter (`readMedian5`) to reject high-frequency workshop noise?
- [ ] **Gate 12: Resistor Ladder Thresholds:** Are all 10 tactile keys tested within their calibrated voltage windows ($\pm 100\text{ mV}$)?
- [ ] **Gate 13: Optocoupler Debounce:** Are digital inputs ($I_0..I_7$) filtered through a 50ms multi-sample debounce algorithm?
- [ ] **Gate 14: Foot Pedal Safety:** Does the start pedal input require release before re-triggering (anti-repeat lock)?
- [ ] **Gate 15: Dual-Key Conflict Resolution:** Does simultaneous pressing of conflicting keys (e.g. `UP` + `DOWN`) result in safe neutral behavior?

---

### Category 4: Apple HIG Visual & LVGL Performance (Gates 16–20)
- [ ] **Gate 16: Framerate Stability:** Does the LVGL engine maintain $\ge 50\text{ FPS}$ during screen transitions?
- [ ] **Gate 17: Zero Software Shadows:** Are dynamic elements configured with `lv_obj_set_style_shadow_width = 0` to preserve GPU/CPU cycles?
- [ ] **Gate 18: Dirty-Driven Rendering:** Are label updates protected with `if (val != last_val)` guards to prevent redundant redraws?
- [ ] **Gate 19: Mathematical Centering:** Are numeric indicators enclosed in dedicated containers with `pad_all = 0` and aligned via `lv_obj_center()`?
- [ ] **Gate 20: Persian BiDi & Zero Tofu:** Are numbers isolated inside LTR containers, and does Persian text render without square box (`[]`) missing glyphs?

---

### Category 5: Embedded WebApp & Memory Health (Gates 21–25)
- [ ] **Gate 21: DRAM Ceiling:** Does internal SRAM free memory remain above $40\text{ KB}$ under peak network load?
- [ ] **Gate 22: WebApp Compression:** Is the embedded web portal compressed with GZIP to $<16\text{ KB}$ and stored in flash/ROM?
- [ ] **Gate 23: Captive Portal Probe 204:** Does the HTTP server immediately return HTTP 204 to `/generate_204` to maintain mobile Wi-Fi connection?
- [ ] **Gate 24: WebSocket Rate Throttling:** Is real-time telemetry throttled to $250\text{ ms}$ intervals to prevent TCP socket starvation?
- [ ] **Gate 25: Heap Fragmentation:** Does the largest contiguous internal DRAM block remain $>30\text{ KB}$?

---

### Category 6: Security, Day-Lock & Release Protocols (Gates 26–30)
- [ ] **Gate 26: Monotonic Clock Guard:** Does the RTC subsystem trigger `STAGE_TAMPER` if the clock rolls back $>300\text{ seconds}$?
- [ ] **Gate 27: Flash Wear Mitigation:** Are leasing runtime commits limited to hourly intervals to ensure $>10\text{ years}$ flash lifespan?
- [ ] **Gate 28: Emergency Grace Code:** Does master code `112233` provide a strictly single-use 24-hour bypass recorded in NVS?
- [ ] **Gate 29: Zero-Desktop & Direct Cloud Deployment Mandate:** Is the Desktop (`C:\Users\Haffler\Desktop`) 100% clean of release files, with `lastfile` and `firmware.bin` residing strictly in `<project>/release/` for direct ArvanCloud S3 publication?
- [ ] **Gate 30: ArvanCloud 4-Step Validation:** Does the OTA update pass HTTP 200, Content-Length match, and SHA-256 verification?
