# Operations

## Owner and source protection

- The owner instruction is literal. Make the smallest safe change requested; ask one concise question if scope or safety is ambiguous.
- Preserve unexpected changes. Never overwrite, revert, recover from an archive, or clean another person's work.
- Before risky or cross-file work: run `git status --short`, inspect the relevant diff, then check the active lock in `WORKLOG.md`.
- Do not touch jog release, destination, LEFT-only backlash, homing, pin polarity, E-stop, guard, cut flow, or motion behavior without an explicit request.
- `firmware/include/tasks/Task4.h`, `libraries/EspUsbHost/**`, USB plug-edge behavior, mouse reset, and NumLock behavior require an explicit “USB keyboard / کیبرد USB” request.

## Owner command interpretation (Persian)

- `اصلاح بزن` means identify the direct source cause and implement every explicitly named acceptance condition within the stated screen or subsystem. Do not substitute a cosmetic workaround for the cause.
- `کامل` or `همش حل کن` means cover every listed item and closely coupled item on that screen; report any remaining unverified path explicitly rather than calling a partial result complete.
- `فقط بررسی` means source/evidence review only. `فقط پچ` means the requested edit only. Neither authorizes a build, upload, or hardware capture.
- `آپلود` authorizes a controlled build of the current tree and an app0-only flash with NVS preserved. Never flash a stale binary: if the source changed after its build, rotate the release version/notes and rebuild first.
- If the owner says `آپلود` while other agents have intentional dirty work, build the complete current tree only when the owner has explicitly accepted that shared scope. Otherwise stop and identify the conflicting files; never silently omit or overwrite another agent's work.
- `تا نگفتم بیلد و آپلود نزن` is an absolute hold on both operations, even after a source patch. `عکس بگیر سریع بدون حرف` means take and return the requested fresh real-HMI capture without surrounding analysis.
- `عکس بگیر` always means a fresh real-HMI SHOT of the installed firmware. If a newer source patch is not uploaded, say clearly that the SHOT is the prior installed version; do not present it as proof of the source patch.
- `مشکلاتش چیه؟` means list only visible, concrete defects in the requested region. Exclude any element the owner has frozen (for example the black hardware-status strip) and do not invent cosmetic work when the region is already acceptable.
- A numbered `اوکی`/`بله` accepts only that immediately preceding numbered item. `همش انجام بده` means implement every accepted item in the most recent list and no rejected or frozen item.
- A pixel/layout request requires exact old/new coordinates, a preserved anchor edge, and a check for parent clipping. Move a modal header/body as a coherent geometry system; never solve an upward move with a negative child coordinate that is clipped by its parent.
- When other agents own other pages, preserve their dirty changes and limit edits to the owner-assigned page/files. A requested status answer must distinguish source evidence, build evidence, and real-device evidence.
- `بیلد امن` means one controlled build with no upload. `پروگرم امن` means protect the current diff, build that exact tree, and upload only its fresh binary. `عکس واقعی` means a fresh HMI capture. `کم حرف` shortens only the report, never the required work or evidence.
- `حالت دقیق کامل` covers every accepted numbered item and its validation. `حالت سبک و امن` means the smallest low-cost patch with no new timer, animation, heavy object tree, or unrelated redesign.

## Managed delegation and quality control

The primary agent is the technical manager: it converts the owner request into a bounded contract, chooses whether delegation helps, assigns ownership, reconciles results, and is solely responsible for the final patch and verdict.

| Risk / task shape | Default execution |
|---|---|
| Read-only search, bounded diagnosis, diff/doc audit | worker: `gpt-5.6-terra`, low/medium effort |
| Independent local patch with mechanical acceptance | worker: `gpt-5.6-terra`, medium effort, one file owner |
| HMI behavior, PIN/input, persistence, navigation, shared UI code | manager/judge: `gpt-5.6-sol`, medium/high; delegate only isolated research/test/review |
| Motion, E-stop, homing, pin polarity, cut flow, USB red lines, release decision | primary `gpt-5.6-sol` manager unless the owner explicitly authorizes a bounded subtask |

