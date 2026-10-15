# Work Items And Change Checklist

This file is the live checklist. Before editing code, add planned work here. After editing, mark completed items with `[x]`.

## Active / Open

### Current Request - F5 official lunar occasions — 2026-08-01

- [x] F5-LUNAR-OCCASIONS-01: Add the published Iranian solar-date mappings for major lunar occasions in 1405.
- [x] F5-LUNAR-OCCASIONS-02: Keep lookup offline, year-bounded, allocation-free, bilingual, and never approximate lunar dates.
- [x] F5-LUNAR-OCCASIONS-QA-01: Static mapping, fallback, and clock integration validated; build/SHOT pending approval.

### Current Request - F3/F4/F5 exact geometry and clock — 2026-08-01

- [x] F3-PICKER-ALIGN-01: Move Persian I/Q choice badges 20px left; mirror badges to the right in English without text overlap.
- [x] F3-INPUT-LABEL-01: Move the ten input-row labels 4px down while preserving LED and I-badge geometry.
- [x] F4-GEOMETRY-01: Apply requested direction-label, card-row, PIN-bar, and date-bar geometry changes with anchored edges.
- [x] F5-CAPTION-ALIGN-01: Move all F5 info captions 7px down in English and 2px down in Persian.
- [x] F5-MAKER-NOSCROLL-01: Keep the manufacturer page fixed because its content fits the viewport.
- [x] F5-CLOCK-STEP-01: Update the red seconds hand once per real second at exact 6-degree steps.
- [x] F5-HW-STATUS-01: Add a lightweight F5 hardware strip with CPU MHz, RAM usage, LCD FPS, and free RAM.
- [ ] F3-F5-GEOMETRY-QA-01: Run static checks; build/upload and real F3-F5 SHOT comparison remain pending approval.

### Current Request - Industrial stability pass — 2026-07-29

- [ ] STABILITY-WIFI-SCAN-01: Root-cause and repair update-page Wi-Fi scanning without repeated driver resets, retry storms, or UI blocking.
- [ ] STABILITY-WEBAPP-01: Root-cause and repair F1 Wi-Fi/Web App startup errors while preserving update Wi-Fi ownership.
- [ ] STABILITY-USB-RECONNECT-01: Repair USB keyboard disconnect/reconnect using the authorized Task4/EspUsbHost scope; preserve jog-stop safety.
- [ ] STABILITY-USB-INPUT-01: Route USB keyboard input consistently to active numeric/text controls across list and F1-F5 screens.
- [ ] STABILITY-OPT-HANG-01: Resume optimizer bar scanning across UI ticks so jobs beyond the per-tick budget always progress or cancel.
- [ ] STABILITY-UI-LOAD-01: Remove proven redundant polling/reflow/allocation hotspots, especially F3, without changing pixels or motion behavior.
- [ ] STABILITY-REGRESSION-01: Run offline Wi-Fi/USB/optimizer/UI tests; leave build, upload, SHOT, and hardware motion checks pending explicit approval.
- [ ] STABILITY-SAFETY-LOCK-01: Do not change jog, destination, backlash, homing, pin polarity, cut sequencing, or emergency-stop behavior.

- [x] SETTINGS-F1-KB-LEAK-01: Close/hide the F1 touch keyboard before a screen/tab transition so it never paints on the next page for one frame.
- [x] SETTINGS-F2-CARDS-01: Move both start/end-line gray caption cards exactly 5px up; change caption cards to white and numeric cards to a softer off-white.
- [x] SETTINGS-F2-HELP-01: Long-press every F2 control except Save opens clear modal help and suppresses the released touch action.
- [x] SETTINGS-SAVE-HELP-01: Redesign Save long-press help across all settings tabs so text never sits behind buttons/timer; double its display duration.
- [x] SETTINGS-F3-CONFIRM-01: Redesign only F3 input/output pin confirmation dialogs with compact consistent geometry, aligned Cancel, and correct I+number typography.
- [x] SETTINGS-F3-PICKER-01: Pin picker header shows the role and current pin when connected; replace the vague free-pin text with one clear explanatory line.
- [x] SETTINGS-F3-HELP-01: Long-press every pin-picker item shows precise modal help and suppresses selection on release.
- [x] SETTINGS-F3-HANG-01: Reproduce and fix the pin-setting freeze using object-count, heap, stack, event, and modal-lifecycle evidence.
- [x] SETTINGS-F4-LAYOUT-01: Move F4 content group 65px up; admin-password title 13px up; error label 20px right; error blue card 12px left; value card 3px up.
- [x] SETTINGS-F4-HELP-01: Long-press every F4 control opens precise modal help and suppresses the released action.
- [x] SETTINGS-F5-HELP-01: Long-press every F5 control opens precise modal help; find and fix intermittent F5 freeze.
- [x] SETTINGS-F5-STATS-01: Replace the clipped/scrolling total-count modal with a fixed non-scroll layout containing only lifetime, today, manual, cut-list, and optimization totals.
- [x] SETTINGS-F5-WARRANTY-01: Move warranty text and its three fields exactly 30px down.
- [x] PANEL-NAV-SETTINGS-01: Five-key MODE navigation must enter every actionable item inside F1-F5 instead of focusing only the settings page container.
- [ ] SETTINGS-F1-F5-QA-01: Build/upload once, capture fresh F1-F5 and relevant modal SHOTs, compare before/after, and hardware-check MODE navigation.

- [x] FORCED-RESET-REMOVE-01: Remove the one-shot 20-second test restart timer and its setup hook without changing explicit update, recovery, or user-requested restart paths.

- [x] JOG-BLADE-OFFSET-01: Decide the final jog seek against the corrected motor target so an exact stop on the visible destination cannot skip the signed blade offset.

- [x] BLADE-OFFSET-MOTOR-01: Keep the displayed destination unchanged; apply the armed signed blade correction only to the real motor target, including the jog release seek, with zero effect while disarmed.

- [x] UPDATE-WIFI-MEM-01: Fix proven WiFi task allocation failure by reducing the flash-safe internal UpdateNet stack to 8KB, avoiding repeated radio deinit/reinit during scans, and stopping failed-init retry storms.

- [x] VIRTUAL-KB-BOOT-01: در حالت AUTO نمایش کیبورد لمسی تا پایان مهلت شناسایی اولیه عقب می‌افتد.
- [x] VIRTUAL-KB-BOOT-02: پس از پایان مهلت، وضعیت نهایی موجود فقط یک‌بار روی صفحه فعال اعمال می‌شود.
- [ ] VIRTUAL-KB-BOOT-QA-01: بعد از آپلود صریح، بازیابی F1/F2 با USB وصل و بدون USB روی LCD واقعی تست شود.

- [x] F5-TEMP-MSG-01: بافر هشدار دمای پردازنده بزرگ شد و متن فارسی در دو خط کامل نمایش داده می‌شود.
- [x] F5-TILE-ROUTE-01: نسخه برنامه فقط اصلاحات کد را باز می‌کند؛ تاریخ برنامه فقط سوابق نصب را باز می‌کند.
- [ ] F5-TEMP-ROUTE-QA-01: پس از بیلد و آپلود صریح، متن هشدار و مسیر هر دو کادر روی LCD واقعی تأیید شود.

- [x] LIST-NUM-TOUCH-01: عددهای متراژ و تعداد با RELEASED و PRESS_LOCK روی تاچ واقعی فوکوس می‌گیرند؛ تشخیص اسکرول موجود حفظ شد.
- [ ] LIST-NUM-TOUCH-QA-01: پس از آپلود، لمس متراژ/تعداد، نشانگر آبی و ورود عدد روی LCD واقعی تأیید شود.

- [x] LIST-TOUCH-KB-01: Keep list touch keypad usable whenever its effective setting is enabled, without changing USB detection/input behavior; support length decimals, integer quantity, and a clear Persian confirm button.
- [ ] LIST-TOUCH-KB-QA-01: After explicit build/upload, verify touch entry for decimal length, integer quantity, backspace, and Persian confirm on the real LCD with USB both absent and connected.

