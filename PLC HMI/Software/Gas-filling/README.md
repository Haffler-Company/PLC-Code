# Gas Filling HMI

Industrial ESP32-S3 HMI for Haffler argon gas filling machines (LVGL 9.5.0, 480x320 ST7796 TFT, GT911 capacitive touch).

---

## Fast Operations

```powershell
# Check environment health & dependencies
.\scripts\doctor.ps1

# Full build and upload with knowledge logging (bumps PATCH version)
.\scripts\program.ps1 -Port COM3 -Knowledge "your change notes"

# Fast rebuild for sketch changes only (preserves core cache)
.\scripts\program.ps1 -Port COM3 -Force -Knowledge "sketch patch"

# Fast direct upload without recompiling
.\scripts\upload.ps1 -Port COM3 -SkipBuild -Knowledge none

# List release versions and rollback
.\scripts\list-versions.ps1
.\scripts\rollback.ps1 -Version 1.2.100 -Port COM3

# Serial Agent commands
python .\tools\hmi_agent.py --port COM3 cmd PING
python .\tools\hmi_agent.py --port COM3 cmd STAT
python .\tools\hmi_agent.py --port COM3 shot --out .\artifacts\shots\screen.png
python .\tools\hmi_agent.py --port COM3 smoke
```

---

## Active Documentation (The 4-Doc Suite)

All active project documentation is strictly consolidated into four core files in `docs/`:

| Document | Description |
|---|---|
| [`AGENTS.md`](file:///c:/Users/HP/Documents/GitHub/PLC/PLC%20HMI/Software/Gas-filling/AGENTS.md) | Universal agent contract, source protection, and safety redlines |
| [`docs/START.md`](file:///c:/Users/HP/Documents/GitHub/PLC/PLC%20HMI/Software/Gas-filling/docs/START.md) | Agent routing table, token discipline, and task navigation |
| [`docs/OPERATIONS.md`](file:///c:/Users/HP/Documents/GitHub/PLC/PLC%20HMI/Software/Gas-filling/docs/OPERATIONS.md) | Owner intent, delegation, HMI completion gates, and upload policy |
| [`docs/REFERENCE.md`](file:///c:/Users/HP/Documents/GitHub/PLC/PLC%20HMI/Software/Gas-filling/docs/REFERENCE.md) | Hardware pinout, process formulas, runtime facts, and stability rules |
| [`docs/WORKLOG.md`](file:///c:/Users/HP/Documents/GitHub/PLC/PLC%20HMI/Software/Gas-filling/docs/WORKLOG.md) | Active edit locks, open work items, and verified release states |

*Historical notes, legacy audits, and reference project materials are archived under [`docs/archive/`](file:///c:/Users/HP/Documents/GitHub/PLC/PLC%20HMI/Software/Gas-filling/docs/archive/).*

---

## Engineering Standards

- **Siemens Industrial Determinism:** Zero guesswork, fail-safe state machines, hardware ISR decoupling, division-by-zero guards, and strict valve output safety.
- **Apple HIG Craftsmanship:** 8px layout grid, touch targets $\ge 40\times 40\text{ px}$, semantic colors, balanced channel symmetry, clean Persian typography, and anti-slop visual gates.