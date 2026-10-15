# Web App Requirements — Pending Owner Approval

Status: research/specification only. Do not implement, build, upload, add a dependency, or change a production API until the owner finishes reviewing the preview and explicitly says to start.

## Cut-list interaction

- Remove the visible `Active rows` field.
- A row is active only when `length > 0` and `quantity` is an integer in `1..999`; derive the API row count automatically at save time.
- Show existing rows plus `Add row`; give each row an explicit remove action. Preserve `LIST_MAX=100`.
- Imported rows remain editable. Nothing reaches the device until the owner presses `Save list` and the normal PIN/machine-state gates pass.

## Import contract

- First-class low-risk inputs: CSV UTF-8, TSV, TXT, and paste from Excel/LibreOffice.
- Recognize comma, semicolon, Tab, and `length x quantity`. Show filename, valid/rejected totals, line number and rejection reason before applying to the form.
- Required fields: `length_mm`, `quantity`.
- Optional mapped fields for future compatibility: part name, project/order, profile/article code, material/color, left/right angle, width/height, piece/barcode, note.
- Direct `.xlsx/.xls` support is desired, but requires a separate dependency/flash/RAM decision and representative owner files. Until approved, the help must explain Excel → CSV UTF-8 export.
- Never claim native LogiKal/Klaes/SchüCal compatibility without a real sample file or official version-specific interface specification. Their production/CNC interfaces are commonly proprietary or machine-specific.

## Export chooser

- Group `Report and sharing`: PDF with preview, Print with preview, XLSX, CSV UTF-8 BOM, TXT, and PNG. Prefer PNG over JPEG for sharp table text.
- Group `Production`: printable labels plus QR/barcode. Add DXF/SVG or machine XML/CNC only after the target controller and schema are explicitly identified; DXF is geometry, not G-code/toolpath.
- Include project/customer, date, unit, kerf, piece count, revision and invalid-row warning where the format supports metadata.
- Use predictable filenames such as `Project_YYYY-MM-DD_CutList.ext`.

## Cut optimization

- Add a clear primary action `Optimize cutting` after the cut-list passes validation.
- Use the current stock length, start trim, blade kerf, valid lengths and quantities; refuse calculation when required values are missing or stale.
- Show a preview before any apply action: required stock bars, pieces on each bar in cutting order, waste per bar, total waste, utilization percentage and any unallocated pieces.
- Keep the original list unchanged until the owner explicitly chooses `Apply optimized order`; provide Cancel/Back and retain both original and optimized views.
- Optimization is calculation only. It must never start motion, cutting or the saw. Sending/applying the optimized list to the device is a separate PIN-gated and machine-state-gated action.
- Allow downloading the completed optimized plan through the approved export chooser.

## Help experience

- Replace `?` with one consistent familiar book or information icon, at least a 44 px target, with `aria-label="Help for this page"` and a visible page-specific title.
- Explain: units and device limits, required/optional columns, valid quantity, 100-row limit, preview/apply/save differences, PIN and stopped-machine requirements, rejected-row correction, export choices, and a worked example.
- Manual Excel/LibreOffice guide: sheet `Parts`; one header row; one unit (mm); no merged cells, formulas, totals or units inside numeric cells; required columns `Part name`, `Length (mm)`, `Quantity`; save as `CSV UTF-8`; import, review every rejected row, apply to the table, then explicitly save.
- Example:

  | Part name | Length (mm) | Quantity | Order | Note |
  |---|---:|---:|---|---|
  | Top frame | 1250 | 2 | 1042 | White |
  | Left post | 2100 | 1 | 1042 | |

## Language selector

- Replace the ambiguous `IR/GB` selector with explicit flag-and-name choices: `🇮🇷 فارسی` and `🇬🇧 English`.
- The flag is a visual shortcut only; always keep the language name for clarity and accessibility.
- Switching to Persian changes the page to RTL and switching to English changes it to LTR without losing the current page or unsaved form state.

## Terminology and information architecture

- Keep two separate values: `Blade kerf per cut (cut width)` for `kerf_mm`, and `Stock start trim` for `trim_mm`. Do not merge or rename one as the other.
- Move `Stop input wiring (NC/NO)` from General to Manager/Safety with a warning and confirmation.
- Rename `Screen-server PIN` to `Device web-access password` and explain that it protects web saves and firmware update, not LCD login.
- Put base direction under Manager → Advanced as `Motor base direction: Normal / Reversed`, with an explicit consequence warning and confirmation.
- Put factory reset only under Manager → Advanced. It requires a real backend contract, a list of erased/preserved data, double confirmation and a recovery path; the current API has no factory-reset route.
- The existing `Reset user counter` is not factory reset and must remain distinctly named.
- Use familiar icons for sections, actions and safety-critical settings (blade, stock start, stop, motor, lock, reset). Do not attach decorative icons to every minor label because that harms scanning and hierarchy.