- [x] LIST-FRESH-ZERO-01: A newly entered length with quantity 0 must not be struck through; depleted styling only after a real quantity decrease caused by cutting.
- [x] OPT-LIMIT-01: Allow up to 5000 requested cuts and up to 1000 stock bars.
- [x] OPT-PROGRESS-01: Calculation modal shows only the centered percentage; no stage text or remaining-time estimate.
- [x] OPT-BAR-PAGING-01: Keep five reusable LVGL rows but page them across every generated bar, up to 1000 bars.
- [x] OPT-BAR-PAGE50-01: `OPT_RENDER_ROWS=50` scrollable page; at bottom loads next 51–100; prev/next + label; left/right swipe pages.
- [ ] OPT-LIST-QA-01: After explicit build/upload, verify fresh zero rows, 5000-piece calculation, centered percent-only modal, and horizontal paging through bars beyond 5 on the real LCD.

- [x] F4-HOME-01: Home button on F4 PIN gate (6,6 44×39) like other F tabs; shared header home kept foreground when unlocked.

- [x] F5-BTN-SPLIT-01: White card +5px bottom; update half-width blue + warranty half-width green inside card; `F5_LAYOUT_VER` 18.

### Current Request - F5 market card + warranty — 2026-07-27

- [x] F5-CARD-01: White card +7 top / +30 bottom intent; compact 4×2 grid; update bar inside card under clock/temp; `F5_LAYOUT_VER` 17.
- [x] F5-CELL-01: Live system clock + CPU temp captions; build date + free RAM cells.
- [x] F5-WARR-01: Warranty activate once; frozen date in NVS; button replaced by date tile.
- [x] F5-HELP-01: Help updated for new keys; FA without Latin tofu.
- [ ] F5-QA-01: Board verify after upload (layout, live clock, warranty one-shot).

### Current Request - F2/F3/F5 polish — 2026-07-28

- [x] F3-PINHELP-TITLE-01: Move help-card serif I down 3px; replace clipped scaled digit with enabled Montserrat 38 and align the I+digit pair.
- [x] F2-KB-RED-01: Touch backspace key red background (overlay on BS cell only).
- [x] F2-USB-01: USB digits/decimal/backspace on F2 auto-start start-size edit (before F1 steal path).
- [x] F2-MM-NUM-01: mm +3 right; size numbers center with 6px left bias.
- [x] F3-LAYOUT-05: Dir+Q1 −3; Pulse text +12; LED↔label↔I dotted bridges; `F3_LAYOUT_VER` 74.
- [x] F5-CARD-02: All info cells equal height/title font; white card bottom −2; `F5_LAYOUT_VER` 19.
- [x] F1-TOOLS-H-01: Tools white bar height 260→258.
- [ ] F2F3F5-QA-01: Board verify after explicit upload.

### Current Request - F2 keyboard + F3 pixels/stepper — 2026-07-27

- [x] F2-KB-DEC-01: Soft KB decimal/backspace for start/end size; no overlay covering `.`; centralized `f2_on_backspace`.
- [x] F2-KB-DEC-02: Init/start label from pulses with 1 decimal; no buffer reload while editing.
- [x] F3-LAYOUT-04: Enable text −3 / Q2 −5; Dir+Q1 −3; Pulse text +19 / Q0 −10; `F3_LAYOUT_VER` 73.
- [x] F3-STEPPER-TOGGLE-01: Second tap while test runs → reverse net pulses to start + EN off.
- [ ] F2F3-QA-02: Board verify after explicit upload (F2 decimal/delete, F3 pixels, stepper re-tap).

### Current Request - F2/F3/F4 pixel + decimal — 2026-07-27

- [x] F2-DEC-01: Start/end size boxes keep 1 decimal via pulses + format helpers (not int `L2_GUI` display).
- [x] F2-SLIDER-01: Three % sliders + title/pct labels −4px left.
- [x] F3-LAYOUT-03: Enable+Q2 −3; Dir+Q1 −3; Q0 badge only +15; whole capsule green during stepper test; `F3_LAYOUT_VER` 72.
- [x] F3-Q-LOCK-01: While motion/stepper-test, Q3–Q8 touch does not toggle outputs (`GUI_Func.h`).
- [x] F3-HELP-02: Richer FA/EN help (capsule + outputs); FA-only body (no tofu Latin).
- [x] F4-LAYOUT-05: Items −9 left; blade title tighter; PIN larger centered; HH/MM caps + date row right; TA no scroll; `F4_LAYOUT_VER` 5.
- [x] F4-LAYOUT-06: Top 3 cards −7 left; FA titles at EN pos; blade letter-space; PIN row −3; timer TA −4 + `S`; dir btns −4 + active/dim; tools bar −4 (`F1_TOOLS_H` 254); `F4_LAYOUT_VER` 6.
- [x] F2F3F4F5-2026-07-28: F2 num +3R + KB OK-reflow fix; F3 pulse/dir/Q1 + dots from centers; F4 eye-hide, S right gray, tools−8, PIN clip, clock KB, joy toggle; F5 title/val Y + RTC clock/temp; vers F3=75 F4=7 F5=20 toolsH=246.
- [ ] F2F3F4-QA-01: Board verify after explicit upload (decimal sizes, F3 green capsule, F4 clock/date).

### Current Request - F5 hang revert — 2026-07-26

- [x] UF-HANG-REVERT-01: Restore `update_flow.h` from git HEAD (undo broken WiFi-KB / lock / layout experiments).
- [x] UF-HANG-REVERT-02: Do not start UpdateNet/WiFi prefetch on admin password page; start only after PIN OK.
- [x] UF-HANG-REVERT-03: Build + flash COM8; boot verify OK.
- [x] UF-RECOVER-01: Re-applied pre-sticker work (PIN keypad/layout/lock/cards/wifi labels) WITHOUT custom WiFi keyboard maps; hang-safe kept; flash COM8 OK.


### Current Request - F5 boot update PIN + cards — 2026-07-26

- [x] UF-PIN-LAYOUT-01: Remove decimal; red backspace left; green confirm right; larger keypad +16px down; title -6; PIN/eye/sub -14.
- [x] UF-CARD-LAYOUT-01: Cards height -11 from bottom; Install -12 up; device images -4 up.
- [x] UF-PIN-LAYOUT-02: PIN box/eye/sub +5px down; wrong-pw text +9px down.
- [x] UF-PIN-LOCK-01: After 3 wrong PIN attempts lock 10 min; master `12773` always accepted while locked.
- [x] UF-WIFI-CONNECT-01: Longer scan (700ms×3), channel/BSSID connect, open-AP + min-security, 3 retries; saved WiFi prefetch kept.
- [x] UF-WIFI-LAYOUT-01: WiFi SSID label -29px left; connecting status -7px more left (x=-42).
- [x] UF-PIN-LAYOUT-03: Wrong-pw +3px more (y=118); show fail count before text; NVS lock survives reset and restarts 10 min.
- [x] UF-WIFI-LAYOUT-02: SSID y=4 x=-44; connecting status x=-49.
- [x] UF-WIFI-KB-01: Custom maps without Arabic «أب» (tofu); deferred KB create; OK=connect; connect tries plain/channel/BSSID.



### Current Request - F3 Q0-Q2 stepper test + Q5/Q8 swap — 2026-07-25

- [x] F3-Q012-LAYOUT-02: Enable text +24, Q2 +20; Pulse text -16, Q0 +12; capsule bottom -3px; Q8<->Q7 swap; active green while stepper test; I-row yellow press highlight. `F3_LAYOUT_VER` 70.