- Capsules carry numeric budgets: writers ≤ ~40 tool calls and ≤ 6 files; judges ≤ ~25 tool calls with hunks provided inline. Exceeding a budget returns partial results plus `BLOCKED` — never open-ended exploration; the manager splits the remainder into a fresh capsule instead of letting one worker grow.
- Delegate only workstreams that are independent and objectively verifiable. A small task, a shared file, an unresolved owner decision, or coupled dirty work stays with one agent; parallelism is not a goal by itself.
- Prefer a capable lower-cost worker for search, tests, mechanical review, or a narrow patch. Keep the strongest available model for scope, architecture, safety, conflict resolution, cross-file integration, and final judgment. Use a strong independent judge when failure impact is high. If named models are unavailable, select the nearest available capability tier with the same effort intent.
- Token-efficient default is one manager plus at most one bounded writer and one read-only judge. Add concurrency only when file ownership is disjoint and the acceptance can be verified independently; do not spawn several agents to inspect the same source or repeat the same tests.
- Owner directive (standing): the primary agent always acts as a maximally strict manager for fix tasks and runs with its two side agents — one bounded writer plus one read-only judge in parallel — whenever the fix is a real batch, not a one-line mechanical edit. The manager owns scope, reconciliation, final diff review, and every evidence gate; workers return only one verified finding or `none`. Delivery bar is artisan: smallest correct change, full scenario/state coverage, no unverified claim. Reports to the owner stay as terse as possible (`کم حرف` shortens only the report, never the required work or evidence).
- One writable file has one owner at a time. Research and judge assignments are read-only. Workers may not edit `WORKLOG.md`, build, upload, capture, stage, commit, expand scope, or touch an unassigned file unless their capsule explicitly authorizes it.
- Every agent, manager and worker alike, starts a task by loading the local skill router `.agents/skills/find-skills/SKILL.md` and picking the minimum matching skill(s) from its table; skill bodies load on demand, never in bulk. The manager selects the minimum matching project skill(s) and names them in the capsule; workers must read those skills before acting and must not load unrelated skills. Use concise technical English for worker capsules when it improves precision, while preserving the owner's Persian acceptance phrases verbatim.
- Every assignment uses this compact capsule; do not forward the full conversation or entire project docs:

```text
Goal/type: <research | patch | review | judge>
Allowed scope: <screen, files, functions>
Frozen scope: <other screens/files, red lines>
Lock: <WORKLOG id or read-only>
Skills: <minimum required skill(s) or none>
Acceptance: <measurable behavior/geometry/scenarios>
Required evidence: <source | test | build | simulator | real HMI>
Forbidden actions: <build/upload/SHOT/WORKLOG/etc.>
Handoff: DONE | BLOCKED | CONFLICT; files touched; evidence; unverified; reusable finding or none
```

- The manager reviews every worker result against the original request and final diff; never merge a claim blindly. For behavior/security/navigation/persistence/USB/OTA/release, use an independent read-only judge and the required project skill.
- If the requested approach creates a concrete safety, usability, architectural, or maintenance regression, the manager must explain the evidence and recommend a better option before implementation; never silently override the owner or blindly execute a harmful design.
- `REJECT` returns only the documented defect to the same owner, then re-runs the affected judge gates. After two failed retries, any file conflict, missing acceptance criterion, or new authority requirement, stop and ask the owner; do not widen scope.
- Every worker reports one reusable finding or `none`. The manager records only verified, stable, cross-task knowledge: workflow in OPERATIONS, technical fact in REFERENCE, current lock/evidence in WORKLOG. Do not record one-off debugging, speculation, chat history, or a rule already present.
- If a rejection exposes a repeatable failure, prefer a narrow automated guard/eval over another prose rule when it can detect the defect deterministically.
- Worker handoffs are terse: verdict, touched files, one reproducible defect or `none`, smallest correction, actual evidence, remaining unverified item, and one reusable finding or `none`. Omit narration, copied source, raw logs, and repeated policy. A `REJECT` goes back to the same writer with only the failing acceptance unless ownership must change.
- Workers never create summary, report, handoff, or state files of their own — no per-session notes, no self-documentation, no extra docs or scripts. Worker output is chat-only per the capsule. If a persistent record is genuinely required, the manager chooses the single location (normally one WORKLOG line) and keeps it to a few lines; anything else is tooling waste and gets deleted.

### Owner feedback loop

