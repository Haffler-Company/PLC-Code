# Gas Filling Documentation

This folder contains the master engineering documentation and operational guidance for the Argon Gas Filling HMI/PLC project.

| File | Use it for |
|---|---|
| `TOUCHLESS_OPERATION.md` | Architecture & complete guide for 10-key front panel & foot pedal touchless operation |
| `HARDWARE_IO_PINOUT.md` | Master hardware pinout, electrical architecture, resistor ladders & I/O diagnostics |
| `DAY_LOCK_SECURITY.md` | 5-stage leasing day lock, offline challenge-response security & emergency unlock |
| `WEB_PORTAL_SPEC.md` | Industrial web app, SoftAP `192.168.4.1`, live telemetry & CSV production log export |
| `OTA_SERVER_PROTOCOL.md` | Dual-partition OTA update client, server lastfile standard & SHA-256 verification |
| `HELP_SYSTEM_SPEC.md` | In-HMI 6-card interactive operator guide, wiring diagrams & troubleshooting modal |
| `AGENT_PLAYBOOK_FA.md` | Comprehensive 9-pillar operational playbook for all AI agents and developers |
| `GAS_FILLING_PLC_MASTER_SPECIFICATION.md` | Master engineering directive: thermodynamic math, FSM, 180s watchdog, 10-key panel, leasing lock, web IoT, dual-OTA |
| `WIRING_DIAGRAM.md` | Industrial wiring, terminal pinout, optocoupled inputs, ADC key levels, snubbers, noise isolation, and bench acceptance |
| `USER_GUIDE_AND_TROUBLESHOOTING.md` | Complete operator guide: touchless 10-key use, web app CSV export, day-lock renewal, OTA updates, troubleshooting matrix |
| `NEXT_AGENT_HANDOVER.md` | Single-source comprehensive blueprint and status handover for the new chat session |
| `SERIAL_SECURITY_PROTOCOL.md` | Specification and developer guide for industrial secure serial protocol and remote navigation |
| `OPERATIONS.md` | Owner intent, task authority, managed delegation, HMI gates, build/upload, tools, and evidence policy |
| `REFERENCE.md` | Hardware baseline, firmware map, UI/LVGL rules, diagnostics, formulas, and machine behavior |
| `WORKLOG.md` | Active locks, open work, current release state, and verification status |
| `GAS_FILLING_HMI_KNOWLEDGE_FA.md` | Complete technical encyclopedia: thermodynamic math, circuits, FreeRTOS, LVGL 9, security & day-lock |
| `START.md` | This routing page |

Historical plans, audits, completed work, and prior documentation are retained under `docs/archive/`. They are evidence only; never load them at task start.

## Fast route for an agent

1. Read root `AGENTS.md`.
2. Equip the appropriate skill from `.agents/skills/` (use `find-skills` if unsure).
3. Read only the route below; do not dump an entire active document, skill, diff, or `WORKLOG.md` into context.
4. Search `WORKLOG.md` with the target screen, file, or subsystem name and read only the matching lock plus 10 surrounding lines.
5. Read the exact source, its direct caller/contract, and the narrow validation material for the requested behavior.

## Context-efficient task route

| Task | Read before action | Do not load unless needed |
|---|---|---|
| Touchless 10-key / Pedal operation | `TOUCHLESS_OPERATION.md`, `panel_keys.cpp`, `panel_keys.h` | full UI styles, archives |
| Hardware I/O, Pins & Wiring | `HARDWARE_IO_PINOUT.md`, `WIRING_DIAGRAM.md`, REFERENCE §Hardware | full UI module, old archives |
| Day Lock & Security Protocol | `DAY_LOCK_SECURITY.md`, `day_lock.cpp`, `day_lock.h` | UI animations, fonts |
| Web Portal & CSV Export | `WEB_PORTAL_SPEC.md`, `webportal.cpp`, `webportal.h` | display SPI drivers, archives |
| Server OTA & Release Package | `OTA_SERVER_PROTOCOL.md`, `tools/lastfile_standard.md`, `tools/gen_lastfile.py` | UI design skills, full WORKLOG |
| Interactive In-HMI Help Modal | `HELP_SYSTEM_SPEC.md`, `screens.c` (Help modal), `actions.c` | compiler internals |
| Machine sequence, FSM & process math | `GAS_FILLING_PLC_MASTER_SPECIFICATION.md`, `variables.h`, `actions.c` | UI cosmetic files, archives |
| Operator UX & troubleshooting | `USER_GUIDE_AND_TROUBLESHOOTING.md`, `webportal.cpp`, `day_lock.cpp` | compiler internals, raw traces |
| Small source fix or diagnosis | target source, direct caller, matching WORKLOG lock | full OPERATIONS/REFERENCE, unrelated diffs |
| LCD/HMI visual work | HMI completion in OPERATIONS, REFERENCE §UI, skill `lvgl-design` or `design-taste` | other screens, simulator |
| HMI behavior/input/navigation | HMI completion in OPERATIONS, skill `complete-hmi-screen` | unrelated styles |
| Managed/multi-agent work | OPERATIONS §Managed delegation, skill `agent-orchestrator`, `task-observer` | full prompt history |
| Pre-completion check | Skills `verify-before-done`, `strict-hmi-firmware-judge` | full archives or every skill |

## Token and evidence discipline

- Use `grep_search` / `rg` first; read bounded line ranges (20-60 lines) only after locating a match. Never read `WORKLOG.md` or generated logs raw.
- One task normally loads one relevant skill. Add another only when its trigger is explicit; do not re-read a skill already active in the same task.
- A manager gives each worker a short task capsule from OPERATIONS, exact files/ranges, and required evidence.
