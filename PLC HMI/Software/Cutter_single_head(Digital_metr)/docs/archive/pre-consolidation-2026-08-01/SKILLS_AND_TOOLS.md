# Skills And Tools

## Required Skills For This Project

Every agent must load these skills whenever available before doing serious work:

- `lcd-hardware-ui-design`
  - Path in this environment: `C:/Users/Haffler/.codex/skills/lcd-hardware-ui-design/SKILL.md`
  - Use for real LCD/HMI layout, LVGL screen polish, screenshots, and hardware-first UI verification.
- `apple-grade-product-design`
  - Path in this environment: `C:/Users/Haffler/.codex/skills/apple-grade-product-design/SKILL.md`
  - Use for visual critique, industrial UI polish, alignment, hierarchy, spacing, and interaction quality.
- `gstack-project-review`
  - Path in this environment: `C:/Users/Haffler/.codex/skills/gstack-project-review/SKILL.md`
  - Use when the user asks for multi-role review or broad product/engineering/QA/security/release thinking.

Required behavior:

- Read `AGENTS.md` and then follow the selective route in `docs/AGENT_START_HERE.md` before choosing tools or editing files.
- Use `lcd-hardware-ui-design` for any LCD/HMI page change.
- Use `gstack-project-review` for broad cleanup, architecture, QA, release, safety, or project-review work.
- Use `apple-grade-product-design` for visual consistency, icon sizing, colors, spacing, hierarchy, and interaction quality.
- If a required skill is unavailable, note it briefly in the handoff and continue from project docs.

Recommended skill order by task:

- Pixel/UI/screenshots: `lcd-hardware-ui-design` first, then `apple-grade-product-design`.
- Broad review/cleanup/release risk: `gstack-project-review` first.
- Performance-sensitive HMI changes: `lcd-hardware-ui-design`, then read the relevant display/UI section of `docs/TECHNICAL_KNOWLEDGE.md`.
- Build/upload/programming: read the build/release section of `docs/TECHNICAL_KNOWLEDGE.md`, then follow the safe commands below.

Local Cursor skills that describe this same project:

- `.cursor/skills/metr-cutter-agent-handoff/SKILL.md`
- `.cursor/skills/metr-cutter-hmi/SKILL.md`
- `.cursor/skills/metr-cutter-build-flash/SKILL.md`
- `.cursor/skills/metr-cutter-hmi-design/SKILL.md`
- `.cursor/skills/apple-hig-design/SKILL.md`
- `.cursor/skills/gstack*/SKILL.md`

Local Cursor rules:

- `.cursor/rules/00-project-context.mdc`
- `.cursor/rules/10-hardware-map.mdc`
- `.cursor/rules/20-lvgl-hmi.mdc`
- `.cursor/rules/30-motion-safety.mdc`
- `.cursor/rules/40-build-debug.mdc`
- `.cursor/rules/50-project-structure.mdc`
- `.cursor/rules/60-hmi-design-apple.mdc`
- `.cursor/rules/70-agent-design-loop.mdc`
- `.cursor/rules/80-gstack-team.mdc`
- `.cursor/rules/metr-cutter.mdc`

## Commands

Only run build/upload when the user explicitly allows it.

Normal incremental build (stable short Windows path, no clean):

```powershell
powershell -ExecutionPolicy Bypass -File tools/fast_firmware.ps1
```

Build once and directly upload the exact resulting factory image:

```powershell
powershell -ExecutionPolicy Bypass -File tools/fast_firmware.ps1 -Upload
```

The helper reuses `.pio`, maps the project consistently to `X:`, synchronizes changed local `file://` library files into PlatformIO's copied dependency cache, and hashes all firmware/library build inputs. If that hash matches the last successful factory image it reuses the image immediately; otherwise it performs one build and records the new hash. It refuses concurrent work, saves logs under `artifacts/logs/`, and avoids a second PlatformIO compile during upload. Run `clean` manually only after confirmed object corruption or a platform/toolchain/library-configuration change.

Check for conflicting build/upload processes before using COM4:

```powershell
Get-CimInstance Win32_Process | Where-Object { ($_.CommandLine -match '\.platformio|xtensa-esp|esptool|COM4|\.pio\\build') -and ($_.Name -notmatch 'powershell|OneApp') } | Select-Object ProcessId,Name,CommandLine
```

Fast flash:

```powershell
firmware\fast_flash.bat
```

List logic test:

```powershell
python tools/list_ui_sim.py --test
```

Real HMI screenshot:

```powershell
python tools/hmi_screenshot.py --port COM4 --baud 115200 --timeout 120 --out simulator/shots/shot.png
```

After 921600 firmware is uploaded, retest:

```powershell
python tools/hmi_screenshot.py --port COM4 --baud 921600 --timeout 30 --out simulator/shots/shot_921600_verify.png
```

## Screenshot Notes

- Use `SCR1`, `SCR2`, `SCR3` to navigate.
- Use `SHOT` to capture framebuffer.
- Always open serial with DTR/RTS disabled.
- Real SHOT images are more trustworthy than simulator screenshots for final UI claims.

## Git Safety Commands

If `.git` exists, run before risky edits:

```powershell
git status --short
git diff -- firmware/include/app/ui_extras.h firmware/include/app/GUI_Func.h
```

If `.git` is missing, initialize it before broad work after generated files are ignored:

```powershell
git init
git add README.md VERSION docs firmware/include firmware/src firmware/config firmware/platformio.ini firmware/*.bat libraries tools simulator .gitignore
git commit -m "Initial protected project snapshot"
```