## Motor controls

- Use bounded sliders for quick adjustment only when paired with an exact numeric value/input, unit, allowed range and reset-to-recommended action.
- Keep travel speed/ramp understandable in Motor settings. Move jog speed, jog ramp and jog soft-start into Manager → Advanced under `Manual-motion tuning`, collapsed by default.
- Never accept a slider change silently: show dirty state, old/new value and explicit Save; preserve firmware validation and machine-state gates.
- Resolve the confirmed drift before release: web saw warm-up currently exposes `1..5`, while the API accepts `0..60` seconds.

## Home page

- Make the primary workflow obvious: machine/connection status → current position and destination → active job/progress → `Cut list`, `Window/Door designer`, and `Reports` actions.
- Replace unclear one-letter mode marks with familiar icons plus complete Persian/English labels. Keep critical state textual; never use color or icon alone.
- Add compact project summary, current list/optimized-plan status, reconnect/help entry and last-save state without hiding machine safety status.

## Window and door designer

- Create a responsive 2D designer for mouse and touch with templates for fixed window, sash, door and sliding elements; support select, split, drag/resize, zoom/pan, snap, undo/redo, duplicate and project quantity.
- Use a real model: `Project → Element → Frame → Field → Sash/Mullion/Transom`, with opening direction, connection and profile role. The drawing is a view of this model, not the manufacturing source of truth.
- Maintain manager-owned, versioned rule sets by material, brand/system/series, profile role and machine recipe. Generate BOM, profile cut-list, angles, glass list and manufacturing warnings from the selected rule set.
- There is no universal uPVC weld allowance. Calculate weld/corner allowance only from an approved profile-system and machine rule set; otherwise mark the result `Draft — factory confirmation required` and never present guessed lengths as production-ready.
- Separate uPVC welded connections from aluminium mechanical/mitre/T-joint rules. Group optimization by compatible profile code/material/color and preserve each design's quantity.
- Before implementation the owner must supply representative catalogs/rules: exact profile codes and series, connection types, length reference, frame/sash allowances, saw kerf/end deductions/limits/angles, stock lengths, reinforcement/hardware/drainage/glazing clearances, rounding, rule-set version and real acceptance samples.

## Access and responsive behavior

- Preserve the existing protected SoftAP, wildcard captive DNS and Android/Apple/Windows probe routes. Automatic captive-portal opening is a best-effort convenience and cannot be guaranteed across every phone, tablet or laptop.
- Provide a persistent HMI access card while the portal is enabled: SSID, password, large QR for `http://192.168.4.1/`, and three fallback steps. Keep the fixed IP visible/copyable.
- Consider `http://haffler-cutter.local` via mDNS only as a secondary convenience after dependency/RAM approval; never make it the sole route.
- Responsive targets: 320×568, 360×800, 390×844, 412×915, 768×1024, 1024×768, 1280×800 and 1366×768; test Persian/English, portrait/landscape, keyboard focus, drawer/modal scrolling and offline/reconnect.
- Planned layout ranges: compact `≤390`, mobile `391..767`, tablet `768..1023`, desktop `≥1024`. The current 470 px shell is phone-friendly but not tablet/desktop optimized.

## Safety and resource constraints

- Parsing/export is event-driven browser work, never a machine-control or unrestricted UI-tick task.
- Keep bounded file size, row count and generated output; measure flash/heap before enabling PDF/XLSX/image libraries.
- Preserve PIN lockout, stopped-machine checks, offline/stale state, and explicit user confirmation. Import/export must not trigger optimization, motion or cutting.
- Production implementation must update the UI and `/api/list/set` together because the current API persists the explicit `rows` field.

## Research evidence

- Orgadata modules and interfaces: <https://www.orgadata.com/en/product/module-cad-suite-2d>, <https://help.orgadata.com/en/documentation/interfaces/cad-3d/autocad/>
- Klaes CAM/e-control: <https://www.klaes-it.de/en-cam-2d>, <https://www.edv.klaes.de/en-e-control>
- SchüCal/SchüCam: <https://www.schueco.com/uk/fabricators/digital-solutions/schuecal>
- Generic cut-list import/export guidance: <https://cutoptim.com/docs/importing-data>, <https://cutoptim.com/docs/exports>
- Orgadata estimation and saw rules: <https://help.orgadata.com/en/documentation/estimation-and-labour-hours/estimation/estimation-data/standard-estimation/>, <https://help.orgadata.com/en/documentation/cnc/machine-setup/saw-setup/>
- Captive-portal platform behavior: <https://source.android.com/docs/core/connect/android-custom-tabs-captive-portal>, <https://support.apple.com/en-ie/102554>, <https://learn.microsoft.com/en-us/windows-server/networking/ncsi/ncsi-overview>
- Espressif mDNS reference: <https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/protocols/mdns.html>
