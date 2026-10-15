# Project Rules

## Communication

- Reply in Persian unless the user asks otherwise.
- Keep messages very short, direct, and practical. The user explicitly dislikes long AI explanations.
- Maintain the project persona from `AGENTS.md`: calm, serious, precise, source-protective, evidence-driven, and low-noise.
- Avoid long explanations. Give only the result, changed files, verification status, and blockers.
- Do not narrate obvious details. If working, send short progress updates only when useful.
- When working for a while, give short progress updates.
- Do not claim a real HMI visual fix is complete without a real LCD screenshot when hardware verification is in scope.

## Token And Attention Budget

- Read only the files needed for the current task; do not bulk-open large files without a reason.
- Prefer `rg` and targeted line reads over scanning whole folders.
- Summarize old context instead of repeating it to the user.
- Keep final answers compact. Put deep detail in docs, not in chat, unless the user asks.
- When work is broad, create a short checklist and execute it instead of talking through every thought.

## Documentation Rules

- All new written documentation must be in English.
- Keep docs inside `docs/`; do not scatter notes in random folders.
- Update `docs/WORK_ITEMS.md` before and after code changes:
  - Add every planned edit as `[ ]`.
  - Mark completed edits as `[x]`.
  - Leave blocked or untested items visible.
- Update `docs/TECHNICAL_KNOWLEDGE.md` whenever new technical facts are learned.
- Update this file whenever the user gives a reusable working preference or rule.
- Update `docs/SKILLS_AND_TOOLS.md` whenever a skill, tool, or test command becomes important.
- Keep docs organized by category instead of creating many small one-off files.

## Folder And Artifact Rules

- Keep the project root clean. The root should contain only source folders, workspace/config files, `README.md`, and `VERSION`.
- Put temporary outputs under `artifacts/`; do not leave logs, screenshots, captures, or generated test files in the project root.
- Use `artifacts/builds/` only for intentionally preserved firmware binaries. Do not commit generated binaries unless the user asks for a release package.
- Use `artifacts/logs/build/` for PlatformIO build logs.
- Use `artifacts/logs/upload/` for upload/flash logs.
- Use `artifacts/logs/clean/` for clean/rebuild logs.
- Use `artifacts/logs/server/` for simulator or local server stdout/stderr logs.
- Use `artifacts/shots/` for one-off root-level PNG screenshots. Use `simulator/shots/` only for curated simulator/reference images that are part of UI work.
- Use `artifacts/captures/boot/` for boot serial captures.
- Use `artifacts/captures/reference/` for reference/homing captures.
- Use `artifacts/captures/shot-raw/` for raw SHOT/SCR text dumps.
- Use `artifacts/captures/misc/` for other serial capture text files.
- Use `artifacts/test-data/` for generated or imported stress-test input files.
- Use `firmware/logs/` only for firmware-local logs that are intentionally tied to the PlatformIO project; otherwise prefer `artifacts/logs/`.
- Do not write bulky temporary files to `C:` when a workspace path can be used. Prefer `artifacts/` or a clearly named workspace-local temp folder.
- If a temporary `C:` build folder is unavoidable, delete it before finishing after copying any needed output into `artifacts/`.
- Before finishing a task, move any new root-level `build*.log`, `upload*.log`, `shot*.png`, `*capture*.txt`, or raw SHOT files into the correct artifact folder.

## Code Editing

- Prefer existing project patterns.
- For performance-sensitive LCD UI fixes, prefer moving/resizing/restyling existing objects over creating new LVGL objects.
- Do not add timers, animations, repeated full-screen invalidation, heavy shadows, or large object loops unless there is a clear technical need.
- If a UI fix needs new objects or extra runtime work, state the performance risk before implementing.
- Prefer `firmware/include/app/ui_extras.h` for runtime UI chrome instead of rewriting generated EEZ files.
- Avoid casual edits to `firmware/src/ui/screens.c`; it is generated and contains UTF-8/Persian strings.
- Use `apply_patch` for manual edits.
- Do not revert user changes.
- Keep changes narrow and related to the request.
- Do not use destructive git commands.
- Never replace active source files from an archive, RAR, generated folder, or old backup unless the user explicitly asks and a current backup/commit exists.
- Before large edits, recovery work, or cross-file changes, make sure there is a git commit or a clearly named backup under `artifacts/backups/`.
- If you find unexpected changes, assume another agent or the user made them. Do not overwrite them; inspect, merge carefully, and document the decision.

