# Industrial Bakery Dough Divider HMI & PLC System

Industrial ESP32-S3 Dual-Core Firmware and LVGL HMI for Automatic Bakery Dough Divider and Portioner (Chone Machine). Designed to strict Siemens industrial safety determinism and Apple Human Interface Guidelines (HIG).

---

## 📋 System Overview & Technical Specifications

- **Central Controller:** Espressif ESP32-S3 DevKitC-1 (240 MHz Dual-Core, 16MB Quad SPI Flash, Quad SPI PSRAM)
- **User Interface:** 3.5" 480x320 SPI TFT Display (ILI9341 / ST7796) with Capacitive Touch (`bb_captouch` over I2C: SDA 8, SCL 9, INT 40, RST 39)
- **Real-Time Clock:** Maxim DS3231 Precision Industrial RTC (I2C) with battery backup
- **Thermal Instrumentation:** Dual K-Type Thermocouple channels via MAX6675 SPI interfaces
- **Motion & Inverter Control:**
  - Piston Motor Relay (`PISTON_MOTOR`, GPIO 36 / Q7) - Cutting and ejection stroke
  - Funnel Feeding Motor Relay (`FUNNELE_MOTOR`, GPIO 42 / Q1) - Dough feeding stroke
  - Inverter Forward Command (`INVERTER_FORWARD`, GPIO 39 / Q2)
  - Inverter Reverse Command (`INVERTER_REVERSE`, GPIO 48 / Q3) with mandatory 100ms dead-time interlock
- **Position Feedback:** Inductive Proximity Sensors for Piston Home (`PISTON_SENSOR`, GPIO 7 / I7) and Funnel Home (`FUNNELE_SENSOR`, GPIO 3 / I6)
- **Operator Keypads:** Analog Resistor Ladder Keypads on ADC1 (`KEY_F` GPIO 1, `KEY_M` GPIO 2) for F1..F5 and M1..M5
- **Acoustic Feedback:** Warning and event buzzer on GPIO 45

---

## 📁 Repository Directory Architecture

```text
Bakery/
├── AGENTS.md                  # Master Engineering Mandates & Anti-Hallucination Guardrails
├── GEMINI.md                  # AI Model Universal Rule Pointer
├── CLAUDE.md                  # Anthropic Assistant Development Guidelines
├── .cursorrules               # Cursor IDE Surgical Guidelines
├── platformio.ini             # PlatformIO Industrial Build Configuration
├── partitions.csv             # 16MB Partition Layout (Dual 7MB OTA Slots + SPIFFS)
├── version.json               # Semantic Version Metadata
├── VERSION                    # Single-line Version Tag
├── HARDWARE_LOCK.md           # Hardware Connection & Upload Authorization Status
├── README.md                  # System Overview & Getting Started Guide
├── PROJECT_INFO.md            # Central Project Orientation Metadata
├── docs/                      # Comprehensive Industrial Engineering Documentation
│   ├── START.md               # Master Onboarding & Routing Guide
│   ├── AGENT_PLAYBOOK.md      # 9-Pillar Operational Governance Playbook
│   ├── BAKERY_HMI_KNOWLEDGE.md# Master Machine Technical Encyclopedia
│   ├── BAKERY_CYCLE_FSM.md    # Finite State Machine & Timing Specifications
│   ├── HARDWARE_IO_PINOUT.md  # Detailed Pinout, Electrical & ADC Ladder Specs
│   ├── REFERENCE.md           # Quick Engineering Cheat Sheet & NVS Key Map
│   ├── OPERATOR_VOCABULARY.md # Industrial Bakery Workshop Glossary (Persian/English)
│   ├── UI_SCREEN_INVENTORY.md # UI Screens, Widgets & Event Binding Inventory
│   ├── ARCHITECTURE_DECISIONS.md # Architecture Decision Records (ADRs)
│   ├── KNOWN_ISSUES_AND_DEBT.md  # Technical Debt & Vulnerability Analysis
│   ├── TODO_ROADMAP.md        # Feature & Architecture Implementation Roadmap
│   ├── INTER_FILE_API_CONTRACT.md# Data Flow Contracts Between Files
│   ├── WORKLOG.md             # Engineering History & Active Session Locks
│   ├── SPEC_FIRMWARE_CORE_AND_FSM.md # Implementation Blueprint: Core FSM & Watchdogs
│   ├── SPEC_WIFI_AND_WEBAPP.md # Implementation Blueprint: Embedded Mobile WebApp
│   ├── SPEC_WEB_OTA_UPDATE.md # Implementation Blueprint: Browser-Based OTA Flashing
│   ├── SPEC_HW_DIAGNOSTICS_SCREEN.md # Implementation Blueprint: Technician I/O Test Screen
│   ├── SPEC_SHIFT_STATISTICS_DS3231.md # Implementation Blueprint: Shift Logging & DS3231
│   └── SPEC_AUDIO_MELODIES_PWM.md # Implementation Blueprint: Multi-Tone PWM Audio
├── tools/                     # Automation, Backup & Build Tools
│   ├── backup_manager.py      # Rolling 10-Backup Manager & Git Micro-Commits
│   ├── backup_manager.ps1     # PowerShell Quick Access Wrapper
│   └── gen_build_time.py      # Pre-build Timestamp & Version Header Generator
└── V.1.1.0/
    └── ChoneProject/          # Core Firmware Source & Generated UI
        ├── ChoneProject.ino   # Entry Point, Peripherals Init & FreeRTOS Tasks
        ├── defines.h          # Clean Pinout, Timing Constants & Logic Levels
        ├── general.h          # Low-Level Motor Control & Debounced Button Reading
        ├── tasks.h            # FreeRTOS Real-Time Tasks
        ├── storage.h          # NVS Non-Volatile Persistence Wrapper
        ├── lvgl_function.h    # UI Event Handlers & Hardware Binding
        ├── lv_conf.h          # LVGL 8.3 Core Configuration
        └── ui.c / ui.h        # Screen Implementations
```

---

## 🛠️ Build & Verification Guide

### Compile Locally via PlatformIO:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

### Create a Rolling Backup:
```powershell
python tools/backup_manager.py create "description"
```

### Commit a Discrete Change with Automated Backup:
```powershell
python tools/backup_manager.py commit "Description of verified discrete fix"
```

> [!CAUTION]
> **Strict Upload Authority:** Flashing firmware directly to hardware (`pio run -t upload`) is strictly forbidden without explicit and direct instruction from the project owner.