- [x] F3-Q012-TEST-01: Short tap Q0/Q1/Q2 runs Task3 bit-bang: 50 pulses right, 1s pause, 50 pulses left, 25 ms/pulse; stop aborts; EN off after; no position update.
- [x] F3-Q012-LAYOUT-01: Pulse text -10px; Q1+Dir -12px; Enable text -8px; `F3_LAYOUT_VER` 69.
- [x] F3-Q5Q8-SWAP-01: Swap Q8+lift with Q5+spray positions (btn2_3_5 <-> btn2_3_1).
- [ ] F3-Q012-QA-01: Board verify on F3: short-tap motion, stop mid-test, long-press help text no squares, Q5/Q8 swap visual.


### Current Request - Main-motion LCD throughput — 2026-07-25

- [x] PERF-MAIN-BASELINE-03: Capture real COM8 main-screen baseline without source changes: idle CPU 7-11%, motion CPU 77-99%, motion FPS 3-12, and RF 96-206%.
- [x] PERF-MAIN-ROOT-02: Confirm the 40 MHz full-screen transport is already near its physical ceiling: 13.88 FPS blocking and 15.48 FPS DMA; do not retry the rejected 80 MHz/frozen-panel path.
- [x] PERF-MAIN-DEST-01: Hardware-tested the per-character destination redraw; it produced no measurable gain and was fully reverted.
- [x] PERF-MAIN-QA-02: Closed after hardware QA failure; the later A8 digit-font experiment rendered the center value as a solid red block and was also fully reverted.
- [x] PERF-MAIN-A8-03: Replace the 4-bpp full-ASCII digit160 export with an exact sparse static-A8 conversion (`-`, `.`, `0-9`), correct per-row stride, and LVGL `static_bitmap`; build, upload, and boot verification passed.
- [ ] PERF-MAIN-A8-QA-04: Measure real motion FPS/CPU once during normal operator use; do not request another dedicated jog loop.

- [x] F2-NUM-01: Bottom-left size numbers visible again (remove clip+scale hide); mm +5px right + montserrat_12; decimal append fixed (soft/USB/numpad) without focus jump; `ui_extras.h` + `Task4.h`.

- [x] F3-TOUCH-01: Root cause — after Q long-press, overlay causes `PRESS_LOST` and `f3_help_q_armed` stayed true forever so every later Q `RELEASED` was `stop_processing`. Fix: disarm on close/PRESS_LOST + timed suppress window + PRESS_LOCK + USER_2 skip generic help; `F3_LAYOUT_VER` 55. No upload until user asks.

- [x] F3-HELP-01: Long-press on F3 I/Q pin badges (and row labels/LEDs, Q0-Q2 capsule) shows a white help card with FA/EN text; Q short-tap test unchanged; long-press does not toggle outputs (`RELEASED|PREPROCESS` suppress). Guard: `F3_PIN_HELP_ENABLE` in `admin_ui.h` (set 0 to disable). `F3_LAYOUT_VER` 42.

- [x] F3-I-TXT-01: White-card FA labels all `naz18b`, 8px gap from I circle, vertical center on badge; I glyph scaled with 3px inset + `montserrat_12` digit (no rim stick); `F3_LAYOUT_VER` 32.


- [x] F3-ALIGN-01: White card left = Home x=6; blue block right = Save right x=408; keep inner identity; `F3_LAYOUT_VER` 30.
- [x] F3-JACK-01: Fix جک نگهدارنده/محافظ/تیغه centering inside Q buttons (width wrap, ox/oy/line_sp).


- [x] F3-Q-BADGE-01: Enlarge black Q-pin circles on blue F3 output buttons from 20px to 30px so `Q#` text fits proportionally; bump `F3_LAYOUT_VER` to 10; keep I badges at 20px.
- [x] F3-Q-BADGE-02: Q badges → navy metallic (`0x1A365D` + steel rim); text slightly smaller via transform scale ~82%; `F3_LAYOUT_VER` 11.
- [x] F3-Q-BADGE-03: Restore visible `Q#` text — remove transform scale (was clipping), use real `montserrat_12`, keep navy metallic; `F3_LAYOUT_VER` 12.
- [x] F3-Q-BADGE-04: Q and digit same font; two sizes smaller (`montserrat_8`); `F3_LAYOUT_VER` 13.
- [x] F3-Q-BADGE-05: Q text size back to normal proportional `montserrat_12` (8 was too small); `F3_LAYOUT_VER` 15.
- [x] F3-Q-SHIFT-01: Blue Q buttons same size, shift 14px left (186/298) clear of F-tabs; button labels naz20→naz24 / m14→m16; `F3_LAYOUT_VER` 20.
- [x] F3-Q-SHIFT-02: Blue Q +10px more left (176/288); labels true center (0,0); `F3_LAYOUT_VER` 21.
- [x] F3-Q-WIDTH-01: Blue Q width 104→90 (X only); badge top-left at (2,2) with pad 0; `F3_LAYOUT_VER` 23.
- [x] F3-Q-GAP-01: Space blue Q from left white card (~10px): QX0/QX1 188/288; left panel untouched; `F3_LAYOUT_VER` 25.

### Current Request - Update page (F5 / boot OTA) — 2026-07-22
- [x] UF-UI-01: Card layout like V.1.5.0 — per-card server version + Install ≥40px; both cards visible; no crushed footer.
- [x] UF-UI-02: FA/EN on update page; fix title «تک‌سر برش».
- [x] UF-SAFE-01: Config install requires confirm dialog; Exit locked while flashing.
- [x] UF-NET-01: Pre-scan both model versions on entering update screen (`uf_req_scan_models`).

### Correction List EN (Desktop DOCX) — 2026-07-19

Source: C:\\Users\\Haffler\\Desktop\\Metr_Cutter_Agent_Ready_Correction_List_EN.docx

Batch A (testable first; upload after each sensitive group):
- [x] CL-OPT-01: Reject totals >3000 pieces before optimize; accept 2999/3000; keep per-row max 999.
- [x] CL-MAIN-02: Target/list numeric entry — leading-zero suppress, one decimal, LTR buffer (USB+numpad).
- [x] CL-LIST-01: Decimal lengths in cut-list; quantity integer-only (shared with MAIN-02).
- [x] CL-REF-01: Reference timeout = 30000*100/speed_percent; safe abort + single-language error; preserve safety stops.

Deferred next batches (do not start until Batch A uploaded/smoke-ok):
- [x] CL-BOOT-01/02 splash rotation + HAFFLER/version overlay
- [ ] CL-UPDATE-01/02 F5 Wi-Fi/update
- [x] CL-MAIN-01 position green/red (Batch B 2026-07-19 — kept)
- [ ] CL-MAIN-03 help
- [x] CL-MAIN-04 cut toast + 5s holdable bar (Batch B)
- [ ] CL-INPUT-01 / CL-MOTION-01/02 NumLock jog + prediction + ramp
- [ ] CL-LIST-02 / LIST-KB-01/02 bottom white + keypad — REVERTED 2026-07-19 (broke working list look; do not touch list geometry without SHOT+owner OK)
- [ ] CL-OPT-02..06 completeness, wrap, header, bar load/context



### Current Request - Physical LCD Freeze At Reference Screen