- Convert a concrete owner observation into `symptom -> exact path/state -> cause -> bounded fix -> regression guard`. Do not argue from source appearance when the owner reports a visible or operator-path failure; obtain the matching evidence level when authorized.
- Preserve unrelated pages and scenarios. Before handoff, compare the named region and direct callers against the pre-change state and reject any unexplained drift.
- Apply the owner's high bar consistently: balanced spacing, clear hierarchy and graphical icons, complete Persian in Persian mode, legible low-literacy wording, explicit feedback, and complete normal/empty/invalid/error/recovery/back states. Color or an icon alone is never the only explanation.
- When a user hint implies nearby usability gaps, inspect them within the authorized feature and include low-risk completion. Anything that changes motion, cut order, output authority, security, persistence, timeout, recovery, or another screen remains a proposal until explicitly authorized.
- When the owner reports the same class of defect again, treat the previous acceptance as insufficient: strengthen the reproducible check or visual scenario before another handoff, and record only the deduplicated stable lesson.
- When the owner sends a new message while a task or build/test pipeline is running, the running task must never be destructively aborted. The message is queued, parsed, and integrated into the manager's live plan (delegating evaluation to an assistant if needed) to ensure continuous, non-stop execution.
- Keep the owner report short: outcome, files, evidence, what still needs real browser/HMI/hardware verification, and a small test checklist only when the owner is ready to test.

## Documentation and artifacts

- Keep only these four active documents in `docs/`; completed material goes to `docs/archive/`.
- Before any source edit, search the matching lock and add one concise active item. Read-only and documentation-only tasks need no lock unless coordination or file ownership requires one.
- Put every screenshot in `artifacts/screenshots/`; use no other screenshot directory.
- Generated logs, captures, browser output, and simulator builds are disposable. Preserve only final evidence that is still referenced by active work.
- When the owner asks for the complete server/update files, deliver exactly the binary built from the current verified tree plus its matching metadata file. The metadata must name product/model, firmware version, channel, binary filename, byte size, SHA-256, compatible hardware/rule scope, build date, and concise changes; state `unsigned` unless a verified signing flow was actually used. Copy them to the requested location only after release checks pass, and verify the copied binary hash against the build output.

## Scope and verification

- Read the exact affected source and its immediate callers/contracts before proposing or changing code. Match the existing style and make the smallest requested patch; do not add speculative features, abstractions, refactors, comments, or impossible-case handling.
- A missing behavior, acceptance criterion, or affected scope is `BLOCKED`; do not substitute an assumption.
- Report verification by evidence type: source inspection, automated check, simulator, or real HMI. Never claim an unrun check or treat simulator evidence as real-device proof.
- Select the narrowest validation that exercises the change. If it is blocked, unapproved, or omitted, record why; do not imply release readiness.
- Do not run UI/browser testing or capture screenshots unless requested, except where an explicit project rule requires it (such as a requested pixel correction or `عکس واقعی`).
- Do not add or upgrade a dependency, change build configuration, or introduce an external service without explicit owner approval.
- Before handoff, inspect the patch for unintended files and run `git diff --check` for text-only changes. Never stage, commit, or bundle unrelated local work.

## Code review

- Report only introduced, reproducible issues with a clear machine/operator impact and safe correction. Do not elevate style preferences, pre-existing defects, or speculative risks as blocking findings.
- Review each behavior change against the absolute red lines, active lock, release/version rules, and task-required evidence.

## HMI completion gates

- Before any HMI behavior, modal, password, input, lockout, confirmation, or navigation change, complete the `complete-hmi-screen` contract. Missing behavioral or security intent is `BLOCKED`, never a guessed implementation.
- For visual-only cleanup, use `safe-layout-refinement`. Objects outside its written old/new-coordinate table are frozen; no touch, navigation, or text behavior may change by implication.
- Use `strict-hmi-firmware-judge` before calling a behavioral, security, navigation, input, persistence, USB, OTA, or release change complete. Correct a `REJECT` and re-judge; report a `BLOCKED` as uncompleted.
- Expand a short owner hint into a proposed complete design, but distinguish reversible design judgment from authority. The manager may choose local spacing, hierarchy, existing style helpers, and low-risk visual details; it must only propose—and not silently implement—new security, input, persistence, timeout, lockout, recovery, motion, USB, or cross-screen behavior.
- Before code, record this compact intent card. Any missing behavior-affecting field is `BLOCKED`:

| Intent field | Required evidence-backed value |
|---|---|
| Identity | F-key + source screen/enum + tab + active layer/modal; entry path and machine state |
| Authority/freeze | visual vs behavior/input/security/persistence; allowed objects/files; every other page, layer, shared control, and red line frozen |
| Input/data | approved touch/panel/keypad/USB combination; allowed characters/range; source of truth; Save/Cancel/reboot behavior |
| States | idle, focus/input, valid, invalid/empty/boundary, success, cancel/back, error, recovery; lockout/timeout only when owner-defined |
| Acceptance | exact success/error/cancel/boundary/recovery paths, required evidence level, and cross-page non-regression |

- For a password hint such as “open a password box,” first inspect the existing keyboard/keypad, masking/reveal convention, PIN source, change-PIN location, error path, and recovery. Propose the best coherent flow and missing change-PIN location; do not invent the PIN policy, persistence, attempt limit, lockout, or recovery secret.
- A named screen freezes every other screen by default. Header, status strip, F-key column, scroll rail, shared renderer/style/font, global navigation, NVS, and input adapters are frozen unless named. If a shared helper must change, list all callers/consumers and either localize the change by screen+layer guard or include every affected consumer in acceptance; otherwise stop as `BLOCKED`.
- Acceptance must include leaving the target, opening an adjacent/affected page, returning, and confirming that frozen geometry, focus, scroll, callbacks, data, and navigation did not drift. A modal must block background touch and panel focus.

## Build, upload, and release

- Firmware root: `firmware/`. Normal build: `tools/fast_firmware.ps1`.
- Build/upload only with explicit owner approval. Keep one controlled PlatformIO/esptool process and do not routinely clean `.pio/`.
- Do not mix the long workspace path and `X:` during a build cycle.
- Every firmware build must run `python tools/check_ui_perf_guard.py`; never weaken or bypass this guard.
- Before a flashable firmware build: bump `VERSION` and `firmware/include/version.h`; move F5 CUR notes to PREV, discard the older PREV block, keep only CUR/PREV, and run `python tools/validate_fw_release_notes.py`. Do not create or restore a release-note archive.
- Upload the binary produced by the current working tree only. Default is app0 and must preserve NVS.
- Full/factory flash or partition migration requires explicit approval that NVS/settings will be erased.

## Tools

| Need | Command or file |
|---|---|
| Firmware build | `tools/fast_firmware.ps1` |
| Real LCD SHOT | `python tools/hmi_screenshot.py --port <detected-HMI-port> --baud 115200 --out artifacts/screenshots/<date-purpose>.png` |
| List logic check | `python tools/list_ui_sim.py --test` |
| F1 layout check | `python tools/f1_layout_check.py --test` |
| PC simulator capture | `python tools/pc_sim_shot.py artifacts/screenshots/pc_sim_<purpose>.png` |

Resolve the current HMI port from REFERENCE/platform evidence or enumerate serial ports; never assume COM4. Open it with DTR/RTS disabled for SHOT. A real LCD SHOT, not a simulator image, is the final visual proof.

## UART service commands

Production UART0 (Serial0 / CP2102, 115200 baud) accepts two categories of commands:

**Read-only diagnostics — no key required:**

| Command | Action |
|---|---|
| `SHOT` | Capture LCD frame |
| `MEM` | Report stack watermarks |
| `PERF` | Start UI performance overlay |
| `QSTA` | Read current state (scr, lang, speed…) |
| `SCR1`/`SCR2`/`SCR3` | Navigate screen |
| `TAB1`–`TAB5` | Select settings tab |
| `LFA` / `LEN` | Switch language |

**Destructive commands — require service key:**

Format: `SVCKEY=haf2025 <CMD>` (send as one line terminated by `\n`)

| Command | Action |
|---|---|
| `SVCKEY=haf2025 RBT` | Restart ESP32 |
| `SVCKEY=haf2025 FRST` | Factory reset (NVS wipe) |

Without the correct key these return `ERR: auth required -- SVCKEY=<key> <CMD>`.

