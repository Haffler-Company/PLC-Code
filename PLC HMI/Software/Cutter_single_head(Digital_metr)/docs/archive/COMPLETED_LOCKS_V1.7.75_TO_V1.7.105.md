# Completed Locks Archive (V1.7.75 to V1.7.105)

Archived on 2026-08-25 to keep `docs/WORKLOG.md` ultra-compact for zero-waste token consumption.

- `COMPLETE-F2-F3-F4-OOBE-V1.7.105` (completed; manager=primary):
  (1) Company Name Screen Redesign (`oobe_wizard.h`): Removed top header bar and step 1 pill; created a large, spacious card (456x136) at pos (12, 12) with centered title ("نام شرکت یا کارگاه را وارد کنید (حداکثر ۲۲ کاراکتر)"), spacious centered textarea (275x46), and blue "مرحله بعد" button (135x46); upon clicking Next, saves company name, marks OOBE done, and navigates straight to Settings Screen 2 Tab F1.
  (2) Device Lock PIN Counter Bugfix (`admin_ui.h`): Removed duplicate `LV_EVENT_READY` registration on `dev_lock_ta` which caused double-triggering of `dev_lock_try_unlock()` and incremented the fail count to 2 on the first wrong attempt.
  (3) F4 Password Card & Keypad Polish (`admin_ui.h`): Shifted "رمز جدید" and "تکرار رمز" labels 8px down (y=-8), eye button 5px down (y=-11), and textareas 4px up (y=12, centered inside bar); integrated Backspace (left) and OK checkmark (right) directly into the `f4_kb` buttonmatrix with colored draw callbacks and removed separate external bottom buttons.
  (4) F2 Layout & Seek Polish (`ui_extras.h`): Shifted limit buttons 3px right, text 4px down, and numeric boxes 5px left; active seek button color set to Yellow (`0xeab308`), updated toasts ("موتور در حال حرکت به ابتدای/انتهای خط" and second tap "موتور در بین راه ایستاد").
  (5) F3 Blade Angle 3-State Cycling (`GUI_Func.h`, `admin_ui.h`): Configured exact 3-state cycling (45°, -45°, 90° free) on Q8 (`IOS.sawl_ift_pin`) and Q9 (`IOS.q9_pin`) outputs; aligned "زاویه تیغه" label straight up with other keys, and aligned Q8/Q9 badge labels to `LV_ALIGN_TOP_LEFT`.

- `F2-F3-F4-IMPROVEMENTS-V1.7.103` (completed, built 22:03 & flashed 2026-08-24 22:03 COM8 full factory NVS erased, BOOT_VERIFY_OK; manager=primary):
  (1) USB Keyboard Detection (`EspUsbHost.cpp`): Fixed interface claim loop logic (`cur_intf_claimed`) so composite/multi-interface HID keyboards and receivers are properly parsed and endpoints allocated without being dropped by non-HID interface descriptors.
  (2) Fresh Boot Direct Setup Navigation (`ui_extras.h`, `oobe_wizard.h`): On fresh install / erased NVS, system bypasses positioning screen 0 and goes straight to the Company Name wizard on screen 1, then transitions directly to tab F1 upon completion.
  (3) F2 Layout & Seeking Polish (`ui_extras.h`): Shifted Start/End size buttons 3px RIGHT, button text 4px DOWN, and numeric boxes 5px LEFT; set active seeking color to Yellow (`0xeab308`), updated toasts ("موتور در حال حرکت به ابتدای/انتهای خط" and second tap "موتور در بین راه ایستاد").
  (4) F3 Blade Angle Alignment (`admin_ui.h`): Aligned "زاویه تیغه" label straight UP (Y offset = 0) with other key pairs; verified 3-state blade angle cycling (45°, -45°, 90° free) on Q8/Q9 outputs with dynamic pinmap support and long-press pin mapping dialog.
  (5) F4 Admin PIN Notice (`admin_ui.h`): Refined factory default PIN warning toast text ("توجه: رمز مدیر ۱۲۳۴۵۶ است. جهت امنیت آن را تغییر دهید").
  (6) Version bumped to 1.7.103; build and flash SUCCESS (BOOT_VERIFY_OK).

- `OOBE-LAYOUT-V1.7.102` (completed, built 21:29 & flashed 2026-08-24 21:30 COM8 app0, BOOT_VERIFY_OK; manager=primary):
  (1) Step 1 Layout Alignment (`oobe_wizard.h`): Shifted "نام شرکت یا کارگاه" label exactly 13px UP (`Y = -17`), centered textarea text without vertical scrolling/jumping (`LV_TEXT_ALIGN_CENTER` + single line), and replaced button label text with "مرحله بعد" (removing unsupported Unicode arrow character that rendered as a square box).
  (2) Streamlined Single-Step Setup Flow (`oobe_wizard.h`): Removed redundant modal steps 2..5; clicking "مرحله بعد" in Step 1 now immediately saves company name, marks OOBE complete (`g_oobe_done = true`), closes modal wizard, displays toast notification, and navigates directly to the workspace.
  (3) Version bumped to 1.7.102; build and flash SUCCESS (BOOT_VERIFY_OK).