- [x] LCD-FREEZE-EVIDENCE-01: Treat the operator's physical panel report as authoritative: UART/QSTA and 480x320 SHOT remained alive while the real LCD stopped on the green reference screen, isolating the failure to panel transport rather than LVGL or Task2.
- [x] LCD-FREEZE-EVIDENCE-02: Trace every object on `MAIN_0`: green `btn0_1`, red exit `btn0_2`, confirmation panel, spinner, and E-stop image. None contains an unbounded loop; the large E-stop image explains the SHOT content but not a live-UART/frozen-panel split.
- [x] LCD-DRIVER-AUDIT-01: Read the active TFT_eSPI ESP32-S3 DMA implementation and upstream evidence. The failing build combines 80 MHz, HSPI, hardware-controlled CS, and separate address/pixel transactions; this matches the upstream first-frame-then-freeze failure pattern.
- [x] LCD-DRIVER-FIX-01: Move the LCD to the supported FSPI path at the conservative 40 MHz stability ceiling, use manual continuous CS exactly as TFT_eSPI documents, and queue only pixel data after setting the address window inside that transaction.
- [x] LCD-DRIVER-FIX-02: Retain bounded 50 ms DMA completion, blocking recovery boot, delayed DMA enable, explicit LVGL RGB565 swap, and dual internal DMA buffers.
- [x] LCD-DRIVER-FIX-03: Replace TFT_eSPI's unbounded `portMAX_DELAY` queue call on the active LVGL path with an ESP32-S3 nonblocking DMA queue primitive; if queuing fails, close CS and fall back to blocking transfer without hanging.
- [x] LCD-DRIVER-QA-01: Source-check CS lifetime, buffer lifetime, cache copy, and build inputs without touching the locked F1/UI file. Eleven assertions passed: single 40 MHz definition, default FSPI, single SPI owner, manual CS, no hardware CS, nonblocking queue call, zero-wait IDF submit, LVGL wait callback, inactive legacy driver, strict UTF-8, and build-helper parse.
- [x] LCD-DRIVER-BOOT-01: Decode the first 40 MHz upload's exact StoreProhibited backtrace to `TFT_eSPI::begin_tft_write()` and prove the address fault: Arduino-ESP32 3.3.9 defines FSPI as class index 0 while the ESP32-S3 register macro accepts hardware indices 2/3, producing invalid address `0x10`.
- [x] LCD-DRIVER-BOOT-02: Select TFT_eSPI's explicit `USE_FSPI_PORT` compatibility path so `SPIClass(FSPI)` remains on FSPI while direct register access uses SPI2 hardware index 2.
- [x] LCD-DRIVER-HW-01: Build/upload the corrected 40 MHz FSPI/manual-CS image, capture clean boot and DMA telemetry, verify `QSTA buz=0`, and confirm the physical LCD progresses; operator reported it was better.
- [ ] LCD-DRIVER-MOTION-01: Stress real main-screen motor motion and confirm the physical panel remains stable; serial SHOT alone cannot prove panel output under motor load.

### Current Request - Global LCD And Main-Motion Performance

- [x] PERF-BASELINE-01: Capture a fresh real main-screen SHOT and run the current `LCDB` benchmark before editing. Baseline: static main `RAM 60% CPU 6% FPS 60 RF 0%`; full-screen blocking push `22.77 FPS`; DMA disabled.
- [x] PERF-LCD-01: Historical 80 MHz/hardware-CS experiment completed, then rejected after the operator confirmed the physical LCD froze; it is not the active design.
- [x] PERF-LCD-02: Use bounded dual internal DMA buffers (32 lines maximum, automatic 24/20/16/12/8 fallback) so LVGL rendering overlaps transfer. Maximum buffer cost is 61,440 bytes; single-buffer fallback remains available.
- [x] PERF-CORE-01: Isolate the LVGL render/tick tasks from the high-frequency stepper ISR by moving the UI pair to core 0 while leaving motion and safety tasks on core 1.
- [x] PERF-MAIN-01: Replace the full-width main-position label redraw with cached per-character labels; normal position changes invalidate at most about 17,360 pixels instead of 71,765 pixels and can update on every 25 ms refresh instead of every second refresh.
- [x] PERF-QA-SOURCE-01: Re-check source diffs, UTF-8, buffer/DMA limits, CS ownership, core placement, fallback paths, and available safe local tests.
- [x] PERF-DMA-HANG-01: Historical 80 MHz/hardware-CS safeguards were tested but did not prevent the physical-panel freeze; superseded by `LCD-DRIVER-FIX-01..03`.
- [x] BUZZER-DEFAULT-01: Default the buzzer OFF both when `buz_stat` is absent and when hardcoded EEPROM/NVS defaults are recreated.
- [x] PERF-BUILD-02: Build the current source successfully after synchronizing the stale TFT_eSPI dependency cache.
- [x] PERF-QA-HW-DMA-01: With explicit permission, build and upload the current binary, capture a clean dual-baud boot, verify `QSTA`, capture a fresh 480x320 SHOT, and rerun LCDB: blocking 24.34 FPS, DMA 30.30 FPS, partial 500 FPS.
- [ ] PERF-QA-HW-MOTION-01: Confirm the physical panel left the splash, then stress real main-screen motor motion and record CPU/FPS/RF; serial SHOT cannot prove panel output or motor-load stability.
- [x] PERF-BASELINE-02: Capture fresh real page baselines on the corrected 40 MHz driver: main `CPU 8/RF 3`, F2 `CPU 85/RF 142`, list `CPU 10/RF 3`, F1 `CPU 16`, F3 `CPU 13`, F4 `CPU 19`, and F5 `CPU 35/RF 45`.
- [x] PERF-F2-REDRAW-01: Stop rewriting six F2 line-box style properties every 25 ms; apply them only when the two sensor states change.
- [x] PERF-F5-REDRAW-01: Replace F5's periodic full layout reflow with a one-second change-aware runtime value refresh.
- [x] PERF-MAIN-ALLOC-01: Keep main-position digit storage persistent and use static LVGL label text so 40 Hz updates do not allocate/free strings.
- [x] PERF-POLL-01: Reduce unnecessary 1 ms polling while preserving exact LVGL tick deltas, continuous input/timers, and the 40 Hz main-position update.
- [x] PERF-CARET-01: Gate caret/keyboard maintenance to 10 ms so active F1/F2 editing does not repeat LVGL style and foreground work every 2 ms.
- [x] PERF-NVS-01: Remove five-second eight-field NVS checkpoints, duplicate terminal-row zero writes, and back-to-back counter sessions that could stall both cores during target transitions.
- [x] PERF-OVERLAY-01: Use persistent static text for the performance overlay to eliminate its periodic LVGL string allocation/free.
- [ ] PERF-BUILD-HW-03: After explicit permission, build/upload the newest CPU patch and capture fresh main, F2, list, and F5 SHOT metrics before real motor-motion validation.

### Current Request - First Two Boot Hangs

- [x] BOOT-HANG-01: Harden F5 boot-update ADC detection so startup drift cannot falsely open the green update/password screen.
- [x] BOOT-HANG-02: Defer the heavy `UpdateNet` task until password is accepted or update is explicitly opened from the F5 UI.
- [x] BOOT-HANG-03: Build, upload, then verify the first two boots no longer hang/reset on hardware.

### Current Request - Main Screen Motion UI Performance

- [x] MAIN-MOTION-PERF-01: While the stepper is moving on the main screen, keep LVGL updates limited to the center position number and minimal safety/indicator state.
- [x] MAIN-MOTION-PERF-02: Prevent the performance overlay from invalidating extra top-screen pixels during main-screen motion.
- [x] MAIN-MOTION-PERF-03: Verify by build/source check now; hardware FPS/feel validation remains required after upload.

### Current Request - Optimization Screen Review Pass

- [x] OPT-REVIEW-01: Keep optimization piece text fully visible before the red waste badge with at least 4 px clearance.
- [x] OPT-REVIEW-02: Keep current piece, `size * count`, and remaining pieces on one line where possible without touching the waste badge.
- [x] OPT-REVIEW-03: Keep optimization rows square-cornered and the white list body continued to the LCD bottom.
- [ ] OPT-REVIEW-04: Re-capture a fresh hardware optimization SHOT when the HMI serial port is available.

### Current Request - USB Unplug, Mouse X Axis, List Soft Keyboard Follow-up

