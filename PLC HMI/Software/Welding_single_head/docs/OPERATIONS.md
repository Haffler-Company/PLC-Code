# Operations Manual & Multi-Agent Protocol — Welding Single Head

## 1. Build and Release Execution

```powershell
# Standard Firmware Build:
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run

# Flash Firmware to Hardware:
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM17
```

*Note:* After every successful build, `tools/copy_release_artifacts.py` automatically stages `release/` with:
- `firmware.bin` (OTA + esptool @ 0x10000)
- `bootloader.bin` (0x0)
- `partitions.bin` (0x8000)
- `pinmap.txt` & `README.txt`

## 2. Multi-Agent Orchestration & Fast-Path Protocol

When tasks arrive, determine execution route immediately:
- **Fast-Path Route (Single Agent):** Routine bugfixes, UI tweaks, string updates, status inspections. No subagents spawned; execute directly using bounded reads.
- **Orchestrated Route (Team):** Complex multi-screen refactors, new state machines, protocol overhauls.

### Role Separation:
1. **Primary Manager:** Owns scope, plans tasks, diffs every modified file, runs judge verification.
2. **Read-Only Researcher:** Investigates code paths and data flows without modifying files.
3. **Bounded Writer:** Edits assigned, non-overlapping files under a strict token and file budget.
4. **Strict Judge (`strict-hmi-firmware-judge`):** Evaluates completed work independently against safety red lines.

### Delegation Capsule Format:
```markdown
- Role: [Writer / Researcher]
- Target Scope: [Exact file paths]
- Frozen Scope: [Untouchable files e.g. tasks.h thermal safety]
- Acceptance Criteria: [Concrete verification criteria]
- Hard Token Budget: [Max tool calls]
```

## 3. Multi-Session Concurrency & File Claiming Protocol

When multiple chat sessions or agents work simultaneously on this repository:
1. **Pre-Flight Lock Audit:** Check `git status --short` and `## Active Locks` in `docs/WORKLOG.md` before editing any file.
2. **Claiming Scope:** For multi-step tasks, register the claim in `docs/WORKLOG.md` under `## Active Locks`:
   ```markdown
   - Lock ID: LOCK-[TaskName]
   - Session/Agent: [Session ID]
   - Claimed Scope: [src/screens.c, include/screens.h]
   - Status: IN_PROGRESS
   ```
3. **Releasing Scope:** Change status to `RELEASED` upon successful build verification.
4. **Zero-Revert Rule:** Never revert, stash, or overwrite uncommitted changes created by a parallel session.

## 4. Prompt Expansion & Industrial Technical Specification

When receiving concise user requests:
1. **Translate to Industrial English:** Map colloquial Persian terms to exact embedded firmware constructs.
2. **Resolve All 5 Architectural Dimensions:**
   - **UI Layer:** Widget layout, font, RTL base dir, dirty-driven update.
   - **Machine Process:** FreeRTOS task, timer, state machine transition in `tasks.h`.
   - **NVS Persistence:** Batched storage in `storage.h` / `setting.h`.
   - **Safety & Interlocks:** Thermal cutoff, E-Stop behavior, clamp interlocks.
   - **Verification:** PlatformIO clean build check.
3. **Clarify via Options:** If ambiguity is critical, ask with 2-3 concrete options. Otherwise, choose the safest Siemens industrial standard default.

## 5. Automated Serial Verification & Control Protocol (`agent_hmi.h` & `screenshot.h`)

The firmware includes a high-speed serial agent protocol (UART0 @ 921600 baud) for remote test automation, diagnostics, and headless CI/CD:
- `PING`: Verifies firmware responsiveness (`PONG welding_hmi_v<VERSION>`).
- `STAT`: Returns current UI screen, temperature readings, jack states, heap memory.
- `TEMP`: Reads live thermocouple readings and setpoints.
- `SHOT`: Captures live 480x320 RGB565 framebuffer in PSRAM via `shot_on_flush` and streams it over Serial in ~2.8s without blocking Core 1 UI ticks.
- `NAV <id|name>`: Navigates to target screen (1..11, 111, MENU, GENERAL, JACK, ADMIN, UPDATE, WIFI, etc.).
- `LANG <1|2|3|FA|EN|TR>`: Switches UI language dynamically.
- `SAFE`: Forces all pneumatic solenoids and SSR OFF in emergency scenarios.
- `TAP x y [ms]`: Injects virtual touch events at specific coordinates.
- `PRESS x y` / `REL`: Press/release touch injection.

### Secure Frame Format ($WELD):
All commands can be signed with HMAC-SHA256:
```text
$WELD,<SEQ>,<NONCE>,<CMD>,<PAYLOAD>*<HMAC_SIGNATURE>\r\n
```
Validated via ESP32-S3 mbedTLS hardware accelerator against `MASTER_SECRET_KEY` with sequence anti-replay checking. Full protocol specification: [`docs/SERIAL_SECURITY_PROTOCOL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/SERIAL_SECURITY_PROTOCOL.md).

## 6. Python Industrial Tooling Suite (`tools/`)

```powershell
# 1. Capture live zero-lag screenshot over serial:
python tools/hmi_screenshot.py --port COM8 --baud 921600 --out simulator/shots/live.png

# 2. Secure remote navigation & diagnostics:
python tools/hmi_commander.py --nav "MENU"
python tools/hmi_commander.py --lang "FA"
python tools/hmi_commander.py --stat
python tools/hmi_commander.py --nav 4 --shot simulator/shots/jack_screen.png

# 3. Studio-grade Persian voice report generator (TTS):
python tools/haffler_voice_reporter.py "گزارش وضعیت دستگاه و نتایج بررسی با موفقیت ثبت شد."
python tools/haffler_voice_reporter.py --file docs/WORKLOG.md --play
```

## 7. Rolling 10-Backup & Micro-Commit Operations

To protect codebase integrity, avoid memory bloat, and ensure granular 10-20 minute recovery points without blind rollbacks:

```powershell
# 1. Take a rolling backup before any corrective action (auto-keeps 10 newest, deletes oldest):
python tools/backup_manager.py create "task_description"

# 2. View all 10 active rolling backups:
python tools/backup_manager.py list

# 3. Create a Git micro-commit with automatic backup (every 10-20 min or per fix):
python tools/backup_manager.py commit "FIX: correct debounce on pedal input"

# 4. Surgically extract a single file from a backup without overwriting the workspace:
python tools/backup_manager.py restore 1 "src/screens.c"
```


