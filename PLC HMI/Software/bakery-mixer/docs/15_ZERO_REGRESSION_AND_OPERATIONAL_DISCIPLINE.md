# 15. Zero-Regression Engineering & Operational Discipline

## Executive Summary
Developing industrial firmware requires uncompromising discipline. Introducing regressions into previously verified machine functions wastes factory hours, damages mechanical assemblies, and frustrates machine owners. This document establishes the **Zero-Regression Code of Conduct** and the operational mechanics that guarantee forward-only progress.

---

## 1. The Anti-Frustration Operational Laws

Every engineer and AI agent operating in the Haffler automation ecosystem must abide by the following operational principles:

### Law 1: Absolute Prohibition of Guesswork (Zero Guesswork)
- Never guess a GPIO pin number, an ADC threshold, a state machine enum, or a timer constant.
- Always inspect the exact definition in the authoritative header file (e.g. `include/haffler_pins.h` or `include/machine_fsm.h`) before writing or modifying code.

### Law 2: The Non-Regression Mandate (Never Break Stabilized Code)
- A fix to one feature (e.g. tuning the saw motor timer) must NEVER alter, degrade, or touch unrelated code paths (e.g. Jog keypad handler or day-lock tracking).
- If an existing subsystem works, leave its structure intact. Make the smallest surgical change necessary.

### Law 3: Rolling Micro-Commits (Every ~3 Builds / Milestones)
- Do not accumulate hundreds of uncommitted lines of code.
- Commit working milestones every 3 builds or after stabilizing any individual screen/feature using structured semantic commit messages:
  - `feat(cutter): implement unilateral left-only backlash compensation`
  - `fix(keypad): add 5-sample median filter to eliminate ADC bounce`
  - `refactor(ui): mathematically center target length display`

### Law 4: Repository Hygiene & Lightweight Footprint
- Never scatter scratch scripts, intermediate binaries, or raw dumps across root directories.
- Intermediate build artifacts belong strictly inside `.pio/`.
- Maintain a clean, readable project structure that can be understood at a glance.

### Law 5: Continuous Knowledge Documentation
- Every technical quirk, hardware discovery, or calibration insight must be recorded immediately in the project documentation.
- Do not let hard-won engineering lessons disappear into transient chat transcripts.

### Law 6: Root-Cause Forward-Fixing (No Blind Reverts)
- When a bug or compiler error occurs, do NOT execute a blind `git reset --hard` that wipes out valuable work.
- Trace the compilation error or logic fault to its exact root cause, inspect the diff, and apply a surgical forward fix.

### Law 7: Absolute Hardware Flashing Prohibition
- **The Ultimate Red Line:** Flashing physical hardware without explicit authorization risks unexpected machine motion in a factory environment.
- An agent must NEVER run `pio run -t upload` or `esptool.py` unless the owner explicitly writes:
  - **"فلش کن"** (Flash it)
  - **"پروگرم کن"** (Program it)
  - **"آپلود کن"** (Upload it)

### Law 8: Strict Version Bumping
- Every code change or build MUST increment the patch version (`V.1.8.0` $\rightarrow$ `V.1.8.1`).
- The version string must remain 100% synchronized across `VERSION`, `include/version.h`, and `include/fw_changelog.h`.
- Each changelog item must be concise ($\le 13\text{ words}$).

### Law 9: Zero-Desktop Release (release/ folder) Mandate
- When requested to prepare server release files (*"فایل برای سرور بده"*), place ONLY two files in the project's internal release/ folder (NEVER on Desktop):
  1. `release/lastfile (روی سرور ابری ota-plc)`
  2. `release/firmware.bin (روی سرور ابری ota-plc)`
- Under no circumstances may any file be placed on C:\Users\Haffler\Desktop.

### Law 10: Mandatory 4-Step Cloud Verification
- Do not declare an OTA deployment finished until `tools/verify_ota.py` confirms HTTP 200, Content-Length match, and SHA-256 integrity against the live ArvanCloud CDN.

---

## 2. Automated Rolling Backup Engine (`backup_manager.py`)

To ensure that no code is ever lost due to accidental overwrites or unexpected tool behavior, every project includes `tools/backup_manager.py`.

### How It Operates:
1. **Rolling 10-Slot Ring Buffer:** Maintains up to 10 timestamped `.zip` archives of the `src/` and `include/` trees inside the `.backups/` directory. When an 11th backup is generated, the oldest archive is automatically pruned.
2. **One-Command Git Checkpoint:** Automatically checks `git status`, stages tracked files, increments the patch version, and creates a clean local Git commit:
   ```powershell
   python tools/backup_manager.py "Stabilized keypad debouncing and FSM state transitions"
   ```
3. **Instant Safe Rollback:** If a refactor goes awry, the script can restore the exact prior milestone with:
   ```powershell
   python tools/backup_manager.py --restore-latest
   ```

By coupling automated rolling backups with strict forward-fix discipline, development progresses rapidly with 0% risk of catastrophic regression.