- `OOBE-USB-KBD-BOOT-V1.7.100` (completed, built 19:44 & flashed 2026-08-24 19:45 COM8 app0, BOOT_VERIFY_OK; manager=primary):
  (1) Direct Lock Screen Boot Transition (`ui_extras.h`, `admin_ui.h`): Eliminated split-second flash of screen 0 ("موقعیت‌یابی") on boot by rendering lock screen before dismissing splash container; upon successful unlock, seamlessly directs machine to positioning screen 0 if unreferenced.
  (2) USB Keyboard in First-Boot Wizard (`oobe_wizard.h`, `Task4.h`): Added `oobe_is_active`, `oobe_try_char`, `oobe_try_backspace`, `oobe_try_enter` and hooked into `usb_keyboard_ui_drain()`, enabling physical USB keyboard typing and Enter step advancement in Company Name, Profile Essentials, and Admin PIN setup on fresh installs.
  (3) Version bumped to 1.7.100; build and flash SUCCESS (BOOT_VERIFY_OK).

- `DEV-LOCK-USB-KBD-V1.7.98` (completed, built 19:01 & flashed 2026-08-24 19:07 COM8 full factory NVS erased, BOOT_VERIFY_OK, SHA256 `DF7BAD5CFD1D384EDCD14B6C418C66BBE8B627677D93E8ECA818814EB6D02768`; manager=primary):
  (1) Device Lock Recovery Number Layout (`admin_ui.h`): Moved `dev_lock_challenge_lbl` 4px down (`Y = 108` instead of `104`) for balanced vertical spacing.
  (2) Default Focus for Numeric Entry (`admin_ui.h`): `dev_lock_ta` given `LV_STATE_FOCUSED` on creation, unhide, and lockout reset so keypad and USB numbers are received immediately without needing to touch the text box first.
  (3) USB Keyboard Generic HID Detection (`EspUsbHost.cpp`): Added `HID_ITF_PROTOCOL_NONE` (0x00) interface claiming, report size-based keyboard/mouse differentiation, and dynamic dispatch in `_onReceive`, restoring functionality for keyboards and wireless dongles that do not report fixed boot protocol.
  (4) Lock Screen Fail Counter Reset on Correct Password (`admin_ui.h`): `dev_lock_security_clear()` explicitly resets `dev_lock_fail_count = 0`, clears `dev_lock_last_fail_shown = 0`, hides `dev_lock_err` ("رمز اشتباه"), and clears `dev_lock_ta` so stale fail counts never persist across successful unlocks.
  (5) Version bumped to 1.7.98; build SUCCESS (`BUILD_OK`, flash 45.0%, RAM 39.2%).

- `F3-PINMAP-F1-OPT-V1.7.95` (completed, built & flashed 2026-08-24 18:43 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary):
  (1) F3 Pin Mapping Lag Elimination (`admin_ui.h`): Implemented in-place badge update fast-path (`f3_pinmap_sync_badge_text`) in `f3_pinmap_reflow()`. Removed `f3_pin_wired = false` on every click, eliminating the massive loop that previously deleted and recreated ~150 LVGL objects upon selecting each pin mapping.
  (2) F1 Cutting Optimization Virtual Scroll Fix (`ui_extras.h`): Restricted `opt_bar_page_gesture_cb` to horizontal gestures (`LV_DIR_LEFT` / `LV_DIR_RIGHT`) and tightened scroll-end threshold to `<= 2` with 500ms guard in `opt_list_scroll_end_cb`, eliminating accidental multi-page jumping during vertical scrolling.
  (3) Structure & Font Decl Cleanups (`ui_extras.h`, `Task3.h`, `oobe_wizard.h`, `GUI_Func.h`): Fixed typo struct member in `Task3.h`, moved limit seek helper functions before reflow, added `panel_five_nav_prev/next` in `GUI_Func.h`.
  (4) Version bumped to 1.7.95; build & flash SUCCESS (BOOT_VERIFY_OK).

- `PERF-CACHE-DMA-OPT-V1.7.94` (completed, built & flashed 2026-08-24 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary):
  (1) LVGL Font/Image/Style Caching (`lv_conf.h`): Enabled `LV_CACHE_DEF_SIZE` (32KB), `LV_IMAGE_HEADER_CACHE_DEF_CNT` (32), `LV_OBJ_STYLE_CACHE` (1), `LV_DRAW_SW_SHADOW_CACHE_SIZE` (4).
  (2) Internal SRAM DMA Buffers (`GUI_Func.h`): Upgraded `dual_fast_line_try[]` from `{ 32, 24, 16 }` to `{ 48, 32, 24, 16 }` (48-line dual buffer DMA streaming).
  (3) Version bumped to 1.7.94; build & flash SUCCESS.

