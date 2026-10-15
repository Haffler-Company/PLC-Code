# Operations (Gas-filling)

## Owner and source protection

- The owner instruction is literal. Make the smallest safe change requested; ask one concise question if scope or safety is ambiguous.
- Preserve unexpected changes. Never overwrite, revert, recover from an archive, or clean another person's work.
- Before risky or cross-file work: run `git status --short`, inspect the relevant diff, then check the active lock in `WORKLOG.md`.
- Do not pulse physical valves (GPIO16/42), alter N.O/N.C output polarity (`vr_OutPutType`), modify touch coordinate transforms, or bypass ISR safety without an explicit request.
- Changes to `GasFilling_HMI.ino` timer ISR (`onTimer`), I2C touch initialization, display flush, or `a2_eeprom.cpp` memory layout require an explicit, bounded request.

## Owner command interpretation (Persian)

- `اصلاح بزن` means identify the direct source cause and implement every explicitly named acceptance condition within the stated screen or subsystem. Do not substitute a cosmetic workaround for the cause.
- `کامل` or `همش حل کن` means cover every listed item and closely coupled item on that screen; report any remaining unverified path explicitly rather than calling a partial result complete.
- `فقط بررسی` means source/evidence review only. `فقط پچ` means the requested edit only. Neither authorizes a build, upload, or hardware capture.
- `آپلود` authorizes a controlled build of the current tree and an app0-only flash with NVS/EEPROM preserved. Never flash a stale binary: if the source changed after its build, rotate the release version/notes and rebuild first.
- If the owner says `آپلود` while other agents have intentional dirty work, build the complete current tree only when the owner has explicitly accepted that shared scope. Otherwise stop and identify the conflicting files; never silently omit or overwrite another agent's work.
- `تا نگفتم بیلد و آپلود نزن` is an absolute hold on both operations, even after a source patch. `عکس بگیر سریع بدون حرف` means take and return the requested fresh real-HMI capture without surrounding analysis.
- `عکس بگیر` always means a fresh real-HMI SHOT of the installed firmware via `hmi_agent.py`. If a newer source patch is not uploaded, say clearly that the SHOT is the prior installed version; do not present it as proof of the source patch.
- `مشکلاتش چیه؟` means list only visible, concrete defects in the requested region. Exclude any element the owner has frozen and do not invent cosmetic work when the region is already acceptable.
- A numbered `اوکی`/`بله` accepts only that immediately preceding numbered item. `همش انجام بده` means implement every accepted item in the most recent list and no rejected or frozen item.
- A pixel/layout request requires exact old/new coordinates, a preserved anchor edge, and a check for parent clipping. Move a modal header/body as a coherent geometry system; never solve an upward move with a negative child coordinate that is clipped by its parent.
- When other agents own other pages, preserve their dirty changes and limit edits to the owner-assigned page/files. A requested status answer must distinguish source evidence, build evidence, and real-device evidence.
- `بیلد امن` means one controlled build with no upload. `پروگرم امن` means protect the current diff, build that exact tree, and upload only its fresh binary. `عکس واقعی` means a fresh HMI capture. `کم حرف` shortens only the report, never the required work or evidence.
- `حالت دقیق کامل` covers every accepted numbered item and its validation. `حالت سبک و امن` means the smallest low-cost patch with no new timer, animation, heavy object tree, or unrelated redesign.
- `فایل برای سرور بده` / `give server release files` authorizes building `firmware.bin` and generating `lastfile` via `tools/gen_lastfile.py` conforming to `tools/lastfile_standard.md`.

## Managed delegation and quality control

The primary agent is the technical manager: it converts the owner request into a bounded contract, chooses whether delegation helps, assigns ownership, reconciles results, and is solely responsible for the final patch and verdict.

| Risk / task shape | Default execution |
|---|---|
| Read-only search, bounded diagnosis, diff/doc audit | Lower-cost worker, low/medium effort |
| Independent local patch with mechanical acceptance | Balanced worker, medium effort, one file owner |
| HMI behavior, PIN/input, persistence, navigation, shared UI code | Strong manager; delegate only isolated research/test/review |
| Valve actuation, output polarity, timer ISR, GT911 touch init, release decision | Primary manager only unless the owner explicitly authorizes a bounded subtask |

- Delegate only workstreams that are independent and objectively verifiable. A small task, a shared file, an unresolved owner decision, or coupled dirty work stays with one agent; parallelism is not a goal by itself.
- One writable file has one owner at a time. Research and judge assignments are read-only. Workers may not edit `WORKLOG.md`, build, upload, capture, stage, commit, expand scope, or touch an unassigned file unless their capsule explicitly authorizes it.
- **Hard Token Caps:** Every delegated worker gets a numeric budget in its capsule: at most ~40 tool calls and ~6 files touched. A worker that hits the cap stops and returns partial findings plus a `BLOCKED` question instead of exploring on.
- Every assignment uses this compact capsule:

```text
Goal/type: <research | patch | review | judge>
Allowed scope: <screen, files, functions>
Frozen scope: <other screens/files, red lines>
Lock: <WORKLOG id or read-only>
Acceptance: <measurable behavior/geometry/scenarios>
Required evidence: <source | test | build | simulator | real HMI>
Forbidden actions: <build/upload/SHOT/WORKLOG/etc.>
Handoff: DONE | BLOCKED | CONFLICT; files touched; evidence; unverified; reusable finding or none
```