> **Agent note:** The service key is `haf2025`. Change it only via a WORKLOG-authorized source edit to `UART_SVC_KEY` in both `serial_import.h` and `screenshot.h` (or just in `screenshot.h` — the `#ifndef` guard shares it automatically).


## UI quality and performance

- Use `lcd-hardware-ui-design` for every LCD/HMI/touch/SHOT task; use `apple-grade-product-design` for visual polish.
- Use `gstack-project-review` for broad project, QA, architecture, or release work.
- Prefer existing LVGL objects and static styles. Do not add timers, large object trees, casual animations, heavy shadows, or full-screen invalidation.
- Initialize a cached state-driven indicator from the same data domain its updater reads; do not mix a pending physical source with a committed logical role.
- Persian UI remains UTF-8 with project Persian fonts. A display message uses one language only.
- Help overlays block background touch; close/header controls stay outside the scrolling body.
- For a requested visual defect, inspect the latest relevant real capture before declaring the fix finished; a simulator or source inspection alone is not visual proof.

### Visual review criteria (owner baseline)

- Before a visual patch, inspect the latest relevant real 480x320 SHOT when available. A fresh after-SHOT is required only for an explicit pixel correction or when the owner requests capture/visual approval, and only after the changed binary is installed; otherwise report hardware visual verification as pending.
- For a pixel fix, move only the explicitly named object. Treat adjacent labels, values, rows, and card boundaries as frozen; record their old/new coordinates, then verify that no row is clipped, overlapped, or pushed off its baseline.
- Preserve deliberate breathing room: no control or white value field may touch its parent card edge; keep balanced top/bottom and left/right margins, with text and units visibly separated.
- Keep the fixed five-item header outside scrollable content; scrolling must never slide cards or controls beneath the header.
- Use semantic states: neutral/idle scroll affordances are gray, active touch/scroll feedback is blue, calibration/action danger is red, and colors must not be the only state cue.
- Check Persian RTL text for joined words, correct title placement, clipping, letter spacing, mixed digits/units, and readable touch targets before declaring the screen visually sound.
- When the owner asks for a suggestion, report any additional visible imbalance or overlap found in the SHOT and label it as a recommendation, not an authorized change.
- `عکس بگیر` captures the page currently visible on the HMI. It must not send any navigation, tab, lock, unlock, or state-changing command. Navigate for a SHOT only when the owner explicitly names the destination page in that same request; wait for the UI to settle, then issue `SHOT`.

## Owner-derived HMI contract

These are accepted owner conventions for this machine. They refine terse Persian requests; they do not authorize changes outside the stated scope.

- `عکس بگیر` means capture a fresh **real-HMI SHOT** of the currently visible page when a connected device is available; save it under `artifacts/screenshots/`, inspect it, and state whether the current source was actually installed. Do not navigate or alter state unless that same request explicitly names a destination. A source patch, simulator, build, upload, and real SHOT are separate evidence levels.
- A short confirmation such as `اوکی` or `بله` approves the immediately preceding, explicitly stated scope only. Ask one short Persian question when a choice would alter motion, persistence, security, input method, or a different screen.
- Do not call a physical-panel change fully resolved until its exact operator path has been tried on the HMI. Report source-level approval honestly when build/upload or real-panel evidence is still pending.
- The black diagnostic/status strip at the top is fixed when the owner says not to touch it. Do not use its pixels for layout recovery or restyle it incidentally.
- Keep visible scroll affordance when content continues below the viewport; it may signal additional content, but must not look like accidental clipping.
- After a pixel correction or an owner-requested capture/visual approval, take the fresh SHOT only after the new binary is installed. Compare named objects against the preceding SHOT; do not navigate to a protected page merely to obtain a capture. Other visual patches do not authorize capture automatically.
- A control that has no operator action must never receive the MODE/focus marker. A touched or held actionable control must show immediate pressed/hold feedback; semantic colors remain blue for primary action, green for active/safe state, yellow for held mapping/edit selection, and red only for danger/destructive action.

### F1 web connection and web OTA

This is the owner's approved product intent for the next bounded Wi-Fi/web-update task. It does not waive the security locks named below.