- `F1-PRINTER-F2-LIMITS-F3-ANGLE-V1.7.93` (completed, built & flashed 2026-08-24 16:44 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary):
  (1) Printer Hardware Models: Explicitly listed selectable hardware models `ESC/POS حرارتی (۵۸ میلی‌متر)` and `CSN-A2 حرارتی (TTL)` in the printer settings modal with instant feedback.
  (2) Auto-Cut Button & Trim Modal: Unified default OFF state (gray `#f1f5f9`) and active ON state (blue `#0284c7`), with refined `460x290` modal layout and clean text wrapping.
  (3) F2 Limit Seek Calibration: Turned `btn2_3` ("سایز اول خط") and `btn2_4` ("سایز آخر خط") into active seek buttons (half speed approach, slow 1/6th speed back-off upon sensor trigger, auto color reset).
  (4) F3 Blade Angle & Sensor Diagnostics: Converted lamp button to "زاویه تیغه" (Q8,9) with cycling $+45^\circ \to -45^\circ \to 90^\circ \to \text{آزاد}$ and state restoration on tab leave; renamed motor fault LED to "سنسور بالا تیغه".
  (5) Version bumped to 1.7.93; build & flash SUCCESS.

- `LCD-PERF-OPT-02` (source-only, verified 2026-08-24; manager=primary):
  (A) SPI Frequency (`User_Setup.h`): 40MHz → 80MHz; frame transfer 2× faster.
  (B) Flush Debug Log removed (`GUI_Func.h`): حذف `ets_printf` از flush callback؛ ~1ms blocking از هر فریم برداشته شد.
  (C) shadow reductions in `ui_extras.h` and `admin_ui.h`.

- `F1-WIFI-INFO-QR-PRINTER-V1.7.92` (completed, built & flashed 2026-08-24 14:16 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary)
- `F1-WIFI-QR-HELP-COLORS-V1.7.91` (completed, built & flashed 2026-08-24 13:55 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary)
- `F1-FACTORY-AUTOCUT-TYPOGRAPHY-V1.7.90` (completed, built & flashed 2026-08-24 13:21 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary)
- `F4-GATE-ALL-KEYPADS-BUZZ-V1.7.89` (completed, built & flashed 2026-08-24 13:15 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary)
- `FULL-TOUCH-SLIDER-NAV-BUZZ-V1.7.88` (completed, built & flashed 2026-08-24 12:45 COM8 app0 NVS preserved, BOOT_VERIFY_OK; manager=primary)
- `TONE-STEP-SPLASH-RESTORE-V1.7.87` (completed, built & flashed 2026-08-24 12:32 COM8 app0 + SPIFFS @0xE30000 NVS preserved, BOOT_VERIFY_OK)
- `RICH-BUZZER-PATTERNS-V1.7.86` (completed, built & flashed 2026-08-24 12:26 COM8 app0 NVS preserved, BOOT_VERIFY_OK)
- `EMG-STOP-FLAG-BUZZ-V1.7.85` (completed, built & flashed 2026-08-24 12:13 COM8 app0 NVS preserved, BOOT_VERIFY_OK)
- `JOG-BUZZ-SETTINGS-GUARD-V1.7.84` (completed, built & flashed 2026-08-24 12:08 COM8 app0 NVS preserved, BOOT_VERIFY_OK)
- `F1-BUZZ-CONFIRM-V1.7.83` (completed, built & flashed 2026-08-24 12:01 COM8 app0 NVS preserved, BOOT_VERIFY_OK)
- `WIFI-QR-BUZZ-USB-V1.7.82` (completed, built 2026-08-23 21:50, flashed 22:05 COM8 app0 NVS preserved, BOOT_VERIFY_OK)
- `OTA-LINKS-BIN-V1.7.81` (completed, built & flashed 2026-08-23 20:40, BOOT_VERIFY_OK)
- `STOP-TOAST-NAVKILL-V1.7.80B` (completed 2026-08-23, source+judge verified)
- `MAINSCR-STOP-PRN-BATCH-V1.7.80` (completed, built & flashed 2026-08-23 12:20)
- `OPT-SCREEN-FIX-BATCH-2` (completed, built & uploaded V1.7.79)
- `OPT-SCREEN-FIX-BATCH-1` (completed, built & uploaded V1.7.78)
- `PASSWORD-RULE-EXPLANATION-V1.7.77` (completed, verified, built & uploaded 2026-08-21)
- `LCD-PERF-OPT-01` (source-only, 2026-08-21)
- `DEV-LOCK-RECOVERY-Y-V1.7.76` (completed & verified 2026-08-21)
- `F1-WIFI-BUZZER-AUTOCUT-V1.7.75` (completed, verified, built & uploaded 2026-08-20)