- The manager reviews every worker result against the original request and final diff; never merge a claim blindly. For behavior/security/navigation/persistence/valves/release, use an independent read-only judge (`strict-hmi-firmware-judge`).
- `REJECT` returns only the documented defect to the same owner, then re-runs the affected judge gates. After two failed retries, any file conflict, missing acceptance criterion, or new authority requirement, stop and ask the owner; do not widen scope.
- Every worker reports one reusable finding or `none`. The manager records only verified, stable, cross-task knowledge: workflow in OPERATIONS, technical fact in REFERENCE, current lock/evidence in WORKLOG. Do not record one-off debugging, speculation, chat history, or a rule already present.

## Documentation and artifacts

- Keep active documents in `docs/`; completed material goes to `docs/archive/`.
- Before any source edit, search the matching lock. Add one concise active item before a material/shared/behavioral or multi-file source change; read-only and documentation-only tasks need no lock unless coordination or file ownership requires one.
- Put every real screenshot in `artifacts/shots/` or `artifacts/screenshots/`; use no other screenshot directory.
- Generated logs, captures, and test outputs are disposable. Preserve only final evidence that is still referenced by active work.

## Scope and verification

- Read the exact affected source and its immediate callers/contracts before proposing or changing code. Match the existing style and make the smallest requested patch; do not add speculative features, abstractions, refactors, comments, or impossible-case handling.
- A missing behavior, acceptance criterion, or affected scope is `BLOCKED`; do not substitute an assumption.
- Report verification by evidence type: source inspection, automated check, simulator, or real HMI. Never claim an unrun check or treat simulator evidence as real-device proof.
- Select the narrowest validation that exercises the change. If it is blocked, unapproved, or omitted, record why; do not imply release readiness.
- Do not add or upgrade a dependency, change build configuration, or introduce an external service without explicit owner approval.
- Before handoff, inspect the patch for unintended files and run `git diff --check` for text-only changes. Never stage, commit, or bundle unrelated local work.

## HMI completion gates (Apple HIG & Siemens Standards)

- Before any HMI behavior, modal, password, input, lockout, confirmation, or navigation change, complete the intent card. Missing behavioral or security intent is `BLOCKED`, never a guessed implementation.
- For visual-only cleanup, use safe layout refinement (`safe-layout-refinement`). Objects outside its written old/new-coordinate table are frozen; no touch, navigation, or text behavior may change by implication.
- Use strict HMI review (`strict-hmi-firmware-judge`) before calling a behavioral, security, navigation, input, persistence, or release change complete. Correct a `REJECT` and re-judge; report a `BLOCKED` as uncompleted.
- Before code, record this compact intent card. Any missing behavior-affecting field is `BLOCKED`:

| Intent field | Required evidence-backed value |
|---|---|
| Identity | Screen/enum + tab + active layer/modal; channel (Glass 1/2/Dual); entry path and machine state |
| Authority/freeze | visual vs behavior/input/security/persistence; allowed objects/files; every other page, layer, shared control, and red line frozen |
| Input/data | approved touch and front-panel key combination; allowed characters/range; source of truth; Save/Cancel/reboot behavior |
| States | idle, dimension input, ready, filling, complete/hold, cancel/stop, error, recovery |
| Acceptance | exact success/error/cancel/boundary/recovery paths, required evidence level, and cross-page non-regression |

- A named screen freezes every other screen by default. Header, status bar, channel cards, shared styles/fonts, NVS, and input adapters are frozen unless named.

## Zero-Lag UI & 50 FPS Policy (Industrial Siemens Standard)

1. **Dirty-Driven Updates Only:** Never call LVGL setters (`lv_label_set_text`, `lv_bar_set_value`, `lv_obj_set_style_*`) inside 20ms render loops unless the underlying data has genuinely changed (`flag_dirty` / `if_changed` guards).
2. **Zero Software Shadows:** Never apply runtime software box-shadows (`lv_obj_set_style_shadow_width > 0`) or 2D matrix scaling transforms on dynamic/moving UI elements.
3. **Zero-Allocation in Loops:** Never allocate dynamic memory (`new`, `malloc`, `String` concatenation) inside periodic UI ticks or ISR paths. Use static stack buffers or PSRAM pools.
4. **FreeRTOS Stack & SRAM Protection:** Keep internal SRAM usage strictly below 45% (>230KB free) to guarantee DMA stability.

## Build, upload, and release

- Build root: project root with `pio run` or `scripts/build.ps1`.
- Fast upload: `scripts/upload.ps1 -Port COM8 -SkipBuild -Knowledge none`.
- Build/upload only with explicit owner approval. Keep one controlled compiler process and do not routinely clean cache.
- Prefer `-Force` / `-CleanSketch` (sketch objects only) over `-Clean` (which wipes core cache).
- Before a flashable firmware build: bump `VERSION`, auto-generate `GasFilling_HMI/version.h` and `GasFilling_HMI/fw_changelog.h`.
- Upload produces fresh binary produced by current working tree. Default preserves NVS/EEPROM. FullFlash is used only for recovery.
- Server release package: `tools/prepare_ota_package.ps1` or `python tools/gen_lastfile.py` producing `firmware.bin` and `lastfile`.
