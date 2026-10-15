# Agent Start Here

Read the root `AGENTS.md` first. It is the binding safety and workflow contract.

## Open only what the task needs

| Task | Required context |
|---|---|
| Any firmware change | `docs/PROJECT_NOTES.md`, relevant source, and the matching active item in `docs/WORK_ITEMS.md` |
| Build or upload | `docs/PROJECT_RULES.md`, `tools/fast_firmware.ps1`, and version/release-note files |
| Motion, pins, safety, homing | `docs/PROJECT_NOTES.md`, affected task source, and `docs/SCENARIO_FA.md` |
| LCD/HMI/touch | matching HMI skill, latest real SHOT if available, affected source, and active work item |
| Bug investigation | affected source plus the relevant section of `docs/TECHNICAL_KNOWLEDGE.md` |
| Simulator work | `docs/SIMULATOR_FA.md` and the relevant simulator/tool only |

Do not bulk-read `WORK_ITEMS.md`, changelogs, old audit reports, screenshots, or archive material. Search by the feature name and open the smallest matching section.

## Stable project facts

- ESP32-S3, Arduino/PlatformIO, LVGL 9.4, 480x320 landscape HMI.
- Firmware root: `firmware/`; normal build helper: `tools/fast_firmware.ps1`.
- Main runtime UI: `firmware/include/app/ui_extras.h`; UI/touch: `firmware/include/app/GUI_Func.h`; UI loop: `firmware/include/tasks/Task2.h`.
- USB keyboard files (`Task4.h`, `libraries/EspUsbHost/**`) are forbidden unless the owner explicitly says “USB keyboard / کیبرد USB”.
- Never build or upload without explicit owner approval. Preserve all unrelated user changes.

## Documentation map

- `docs/PROJECT_NOTES.md`: hardware, task map, key source files, build facts.
- `docs/PROJECT_RULES.md`: repository, artifact, and documentation rules.
- `docs/TECHNICAL_KNOWLEDGE.md`: concise, current lessons and risks.
- `docs/WORK_ITEMS.md`: active work only; completed history is not start-up context.
- `docs/SKILLS_AND_TOOLS.md`: commands and tool selection.
- `docs/SCENARIO_FA.md`: operator/machine behavior and hardware test scenarios.
- `docs/archive/`: historical reports and superseded detail; open only for a named regression or provenance question.

## Working standard

Use the smallest safe patch, do not touch adjacent motion/UI behavior, and report only changed files, verification, and what was not verified. For HMI pixel work, a current real SHOT is final evidence.