- [x] USB-UNPLUG-FINAL-01: On USB keyboard/mouse removal, do not call transfer/interface/device cleanup from the active runtime path; only forget local state so Task4 cannot hang any screen.
- [x] USB-MOUSE-X-FINAL-01: Treat 4/5-byte mouse reports with report IDs 1..8 as report-ID-prefixed so horizontal X is not misread as Y.
- [x] USB-MOUSE-X-FINAL-02: Use HID `pan` as a fallback horizontal movement source when standard X is zero.
- [x] LIST-KB-FINAL-01: On cut-list screen, open the software keyboard when soft keyboard is enabled or no USB keyboard is present, independent of the global effective flag.
- [x] LIST-KB-FINAL-02: If no cut-list cell is active, focus the first row on the current page before opening the software keyboard.

### Current Request - USB Mouse And Disconnect Stability

- [x] USB-MOUSE-XY-01: Fix USB mouse horizontal movement by making HID mouse delta parsing tolerant of common boot/report-id/absolute-ish reports and avoiding double signed casts.
- [x] USB-DISCONNECT-01: Stop doing heavy USB resource cleanup inside the DEV_GONE callback; defer it to Task4 loop to avoid watchdog reset on keyboard unplug.
- [x] INPUT-NVS-01: Remove direct NVS writes from USB key repeat/edit paths so keyboard input cannot stall while typing or hot-unplugging.
- [x] HANG-AUDIT-01: Static-audit screen/task paths for obvious unbounded UI object creation, blocking loops, and flash writes inside input handlers.

### Current Request - List/Optimization Screenshot Corrections

- [x] LIST-SHOT-01: Extend the cut-list white body to the bottom so the last row is not visually clipped.
- [x] LIST-SHOT-02: Widen the cut-list title white card 2 px to the right and reduce its radius to match header buttons.
- [x] LIST-SHOT-03: Move the cut-list minus button 1 px right.
- [x] LIST-SHOT-04: Make the first cut-list row rectangle use square corners so its lower left/right corners are not rounded.
- [x] OPT-SHOT-01: Rework the optimization header/body from the real shot: remove duplicated top-right labels, keep header controls inside the LCD, extend the white list body to the bottom, and make optimization rows square-cornered.

### Current Request - Mouse, Scroll, List Keyboard, Opt Polish

- [ ] MOUSE-MOVE-01: Make USB mouse pointer move smoothly in all four directions and clamp safely inside the 480x320 screen.
- [ ] MOUSE-SCROLL-01: Route mouse wheel to the active scrollable content on list, optimization, and help screens; do nothing on non-scrollable main screen.
- [ ] MOUSE-HELP-01: Show a short tooltip/help toast on mouse right-click or touch long-press over controls.
- [ ] MOUSE-BACK-01: Use the mouse side/back button to return to the main screen.
- [ ] LIST-KB-01: Add a cut-list software numeric keyboard when USB keyboard is absent, with decimal allowed only in length fields.
- [ ] LIST-VISUAL-03: Move list minus button 1 px left; move list title card 1 px left, 2 px down, and widen 3 px to the right.
- [ ] OPT-VISUAL-01: Fix duplicated/overlapping top-right Bars/Cuts labels on optimization screen.
- [ ] OPT-VISUAL-02: Keep optimization piece text before the red waste badge, keep `size * count` on one line, move text 3 px left, and extend it to 4 px before the red badge.
- [ ] OPT-VISUAL-03: Move optimization title box 3 px left and keep title centered; move printer button 4 px left.
- [ ] OPT-VISUAL-04: Extend optimization list white body to the bottom so gray background is not visible.
- [ ] VERIFY-UPLOAD-02: Build, upload, then capture real list and optimization SHOTs.

### Current Request - Critical Opt/USB/List Fixes

- [x] OPT-CRASH-01: Prevent optimization from creating hundreds of LVGL row objects at once; cap visible row widgets and keep high-count jobs from crashing RAM/LVGL.
- [x] OPT-TIME-01: Make optimization reject or handle oversized cut counts gracefully with a clear toast instead of hanging/resetting.
- [x] USB-HOTPLUG-01: Harden USB keyboard/mouse hot-plug handling so disconnect/connect does not block Task4 long enough to trip reset.
- [x] LIST-VISUAL-01: Move the cut-list title white card 5 px right, move its top edge 2 px down, and reduce height by 2 px from the top.
- [x] LIST-VISUAL-02: Fill the 2-3 px bottom strip under the cut-list rows with the white list panel instead of gray background.
- [x] VERIFY-UPLOAD-01: Build, upload current source to COM4, then capture real SHOT evidence.

### Current Request - NVS Persistence And E-Stop Touch Behavior

- [x] NVS-SAFE-01: Harden NVS persistence so settings and cut-list row count/row values are saved immediately or flushed from pending queues before reset-sensitive paths.
- [x] ESTOP-TOUCH-01: Keep the E-stop logo visible while allowing normal HMI touch/navigation; the E-stop overlay must not own `GUI.lock_screen`.
- [x] ESTOP-SAFETY-01: While E-stop is active, block physical start, jog/reference/manual motion, cut cycle, and all machine outputs from turning on.
- [x] VERIFY-NO-UPLOAD-01: Verify with source inspection and safe local tests only; do not upload/program.

### Pixel-Exact HMI Visual Fix List From `artifacts/shots`

Source images inspected: `artifacts/shots/2026-07-15/raw/*.png`, `artifacts/shots/2026-07-16/raw/*.png`, and the latest list verification shots under `artifacts/shots/2026-07-16/list_fix*/raw/*.png`. Treat the latest 2026-07-16 final/list-fix images as current evidence; use older images only as regression evidence.