- On the first F1 Wi-Fi tap, show one clear modal with `Direct / اتصال مستقیم` and `Hotspot or router / هات‌اسپات یا مودم`. Cancel leaves Wi-Fi off and changes nothing. Persist the successful selection; later short taps start/stop that saved mode instead of asking again.
- A discoverable way to change the saved mode is required. Its exact input is not yet owner-defined; long-press on the Wi-Fi control is only the recommended proposal and must not be implemented silently.
- Direct mode preserves the current SoftAP, captive page, and `http://192.168.4.1`. Hotspot/router mode performs a fresh bounded STA scan, reconnects automatically to the previously saved SSID/password when found, and otherwise opens the existing scan/password flow. Store credentials only after a successful connection; invalid password, missing SSID, cancel, retry, timeout, reboot, and return-to-F1 must have explicit non-blocking states.
- Hotspot/router mode must support both a phone hotspot and ordinary supported 2.4-GHz routers. Reproduce the reported “phone hotspot connects but modem does not” case using Wi-Fi status/reason evidence; do not hide a 5-GHz-only, hidden-network, security-mode, DHCP, BSSID/channel, or signal problem behind a generic failure. Do not pin a stale BSSID/channel when the saved SSID is expected to roam.
- After STA connection, start the same local web service on the acquired IP and show SSID, IP, `http://haffler.local`, and a QR code. Plain `haffler` and automatic browser launch from a phone-hosted hotspot are not portable OS guarantees; direct-mode captive opening remains the automatic path. The UI must say this truthfully and keep IP/QR as the reliable fallback.
- The web update page mirrors the PLC update screen with two unambiguous cards: `Digital cutter single head` and `Config`. Each card shows current/server version, checking, available/current, download, verified/installing, success, and actionable error/offline states; it never reports success before the ESP has verified and installed the image.
- The ESP—not browser JavaScript—owns HTTPS discovery, download, validation, maintenance lock, flash write, and reboot. The web page only submits an authenticated bounded request and polls bounded status; it must remain usable when internet is absent.
- Keep one persistent editable HTTPS base per program. For the cutter, the owner-supplied base is `https://ota.haffler-infi.ir/upvc%2Fdigital-cutter-single-head`; the current config base remains its compiled default until the owner changes it. Trim whitespace, prepend `https://` when the scheme is omitted, reject every non-HTTPS/invalid value, remove a trailing slash, preserve the encoded path without double encoding, and derive the existing text/version and binary endpoints as `<base>%2Flastfile` and `<base>%2Ffirmware`. Invalid or cancelled edits preserve the last valid value.
- Editable-source installation must not bypass `OTA-MANUAL-SOURCE-SAFE-SCOPE-01`, `F5-OTA-PIN-POLICY-01`, model/hardware compatibility, anti-downgrade, integrity/signature, TLS trust, rollback-health, machine-idle, or Wi-Fi ownership. Resolve those locks explicitly before calling remote installation complete.

### F3 stepper and pin mapping

- The blue Q0/Q1/Q2 capsule has two deliberate long-press zones: holding a Q text/badge opens pin mapping for that Q role with yellow feedback; holding the capsule body opens the separate stepper test-settings modal. A normal Q tap retains its existing test behavior.
- The stepper test is a round trip. Pulse count and interval are both constrained to 10..1000; one touch changes by 1, then holding for one second changes by 10 every 100 ms. The interval is the delay for each pulse, not a direct speed unit.
- Closing the stepper modal does not store data. The operator must use the F3 green Save control; the global Save color represents a real difference from the settings-entry snapshot and returns to its saved color if every change is reversed.
- Resetting temporary F3 input/output mappings requires an explicit timed confirmation. Cancellation or timeout must make no mapping change.

### Panel-key navigation

- MODE cycles only actionable controls in the visible active settings page or active modal; it skips passive labels, decorative surfaces, and controls behind an overlay. On a newly opened help/message/modal, the next MODE action belongs only to that new layer.
- For an eligible slider, roller, dropdown, arc, or numeric settings field, panel START increases and panel STOP decreases within that field's defined bounds. PIN/password fields, cut-list entry, calibration, USB behavior, and motion controls are excluded unless the owner explicitly names them.
- A visible soft keypad is one MODE destination. START enters keypad-key selection; then MODE cycles its keys and START activates the selected key. CANCEL leaves keypad-key selection before leaving its modal.
- Touching the screen exits panel-key focus so a stale marker is not left over ordinary touch interaction.
