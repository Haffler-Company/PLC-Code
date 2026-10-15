# 12. Standard HMI Screen Flow & Product Brainstorming Framework

## Executive Summary
Every Haffler industrial machine adheres to a standardized 3-tier user interface architecture. This guarantees that operators trained on one machine (e.g., a UPVC welder) can immediately navigate another (e.g., a digital cutter or corner cleaner) without opening a manual.

Furthermore, this document outlines the **4-Pillar Product Brainstorming Framework** used by engineering teams to generate high-value, field-grounded features for future machine revisions.

---

## 1. Standard 3-Tier Screen Flow

```mermaid
graph TD
    A[Power On / Reboot] --> B[Tier 1: Splash & Hardware Self-Test]
    B -->|Self-Test PASS (1.5s)| C[Tier 2: Main Operating Dashboard]
    C -->|Press F5 or Touch Cog| D[Tier 3: Multi-Tab Settings]
    D -->|Tab 1| D1[Process Timers & Speeds]
    D -->|Tab 2| D2[Machine Geometry & Calibration]
    D -->|Tab 3| D3[Network & WebApp Portal]
    D -->|Tab 4| D4[Leasing Day-Lock & Security]
    D -->|Tab 5| D5[Hardware Diagnostics & I/O Map]
    D -->|Press Back / F5| C
    C -->|Alarm Triggered| E[Alarm Sheet / Modal Overlay]
    E -->|Acknowledge / Reset| C
```

---

## 2. Screen Specifications & Apple HIG Visual Guidelines

### Tier 1: Startup Splash Screen (`ui_screen_splash`)
- **Visual Composition:** Dark background `#0B0F17`, centered high-resolution Haffler emblem, and machine silhouette/icon.
- **Branding & Versioning:** Below the logo, display `"Haffler Automation"` in clean typography, followed by firmware revision (`V.1.8.2`) and build date.
- **Boot Progress Bar:** A sleek 4px rounded horizontal bar displaying hardware init stages:
  - `0% - 25%`: DS3231 RTC verification & anti-tamper monotonic check.
  - `25% - 50%`: NVS calibration data load & CRC validation.
  - `50% - 75%`: GPIO clamping & optocoupled input scan.
  - `75% - 100%`: LVGL display buffer allocation & font engine ready.
- **Duration:** Exactly $1500\text{ ms}$ on clean boot; immediately advances to Tier 2 without visual stutter.

### Tier 2: Main Operating Dashboard (`ui_screen_main`)
- **Header Bar (Height 32px):**
  - Left: Machine model name and active state pill (`IDLE`, `RUNNING`, `HELD`).
  - Center: Day-lock warning pill (only appears if remaining time $\le 48\text{ hours}$).
  - Right: Wi-Fi status icon, USB keyboard icon, and current time (`HH:MM`).
- **The Hero Metric (Center Canvas):**
  - The single most critical process variable must occupy the visual center in large, crisp digits (`ui_font_digit60` to `digit120`).
  - UPVC Cutter: Target length in millimeters (`1450.5 mm`).
  - UPVC Welder: Active heater temperature (`255 °C`).
  - Gas Filler: Argon percentage (`94.2 %`).
  - Ice Cream: Motor freeze current (`4.8 A`).
- **Footer Function Bar (Height 40px):**
  - Maps directly to front tactile keys `F1` through `F5`:
    - `[F1] JOG / MANUAL`
    - `[F2] RECIPES`
    - `[F3] COUNTER / ZERO`
    - `[F4] CLAMP TOGGLE`
    - `[F5] SETTINGS`

### Tier 3: Settings & Diagnostics (`ui_screen_settings`)
- Segmented control navigation across 5 categories: Process, Calibration, Network, Security, Diagnostics.
- **Breathing Margin:** Guaranteed minimum 16px separation between stepper buttons (`+` / `-`) and parameter titles.
- **Mathematical Centering:** Numeric values enclosed in dedicated zero-padding containers (`pad_all = 0`) and centered mathematically via `lv_obj_center()`.

---

## 3. The 4-Pillar Product Brainstorming Framework

When planning software feature additions or firmware enhancements, evaluate ideas against these four industrial value pillars:

### Pillar 1: Operator Ergonomics & Fatigue Reduction
- **Acoustic Confirmation:** Subtle, pleasant dual-frequency buzzer chirps for successful actions; distinct low-frequency buzz for invalid touches.
- **High-Contrast Night Mode:** Low-luminance color palette optimized for dim workshop environments to prevent eye strain during 12-hour shifts.
- **Single-Key Shortcuts:** Minimize screen transitions by enabling direct long-press shortcuts on front tactile keys (e.g., hold `SET` for 2 seconds to zero the counter).

### Pillar 2: Factory Efficiency & OEE (Overall Equipment Effectiveness)
- **Automatic Cycle Timer:** Measures and displays active cycle duration in milliseconds to identify pneumatic pressure drops or operator delays.
- **Shift Production Counter:** Tracks total cycles, good parts, and scrap count per shift, reset via supervisor PIN.
- **Profile Waste Optimization:** Suggests optimal cutting combinations to minimize off-cut scrap from raw 6-meter profile bars.

### Pillar 3: Predictive Maintenance & Tool Life
- **Blade & Tool Wear Counter:** Monitors total cutting or welding cycles against manufacturer thresholds; triggers amber maintenance badge at 90% lifespan.
- **Pneumatic Leak Detection:** Analyzes the time required for cylinder advance sensors to close; if cycle duration drifts $>20\%$ over baseline, warns of pneumatic pressure loss or seal wear.
- **Lubrication Schedule:** Countdown timer enforcing blade cooling mist refill intervals.

### Pillar 4: Connected Smart Factory & Fleet Management
- **Barcode / QR Job Ticket Scanner:** USB HID barcode reader support to instantly load cut lists or welding parameters directly from workshop job travelers.
- **REST & WebSocket Telemetry:** Real-time production streaming to local factory dashboards (MES / ERP integration).
- **Zero-Touch Fleet OTA:** Remote wireless firmware deployment over ArvanCloud S3 without visiting physical machine bays.