- [ ] GLOBAL-PERF-01: Production visuals must not show the top performance overlay. In `firmware/include/app/perf_overlay.h`, gate `perf_overlay_tick()` / `perf_overlay_build()` behind a compile-time or runtime QA flag that defaults OFF for normal firmware. Evidence: `final_f2_keyboard.png`, `final_help_settings.png`, `list_after_fix_clean.png`, and `home_fa.png` show the 286x18 overlay covering title cards or the main target number.
- [ ] SETTINGS-HELP-01: Fix F1-F5 help opening so `QH1..QH5` show `f1_help_dlg`, not the soft keyboard page. Use a two-step Task2 flow: first set `tabview2_1` active and settle/reinit the tab, then close `numpad_cont`, hide `f1_ok_overlay`, clear `GUI.pending_f1_numpad_open`, and call `f1_help_show()` on the following Task2 pass. Verify with fresh FA and EN SHOTs for all F1-F5 help pages.
- [ ] SETTINGS-HELP-02: Make Settings help truly modal. Convert `f1_help_dlg` into a child of a 480x320 top-layer clickable modal catcher, same pattern as `opt_help_modal`; close must delete/hide both the catcher and the card and clear `GUI.lock_screen`.
- [ ] F1-NUMPAD-01: In `numpad_style_f1()`, sync the green `f1_ok_overlay` label with `GUI.system_language`: Persian uses `ui_font_naz18b` and `تأیید`; English must use `LV_SYMBOL_OK` or `OK` with Montserrat, not Persian text. Evidence: `help_f1_general_en_v2_open.png` shows English UI with Persian `تأیید`.
- [ ] F2-KB-01: In `f2_motor_fields_reflow()`, give the left F2 labels more breathing room without changing touch boxes: move `objects.obj9` and `objects.obj10` from `x=80` to `x=83`; keep their `y` and size unchanged; keep `f2_line_kb` at `x=225 y=195 w=168 h=100`. Verify FA labels `سایز اول خط` and `سایز آخر خط` have at least 6 px gap before the value boxes in a fresh `QF2` SHOT.
- [ ] F-TABS-01: Make the active F1-F5 right-side tab unmistakable on every settings tab. Active tab background should be `0xeaf4ff`, border `0x93c5fd`, text `0x0a84ff`; inactive tabs should stay neutral (`0xf7f8fb` / gray text). Apply to F1-F5 consistently and verify FA/EN screenshots.
- [ ] F3-PINMAP-01: In `firmware/include/app/admin_ui.h`, `f3_pinmap_reflow()`, move bottom-row jog/status items upward so I4/I5 are not clipped. Change `ROW5` from `206` to `194`; for I4/I5 badge align entries (`objects.lablef2_26`, `objects.lablef2_23`), change `oy` from `2` to `-4`. Verify the full black I4/I5 circles are visible above y=319 in `help_f3_manual_fa.png`.
- [ ] F3-PINMAP-02: In the same `ii[]` table, move the single-column I2/I0/I1/I3 badges 8 px left by changing their `ox` from `4` to `-4`; keep labels and green LED dots unchanged. Target: badge right edge must have at least 5 px white-card margin and must not touch Persian sensor text.
- [ ] CUTLIST-HEADER-01: In `list_reflow_compact()`, separate the FA length title from the unit label. Preferred exact patch: for Persian, set `objects.lablef3_1` to `متراژ (میلی متر)` at `x=72 y=56 w=190 h=30` and hide `list_meter_unit_lbl`; for English, keep `Length` plus `(mm)` as two separate labels only if they do not overlap. Evidence: `list_after_fix_clean.png` shows `متراژ` and `(میلی متر)` visually merged.
- [ ] CUTLIST-TITLE-01: Apply the user's corrected title-card direction only after a fresh list SHOT confirms it is still needed. Current source is `title_box x=109 y=10 w=201 h=36`; corrected target is `x=112 y=13 w=201 h=33`, so the card moves 3 px right and its top edge moves down 3 px while the bottom stays at y=45. Do not reuse the old "move left" instruction.
- [ ] CUTLIST-PANEL-01: Apply the exact pending white-panel resize in `list_reflow_compact()`: change `objects.panel3_1` from `x=6 y=95 w=458 h=225` to `x=6 y=97 w=464 h=223`. This moves only the top edge down 2 px, keeps the bottom at y=319, and extends the right edge from x=463 to x=469.
- [ ] CUTLIST-BOTTOM-01: Fix the bottom strip without adding overlay objects. Evidence from `list_after_fix_clean.png`: y=316..319 still samples as gray/background, not white row/panel. In `list_reflow_compact()`, inspect parent clipping on `objects.obj22` and `objects.panel3_1`; ensure `objects.obj22` is `480x320`, non-scrollable, and does not clip `panel3_1`. Fresh SHOT target: within x=12..467, y=316..319 must be white panel/row pixels, not gray `(197,194,197)` or background `(230,226,230)`.
- [ ] LIST-HELP-01: Make cut-list help modal block the whole LCD, not only the white card. Add a `list_help_modal` 480x320 clickable top-layer parent/catcher; create `list_help_cont` inside it at the current visual card position (`440x244`, centered with y offset 18). Close must hide/delete both modal and card and clear `GUI.lock_screen`. Evidence: `final_list_help_open.png` leaves header/buttons and row edges visible outside the card.
- [ ] LIST-HELP-02: Remove duplicate/stale text writes inside `list_help_show()`. Keep only the final FA block and final EN block; delete the earlier duplicated `lv_label_set_text(title, ...)` and obsolete body text assignments so future edits cannot silently change the wrong copy.
- [ ] OPT-HEADER-01: Put optimization header buttons fully inside the 480x320 screen. In `opt_open_panel()`, do not use negative y/x for `home`, `help`, `title_box`, or `print_btn`. Target coordinates: `home x=8 y=6 w=44 h=39`, `help x=58 y=6 w=44 h=39`, `title_box x=109 y=10 w=198 h=36`, `print_btn x=316 y=6 w=38 h=38`; then re-place bar/cut stats so they remain readable and do not touch the right rail.
- [ ] OPT-ROW-01: Make optimization row cards square-cornered inside the rounded white list body. In `opt_open_panel()`, change `lv_obj_set_style_radius(row, (b == 0) ? 8 : 0, ...)` to `lv_obj_set_style_radius(row, 0, ...)` for all `opt_bar_rows[b]`. Do not add overlay rectangles. Evidence: `final_opt.png` row 01 has rounded bottom corners while lower rows are square.
- [ ] OPT-HELP-01: Fix optimization help bottom clipping in both FA and EN. In `opt_help_show()`, keep close/title fixed, but give `body_box` safe bottom space: reduce body viewport height by at least 14 px or add at least 18 px bottom padding; fresh SHOT target: no partially cut text on y=300..319. Evidence: `final_opt_help.png` and `opt_help_en.png`.
- [ ] OPT-HELP-02: Add a subtle modal dim to `opt_help_modal`: set modal background to black with about `LV_OPA_20` while keeping it clickable. This makes the help state visually modal without adding timers or per-frame work.
- [ ] OPT-PIECES-01: Compact repeated identical optimization pieces per bar. In `opt_apply_piece_labels()`, collapse consecutive equal sizes into `size * count` before building `done`, `current`, and `todo` strings; example target text: `2000 * 2 - 1500`, not `2000 - 2000 - 1500`. Keep the yellow current-cut pill at the same x/y and keep orange separators with `lv_label_set_recolor(..., true)`.
- [ ] LIST-HEADER-REGRESSION-01: Add a regression check after `QDL`, `QST`, reboot/persist, FA/EN language switch, and list reflow: the list title and both column headers must be non-empty. Evidence: `persist_after_reboot.png`, `stress_list.png`, and `stress_opt.png` show a blank title card and missing Quantity header, while `final_persist_after.png` and `list_after_fix_clean.png` are corrected.
- [ ] SCREENSHOT-QA-01: After the above visual fixes, capture fresh real SHOTs, not simulator images: main FA/EN, F1-F5 tabs FA/EN, F1-F5 help FA/EN, F2 keyboard FA/EN, cut list normal/help/error-toast, optimization normal/help/stress-toast. Use 115200 until 921600 SHOT is reverified.

