# Metr & Cutter Documentation

This folder intentionally contains only four active documents. Do not create another active document unless it replaces one of these.

| File | Use it for |
|---|---|
| `OPERATIONS.md` | owner intent, task authority, managed delegation, HMI gates, build/upload, tools, and evidence policy |
| `REFERENCE.md` | hardware, firmware map, UI/LVGL rules, diagnostics, simulator, and machine behavior |
| `WORKLOG.md` | active locks, open work, current release state, and verification status |
| `START.md` | this routing page |

Historical plans, audits, completed work, and prior documentation are retained under `docs/archive/`. They are evidence only; never load them at task start.

## Fast route for an agent

1. Read root `AGENTS.md`.
2. Read only the route below; do not dump an entire active document, skill, diff, or `WORKLOG.md` into context.
3. Load the local skill router `.agents/skills/find-skills/SKILL.md` and select the minimum matching skill(s) from its table for this task; never bulk-load the whole catalog. This applies to the primary agent and to every delegated worker.
4. Search `WORKLOG.md` with the target screen, file, or subsystem name and read only the matching lock plus 10 surrounding lines.
5. Read the exact source, its direct caller/contract, and the narrow validation material for the requested behavior.

## Context-efficient task route

| Task | Read before action | Do not load unless needed |
|---|---|---|
| Small source fix or diagnosis | target source, direct caller, matching WORKLOG lock | full OPERATIONS/REFERENCE, unrelated diffs, archives |
| LCD/HMI visual work | HMI completion/quality sections in OPERATIONS, platform/runtime rules in REFERENCE, matching skill(s), latest requested SHOT | other screen source, old SHOTs, simulator |
| HMI behavior/input/navigation | HMI completion section, exact event/state path, matching lock and behavior skill | unrelated HMI styles or full UI module |
| Build/upload/release | Build/release section, VERSION/version/changelog, exact build tool | UI design skills, full WORKLOG history |
| Web/API issue | matching server/page source, API route and response contract | firmware UI docs unless the request changes HMI |
| F1 Wi-Fi mode or web OTA | OPERATIONS §F1 web connection and web OTA, matching WORKLOG locks, exact Wi-Fi/update owner paths | unrelated HMI pages, old OTA investigations, full web page |
| Managed/multi-agent work | OPERATIONS §Managed delegation, the target task row, matching lock | full prompt history, every worker's full output, overlapping write scopes |
| Broad review | gstack route, target docs and diffs only | complete archives or every skill |

## Token and evidence discipline

- Read the "Current release state" block of `WORKLOG.md` before any build or environment action; environment facts (X: drive, core dir, pinned platform, serial port) are already recorded there — never rediscover them by trial and error.
- Give writers exact file/function anchors and judges the new hunks inline. Exploration is the top token sink: numeric caps belong in every capsule.

- **Zero-Waste Single Agent Default:** Execute directly with single-agent discipline. Never auto-spawn subagents unless explicitly requested by the owner.
- **Forbidden Bulk-Loads:** NEVER read entire large files into context (especially `uf_images.h`, `ui_extras.h`, `admin_ui.h`, `GUI_Func.h`, `Task2.h`, `main.ino`). Always use line-search to locate line numbers first, then view strictly bounded slices (max 40-80 lines). Never view binary or font/image C-arrays.
- Use `rg`/`Select-String` first; read bounded line ranges only after locating a match. Never read `WORKLOG.md` or generated logs raw.
- Every agent starts a task through the `find-skills` router (see fast route step 3); the catalog of local skills lives only in that router, so it is never duplicated here. One task normally loads one relevant skill beyond it. Add another only when its trigger is explicit (for example behavior flow, visual polish, or release review); do not re-read a skill already active in the same task.
- Keep tool output narrow: filter compiler logs to errors, memory/flash metrics, hash, and boot result. Persist full logs in `artifacts/logs/` instead of sending them back into model context.
- Maintain a compact task ledger in the final response: changed files, evidence level, unverified item, and whether build/upload ran. Do not restate prior work.
- Before adding a new rule, search for an equivalent active rule and extend or replace it rather than duplicating it. Move completed long investigations to `docs/archive/`.
- Each worker must return one verified reusable finding or explicitly `none`. Only the primary manager promotes stable, deduplicated knowledge into OPERATIONS/REFERENCE; workers do not independently grow policy docs or WORKLOG.
- Parallel agents reduce elapsed time, not necessarily token use. Use them only for independent work; normally keep one bounded writer and, when risk requires it, one read-only judge. Reuse the same owner after a `REJECT` instead of starting duplicate audits.
- When the owner flags rapid token use, or this conversation contains several completed task batches, finish the current safe unit and write one compact `WORKLOG` checkpoint: exact scope/files, latest passed evidence, remaining gates, and frozen actions. Then explicitly ask the owner to open a new conversation with: `docs/START.md را بخوان و از lock <ID> ادامه بده`. The new conversation must not reload or summarize the old transcript.
- A checkpoint transfers only unresolved work and verified reusable knowledge. Do not carry completed narratives, rejected speculation, full worker messages, raw logs, or repeated explanations into the new conversation.

All real-HMI, simulator, and browser screenshots belong in `artifacts/screenshots/`. New captures must use a descriptive date-and-purpose filename. Do not create screenshot folders elsewhere.