## Git And Source-Loss Prevention

- This project must have local git history. If `.git` is missing, stop broad code work and initialize/repair it before risky edits.
- Before any build/upload after code changes, check `git status --short` when git exists.
- Programming/upload is allowed to build and flash only the current working tree. It must never restore older source as a side effect.
- If the user says "program/upload", first preserve or review the current diff, then build from `firmware/`; do not fetch old files from backups, archives, or generated folders.
- Commit meaningful stable checkpoints after verified work when the user asks for broad cleanup, release, or safety work.
- Never use `git reset --hard`, `git checkout --`, or mass revert commands unless the user explicitly requests that exact operation.
- Do not copy older source trees over `firmware/include/app/ui_extras.h` or other active files. This previously caused severe source loss.
- If recovery is needed, compare files first and preserve both versions under `artifacts/backups/` before choosing.

## Multi-Agent Coordination

- The user may run multiple agents at the same time.
- Before editing, read `docs/WORK_ITEMS.md` and check whether another agent is already working on the same area.
- For substantial edits, create or update `docs/EDIT_LOCK_FA.md` with the file names and intended scope. Keep the lock short and remove/update it when done.
- Never run build/upload while another agent is building, uploading, taking SHOT, or using COM4.
- If a conflict is detected, stop writing, report the conflicting files, and merge deliberately instead of overwriting.
- Keep changes small enough that another agent can understand and review them quickly.

## Build, Upload, And Hardware

- Do not build unless explicitly requested.
- Do not upload/flash unless explicitly requested.
- Only one build/upload process may run at a time. Kill stale PlatformIO/esptool processes only when they are clearly from this project and blocking COM4 or `.pio/build`.
- On Windows, PlatformIO parallel builds can corrupt or lock `.pio/build`. If that happens, stop stale processes, clean, then run a single controlled build.
- Use UTF-8 output for esptool on Persian Windows consoles: set `PYTHONIOENCODING=utf-8` before direct flash.
- When screenshotting real hardware, use COM4 with DTR/RTS disabled.
- After upload or power reset, wait about 20 seconds before taking a SHOT.
- If UI does not change after upload, a full power-cycle may be required.

## HMI Design

- Target resolution is 480x320.
- Treat touch targets as industrial controls; keep them large and visually clear.
- For pixel-level layout requests, make the exact requested move first.
- Pixel-level review items must be implementation-ready: name the object, direction, pixel value, current evidence, and validation method.
- Do not write vague visual tasks such as "review title card", "align better", or "make nicer". Convert them into exact actions such as "move the cut-list title white card 3 px left and shorten its top edge by 3 px".
- Before writing a visual correction list, inspect the latest source and latest SHOT/work items so already-fixed items are not repeated.
- Before editing any pixel-level item, write the coordinate interpretation for yourself: current `x/y/w/h`, target `x/y/w/h`, and which edge is anchored. This prevents reversing right/left or shortening the wrong edge.
- Phrases such as "shorten from the top" or "make top smaller" must change the top edge, not only the object height. Height-only changes usually remove pixels from the bottom in LVGL.
- The user's newest direction overrides an older assistant checklist. Do not execute an old "left" item when the user says "right".
- If the exact pixel correction cannot be known from current evidence, write "needs fresh SHOT before correction" instead of guessing.
- Use real SHOT images for final visual judgment.
- Persian and English layouts must match unless the user requests different locations.
- Avoid clipped text, hidden glyphs, and labels that drift between languages.
- Avoid nested card-like UI and unnecessary decoration.
- Keep icon sizes, button colors, radius, error messages, and typography consistent across pages unless the user asks for a specific exception.
- Error messages must use only the active language. Do not show Persian and English together on the LCD.
- Help dialogs must be modal, block touches behind them, and keep their close button/header outside the scrolling body.

## Safety

- This is a real cutter/motion machine.
- Do not change pins, direction polarity, homing behavior, or motion safety without explicit user request and hardware test notes.
- E-stop, guard, limit, and cut confirmation behavior must stay conservative.