- [x] Root-cause LCD slowdown by comparing the current display/Task2 path against the fast Metr reference and V.1.5.0, then add a hardware LCD benchmark before applying global optimizations.
- [x] Fix SCR2/settings stack-canary crash and reduce stepper ISR CPU load from pulse `digitalWrite`.
- [x] Add LCD flush-area telemetry to the top overlay so full-screen refresh can be distinguished from normal partial redraw.
- [x] Tune LCD refresh latency with a larger single draw-buffer chunk, LVGL-side RGB565 byte swap, faster refresh period, and lower Task2 idle delay.
- [x] Reduce internal RAM pressure by switching the blocking TFT flush path to a single LVGL draw buffer with safe fallback.
- [x] Add a lightweight always-on top-center performance overlay showing RAM, CPU/load, and FPS across all screens.
- [ ] Continue HMI screen/runtime audit on real hardware after the safe first-pass LCD and NVS optimizations.
- [x] Add QA serial helpers for language, save, reboot, factory reset, status, and buzzer-off testing.
- [x] Program the device and capture Persian/English real-device SHOTs for main, F1-F5, cut list, and cut plan.
- [x] Disable buzzer automatically during QA runs with `BOFF` so automated tests do not beep.
- [x] Fix factory-reset/NVS capacity failure by adding a larger 8 MB partition layout and clearing `params` before writing defaults.
- [ ] Power-cycle and retest touch I2C after seeing `ESP_ERR_INVALID_STATE` in serial logs during reset/runtime QA.
- [ ] Fill 12-row and 100-row cut lists, reboot, and verify all size/count values persist from NVS.
- [ ] Capture active edit states on cut-list size/count fields to verify blue highlight pixel geometry.
- [ ] Capture active cut-list scroll states to verify rows visibly move and deletion keeps the bottom visible.
- [ ] Capture optimization rows with long multi-line piece lists and verify green/red badge vertical centering.
- [ ] Verify optimization print output on hardware after creating a populated cut list.
- [ ] Retest keyboard-only arrows/Enter/Backspace and mouse-only down movement/click/hot-plug on real hardware.
- [ ] Rate-limit or suppress low-level touch I2C error spam so serial QA parsing remains clean.
- [x] Add safe QA commands for help pages, F2 keyboard, demo list, and stress list.
- [x] Fix `QST` / `QSTA` serial command collision.
- [x] Make committed cut-list length/count values save immediately to NVS.
- [x] Suppress Arduino core debug spam in production build with `CORE_DEBUG_LEVEL=0`.
- [x] Verify cut-list values survive reboot after direct saved entry/demo list.
- [x] Capture F2 keyboard on hardware and inspect the OK/backspace visual treatment.
- [x] Capture cut-list help and optimization help on hardware.
- [ ] Implement `SETTINGS-HELP-01` and `SETTINGS-HELP-02`; then retake all F1-F5 help pages in Persian and English.
- [ ] Implement `F2-KB-01`; then retake F2 keyboard SHOT in Persian and English.
- [ ] Implement `F-TABS-01`; then verify the active right-side F1-F5 tab state on every settings tab.
- [ ] Implement `OPT-HELP-01` and `OPT-HELP-02`; then retake optimization help in Persian and English.
- [ ] Implement `OPT-PIECES-01`; repeated equal sizes must render as `size * count`, not repeated strings.
- [ ] Upgrade optimization from First-Fit Decreasing heuristic to exact bounded optimization for small/medium jobs, with a fast fallback for large jobs.
- [ ] Add optimization stress scenarios: empty, oversized, 5-row demo, 100x1 rows, near-500-bar high count, and over-capacity list.
- [ ] Add one-shot retry to `tools/qa_hmi.py` when SHOT times out after navigation or reboot; retry once after 1200 ms and record both attempts in the QA log.
- [x] Optimize safe refresh paths so LVGL updates only changed regions where possible and Task2 reduces CPU load after hardware validation.
- [ ] Investigate guarded DMA or higher-safe SPI throughput for true full-screen 32 FPS; raw blocking SPI full-screen benchmark is currently about 23 FPS.
- [x] Build and upload the current LCD/NVS responsiveness optimization firmware to COM4.
- [x] Fix list-capacity drift between firmware, web portal, simulator, and documentation/tests.
- [x] Fix web portal row-count limits to match firmware count limits.
- [x] Repair broken F1 layout-check tooling without touching the active locked F1 UI implementation.
- [x] Document remaining hardware-only FPS/SHOT validation required after code changes.
- [x] Move firmware build logs out of `firmware/` and into `artifacts/logs/build/`.
- [x] Remove generated build cache and the temporary `C:\fwbuild_metr` folder after preserving the uploaded binary.
- [x] Initialize local git history and create the protected project snapshot commit `75edaac` so source loss can be traced and reverted safely.
- [x] Add strict multi-agent, token-budget, build/upload, and documentation rules.
- [x] Add an English agent development prompt for future autonomous project work.
- [x] Add current known project risks and follow-up problems to the work list.
- [ ] Implement `SCREENSHOT-QA-01` after the next uploaded visual firmware.
- [ ] Implement `CUTLIST-TITLE-01`; do not apply the old left-direction title-card instruction.
- [x] Cut-list main white rows panel with all row contents: move the full panel/content group 2 px left so it is centered on the LCD; verify left/right margins on a fresh SHOT.
- [ ] Implement `OPT-ROW-01`; verify row 01 bottom corners are straight in a fresh optimization SHOT.
- [ ] Implement `CUTLIST-BOTTOM-01`; verify y=316..319 inside the list panel is no longer gray/background.
- [x] Help button book icon: move the icon 1 px left so it is optically centered inside the button.
- [ ] Implement `CUTLIST-PANEL-01` and `CUTLIST-BOTTOM-01`; investigate parent/display clipping instead of adding a risky overlay object.
- [ ] Retest USB mouse/keyboard hot-plug switching after the latest uploaded firmware.
- [ ] Retest mouse downward movement with the user's dongle mouse.
- [ ] Retest direct SHOT at 921600 after the latest uploaded firmware; keep 115200 as known-safe fallback until verified.
- [ ] Create a stable build/upload script that performs process checks, UTF-8 esptool, artifact copy, and log cleanup automatically.
- [ ] Implement `LIST-HELP-02`; remove duplicated/stale help text writes only after preserving the final FA/EN strings.
- [x] Make optimization help modal block home/print/list actions behind it.
- [x] Keep optimization help close/header fixed while only the body scrolls.
- [x] Rewrite optimization help Persian/English copy with font-safe text and document the tofu/square cause.
- [x] Enlarge the optimization help white panel 10 px downward.
- [x] Tune optimization row green/red badges, orange separators, vertical centering, and scrollbar position.
- [x] Pixel-tune optimization header home/help/print buttons.
- [x] Move and resize the optimization white title card and center its title.
- [x] Adjust optimization cut/bar stat labels and current-item text upward/left.
- [x] Move optimization list body, row cards, badges, and scrollbar to the requested positions.
- [x] Improve cut-list scrolling feel so rows visibly move instead of only changing numbers.
- [x] Move the cut-list white panel 2 px right, move the millimeter unit 2 px up, and tune blue edit highlights.
- [x] Keep the cut-list help close button/header fixed while only the help body scrolls.
- [x] Fix the cut-list help length-entry wording/formatting issue.
- [x] Make cut-list range errors use stock length/initial position and show only the active language.
- [x] Keep minus/delete row flow scrolled to the bottom so the operator sees rows being removed.
- [x] Confirm cut-list values still save to NVS without changing the existing save path.
- [ ] Implement `CUTLIST-PANEL-01`; exact target is `objects.panel3_1 x=6 y=97 w=464 h=223`.
- [x] Move the whole cut-list panel and its contents 2 px left.
- [x] Make the cut-list title white card 5 px wider to the right.
- [x] Revert the bad cut-list header text movement while keeping the requested list panel sizing.
- [x] Capture the optimization screen from hardware and use it for the next visual pass.
- [x] Add the missing optimization print button on the right side of the white panel.
- [x] Move optimization waste badge to the right side of each row.
- [x] Increase optimization green/red badge height by about 8 percent.
- [x] Clean optimization top controls and remove trailing piece separators.
- [x] Polish the optimization screen badges, skip color, print button, and piece-list punctuation.
- [x] Add a serial-safe optimization screenshot entry path for hardware SHOT verification.
- [x] Add a serial `OPT` command to open the optimization panel safely from Task2.
- [x] Build and upload the updated firmware.
- [x] Tune optimization header controls: move home/help slightly left, stats right, and print button beside the title card.
- [x] Fine tune optimization list upward, row frame right, and equal spacing for top-right items.
- [x] Polish cut-list header spacing and millimeter unit label.
- [x] Fix bottom cut-list row clipping on the 480x320 LCD.
- [x] Verify cut-list layout with the safest available test after visual edits.
- [x] Fix USB mouse downward movement by making HID mouse report parsing handle report-ID and 16-bit axis packets.
- [x] Verify mouse parser still keeps keyboard packets from injecting digits.
- [x] Restore the full 219KB `ui_extras.h` that contains today's F1-F5 UI work.
- [x] Re-apply the list-screen row/highlight geometry fixes onto the full `ui_extras.h`.
- [x] Verify F1/F2 helper code and list layout values exist in the final active file.
- [x] Disable `ui_compat.h` fallback bodies when the full settings module is present.
- [x] Restore the pre-restore `ui_extras.h` that contains the latest list-screen fixes.
- [x] Back up the currently reverted `ui_extras.h` before replacing it.
- [x] Verify the restored list-screen layout values and run the safest available UI test.
- [x] Fix USB mouse reports being interpreted as keyboard input.
- [x] Hide the mouse cursor until a mouse actually moves or clicks.
- [x] Improve USB mouse Y-axis handling for normal and report-ID mouse packets.
- [x] Add keyboard focus navigation with arrow keys and Enter selection.
- [x] Add Backspace page-back behavior that returns toward the main screen.
- [x] Document keyboard/mouse hardware validation scenarios.
- [ ] Build/upload and hardware-test full mouse/keyboard switching scenarios.
- [x] Capture a real list-screen SHOT for the requested list layout review.
- [x] Move the blue selected-number highlight upward and reduce its visual height.
- [x] Shorten the right side of each list row so the background remains visible at the screen edge.
- [x] Verify list layout with the safest available test after edits.
- [ ] Build/upload and take a new real list-screen SHOT to confirm the visual change on hardware.
- [x] Add USB mouse pointer support on the LCD.
- [x] Route left mouse click through LVGL as a normal touch/click.
- [x] Keep right/middle/wheel mouse actions inert for machine safety.
- [ ] Verify mouse behavior with build and real hardware after user approval.
- [ ] Build and upload have not been run for the latest USB keyboard hot-plug fix.
- [ ] Retest USB keyboard disconnect/reconnect on real hardware after upload.
- [ ] Retest SHOT at 921600 after firmware with `SERIAL_DEBUG_BAUD = 921600` is uploaded.
- [ ] Continue monitoring large optimization calculations for UI freeze/reset.
- [ ] Verify optimization print button code with a build when the user allows building.
- [x] Captured current hardware screen for the optimization review request; device was showing the homing modal, not the optimization panel.
- [x] Added hardware LCD benchmark command `LCDB` for raw SPI/DMA throughput checks.
- [x] Proved TFT DMA on target hardware: about 42 FPS full-screen versus about 23 FPS blocking SPI.
- [x] Enabled guarded DMA LVGL flush path and uploaded the firmware to COM4.

