# قفل ویرایش موازی — تا اطلاع بعدی

| فیلد | مقدار |
|------|--------|
| **وضعیت** | فعال |
| **نقش عامل** | طراح محصول (gstack) + Apple HIG |
| **کار** | اصلاح ظاهر صفحهٔ تنظیمات عمومی (F1) |
| **آپلود/پروگرام** | ممنوع تا تأیید — کد V.1.5.20 آماده (بیلد OK) |

## فقط این ناحیه را من ویرایش می‌کنم
- `firmware/include/app/ui_extras.h` — توابع F1: `f1_*`, `vkb_settings_*`, `numpad_style_f1`, `settings_*` مربوط به تنظیمات
- `VERSION` / `version.h` (bump آمادهٔ فلش بعدی)
- اسناد طراحی در `docs/` اگر لازم باشد

## لطفاً شما فعلاً دست نزنید به
- همان بخش‌های F1 داخل `ui_extras.h`
- ترجیحاً کل `ui_extras.h` تا وقتی این قفل باز شود

## برای کار موازی شما امن است
- `Task3` / `Task8` / حرکت / ایمنی
- لیست برش (به‌جز اگر عمداً همزمان لیست بخواهید)
- `screens.c` EEZ (من عمداً دست نمی‌زنم)
- وب‌پورتال، سریال ایمپورت

وقتی تأیید دادید «آپلود کن»، قفل را برمی‌دارم یا به‌روز می‌کنم.

## Parallel runtime scope — DMA hardening

- Owner: current root agent.
- Files: `GUI_Func.h`, `main.ino`, `Task2.h`, `main_position_fast.h`, `TFT_eSPI/User_Setup.h`, the ESP32-S3 TFT_eSPI DMA declaration/implementation, build helper, and performance documentation.
- Excluded: all F1 sections and all edits inside `ui_extras.h`.
- Current evidence: physical LCD freezes while UART/SHOT stay alive; driver transport only.
- July 25 destination-redraw experiment is closed and reverted; no further speed experiment is active.

## Current lock — F1 through F5 comprehensive pass (2026-07-29)

- Owner: current root agent.
- Scope: `ui_extras.h`, `admin_ui.h`, `GUI_Func.h`, `Task2.h`, settings help/modal/navigation code, version/changelog, and SHOT tooling.
- Purpose: F1 keyboard transition, F2/F4 pixel layout, F3 pin dialogs/picker freeze, F5 stats/warranty/freeze, settings long-press help, and five-key coverage.
- Excluded: `Task4.h`, `libraries/EspUsbHost/**`, stepper motion, pin polarity, homing, cut sequencing, and unrelated list/optimization behavior.
- Release: one controlled build/upload only after all items pass static/simulator review.

## USB hot-plug extension — owner authorized (2026-07-29)

- Owner explicitly requested «کیبرد USB» reconnect and lock-password input repair.
- Scope added only for this release: `Task4.h` and `libraries/EspUsbHost/**`.
- Motion, pin polarity, homing, and cut sequencing remain excluded.