## Recently Completed In Code

- [x] Adjusted the cut-list header and compacted the list rows so the bottom row no longer sits on the LCD edge.
- [x] Made USB mouse report parsing handle report-ID packets and 16-bit X/Y axis layouts so downward movement is not lost on dongle mice.
- [x] Restored the full F1-F5-capable `ui_extras.h` and kept the list-screen geometry fixes on top of it.
- [x] Added `UI_EXTRAS_FULL_SETTINGS_MODULE` guard so `ui_compat.h` does not redefine fallback settings functions when the full module is active.
- [x] Restored `firmware/include/app/ui_extras.h` from `tools/_recovery_inspect/before_restore_20260714_232340/` after detecting it had been replaced by the `from_rar_main` copy.
- [x] Disabled raw USB transfer keyboard parsing in `Task4.h` so mouse packets cannot type digits.
- [x] Added hidden-until-used mouse cursor behavior and USB disconnect reset.
- [x] Added LVGL keypad/group navigation for keyboard arrows and Enter.
- [x] Added Backspace page-back behavior outside active numeric edits.
- [x] Improved mouse report parsing for normal boot reports and likely report-ID packets.
- [x] Tuned list row width, scrollbar position, and edit-highlight geometry in `firmware/include/app/ui_extras.h`.
- [x] Added a second LVGL pointer input for USB mouse movement/clicks in `firmware/include/app/GUI_Func.h`.
- [x] Added a visible mouse cursor on the LVGL top layer.
- [x] Connected USB mouse move and left-button events in `firmware/include/tasks/Task4.h`.
- [x] Fixed USB boot mouse report parsing in `libraries/EspUsbHost/src/EspUsbHost.cpp`.
- [x] Added USB host recovery after keyboard disconnect/reconnect in `libraries/EspUsbHost/src/EspUsbHost.cpp`.
- [x] Scheduled recovery when `usb_host_device_open` fails during enumeration.
- [x] Scheduled recovery when active config descriptor read fails during enumeration.
- [x] Preserved cut-list cell value when touched; it no longer visually turns into zero before typing.
- [x] Added immediate save while typing cut-list length/count values.
- [x] Added auto-save when cancelling an edit restores the snapshot.
- [x] Committed active cut-list edit before optimization starts.
- [x] Delayed optimization progress modal until about 2 seconds.
- [x] Added optimization cancel/progress flow scaffolding.
- [x] Added optimization print request path and print button in source.
- [x] Virtualized optimization list rows to reduce LVGL object load.
- [x] Limited count entry to 999.
- [x] Set optimization source limits to 500 bars and 4000 pieces.
- [x] Documented USB flash-drive limitations: CP210x/UART cannot read USB flash without a real USB host path.

## Recently Completed In Documentation

- [x] Organized root-level build/upload logs, screenshots, serial captures, raw SHOT dumps, and stress-test data under `artifacts/`.
- [x] Added `artifacts/README.md` to document artifact folder categories.
- [x] Added folder and artifact placement rules to `docs/PROJECT_RULES.md`.
- [x] Added `docs/AGENT_START_HERE.md` as the first-read handoff for new agents.
- [x] Added `docs/PROJECT_RULES.md` for communication, code, build/upload, HMI, safety, and documentation rules.
- [x] Added `docs/TECHNICAL_KNOWLEDGE.md` for live technical knowledge.
- [x] Added `docs/SKILLS_AND_TOOLS.md` for required Codex skills, local Cursor skills/rules, and commands.
- [x] Added `docs/INDEX.md` as the English documentation index.
- [x] Replaced the root `README.md` with a short English pointer to the new docs.

## Recurring Visual Verification Items

- [ ] Take a real list-screen SHOT after any list UI layout change.
- [ ] Take a real optimization-screen SHOT after any optimization UI layout change.
- [ ] Check Persian and English label positions after language-related layout changes.
- [ ] Check no text is clipped, especially Persian labels with descenders or parenthesized units.
- [ ] Check scrollbar visibility and position on both list and optimization screens.

## Template For New Requests

## Rejected July 19 DMA Candidate And Retained Build Workflow

- [x] Keep splash removal and the first main-screen paint on blocking SPI.
- [x] Enable DMA only after a proven blocking boot frame.
- [x] Reject DMA-device hardware CS at 80 MHz after the physical panel froze.
- [x] Add bounded DMA completion monitoring and safe software-state diagnostics; raw SPI register reads were removed after hardware boot proved they could crash.
- [x] Restart once into blocking fallback if DMA hardware ever stalls.
- [x] Document the stable short-path incremental build workflow.
- [x] Add cache-safe fast build/direct-upload helper.
- [x] Upload, capture boot logs, run LCDB, and verify a fresh SHOT.
- [x] Confirm the new 40 MHz FSPI/manual-CS path boots and progresses on the physical panel.
- [ ] Confirm it remains stable under real motor-motion stress.

Copy this section when a new user request arrives:

- [ ] Read the affected files.
- [ ] Add exact requested changes here.
- [ ] Apply narrow code changes.
- [ ] Update technical knowledge if something new was learned.
- [ ] Verify in the safest available way.
- [ ] Mark completed items `[x]`.
- [ ] Tell the user what changed and what was not tested.
# Five-key panel navigation (active)

- [x] PANEL-NAV-01: MODE enters an ordered visible-control navigation mode on every interactive screen and modal; pressing MODE after the final page item exits, and the next press restarts at the first item.
- [x] PANEL-NAV-02: Use one clear low-cost blue focus indicator; never add per-tick object creation, animation, or full-screen invalidation.
- [x] PANEL-NAV-03: In navigation mode START confirms buttons and increases adjustable values; STOP decreases adjustable values and deletes one character on a keyboard.
- [x] PANEL-NAV-04: When focus reaches an on-screen keyboard, MODE cycles keyboard keys continuously from first to last and wraps inside the keyboard.
- [x] PANEL-NAV-05: SAVE keeps the screen's real save behavior; CANCEL closes/exits safely. Touch or F1/F2/F4/F5 exits navigation without triggering the selected control.
- [x] PANEL-NAV-06: Preserve physical STOP emergency behavior and all motion/jog/cut/start1/start2 paths outside navigation mode.
- [ ] PANEL-NAV-07: Hardware-verify coverage for main, settings F1-F5, cut list, optimization, homing, lock/update keyboards, help, error, confirmation, and toast/modal overlays after approved build/upload.
