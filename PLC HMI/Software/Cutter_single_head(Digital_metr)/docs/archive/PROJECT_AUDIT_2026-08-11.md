# Project Audit — 2026-08-11

وضعیت: ممیزی فقط‌خواندنی کامل شد؛ اجرای اصلاح‌ها منتظر تأیید مالک است. در این ممیزی firmware تغییر نکرد، build/upload اجرا نشد و هیچ فرمان حرکتی، USB یا PIN ارسال نشد.

## دامنه و شواهد

- کد و سناریو: `GUI_Func.h`، `Task2.h`، `ui_extras.h`، `webportal.h`، `screen_server_page.h`، `main.ino` و `simulator/scenarios.json`.
- سخت‌افزار: SHOT واقعی 480x320 از COM10 برای Main، List، F1..F5، Helpهای F1..F5، List/Optimizer Help و تلاش ورودی F2؛ فایل‌ها در `artifacts/screenshots/2026-08-11-audit-*.png`.
- اندازه‌گیری واقعی بدون حرکت: فرمان‌های `MEM` و `PERF` روی COM10.
- بررسی خودکار: `check_ui_perf_guard.py` و release-note validator پاس؛ simulator برابر 24/24 پاس با هشدار drift در F1 designer؛ `git diff --check` پاس.
- آخرین build موجود (این ممیزی آن را اجرا نکرد): RAM استاتیک 115300/327680 = 35.2% و Flash برابر 2965862/7340032 = 40.4%.
- وب live در `192.168.4.1` از رایانهٔ ممیزی قابل‌دسترسی نبود؛ نتیجهٔ وب بر پایهٔ source و اسناد رسمی است، نه screenshot زنده.

## یافته‌های اولویت‌دار

### P0 — هم‌زمانی WebPortal با ماشین

`portal_api_settings_set()` و `portal_api_list_set()` در task وب، پس از یک busy-check لحظه‌ای، مستقیماً `STP`، `GUI`، `AUT` و Preferences را تغییر می‌دهند (`webportal.h:343-495`). Task2/Task3 همان داده‌ها را هم‌زمان مصرف می‌کنند و command queue/mutex واحدی وجود ندارد. اثر ممکن: snapshot ناهماهنگ، تغییر پارامتر در مرز شروع ماشین و NVS write هم‌زمان. اصلاح پیشنهادی: callback وب فقط command ثابت و bounded را enqueue کند؛ یک owner در UI/idle آن را دوباره validate، apply و persist کند.

### P1 — OTA محلی و PIN پیش‌فرض

PINهای اولیه `1234` و `123456` هستند (`main.ino:112-113`، `Pref_Func.h:548-549,662-664`). مسیر محلی `/update` پس از PIN مستقیماً به `Update.begin/write/end` می‌رسد (`webportal.h:548-613`) و امضای image/manifest مورداعتماد ندارد. lockout سه خطا/ده دقیقه وجود دارد، اما بعد از factory reset حدس‌پذیری PIN و firmware بدون امضا باقی است. تغییر سیاست PIN اجباری یا signed image نیازمند تصمیم مالک و هماهنگی release/server است.

### P1 — حاشیهٔ حافظه

`MEM` واقعی: internal heap آزاد 95132B و largest block برابر 47092B؛ این مقدار از خط قرمز 100KB پروژه پایین‌تر است. Stack high-water پایین: Task10=464B، Task9=504B، Task5=520B، Task6=548B و Task7=572B. هنوز crash یا overrun مشاهده نشد؛ قبل از کم‌کردن/زیادکردن stack باید همین مقادیر روی مسیرهای stress واقعی دوباره اندازه‌گیری شوند.

### P1 — مسیرهای HMI بدون شاهد کامل

- Main واقعی: آیکن gear در لبهٔ راست بریده دیده می‌شود (`2026-08-11-audit-main.png`).
- Help F1/F2: header/title در SHOT واقعی clip می‌شود؛ F2 عنوان ناقص دیده می‌شود.
- Help F4 در locked-state پشت password screen قرار گرفت؛ این مسیر QA شاهد معتبرِ help ایجاد نمی‌کند.
- فرمان QA برای بازکردن keyboard در F2 صفحه‌کلیدی نشان نداد (`2026-08-11-audit-f2-keyboard.png`). مسیر input باید با touch واقعی و state/focus مشخص بررسی شود.
- Splash/Reference page عمداً با reboot/SCR0 باز نشد، چون می‌تواند وارد مسیر مرجع‌گیری حساس شود.

### P1 — پوشش سناریویی HMI ناکافی

`scenarios.json` منطق machine/list را پوشش می‌دهد، اما screen/tab/modal/input assertion ندارد. پوشش گمشده: ورود/Back/Cancel، help locking، numpad/keyboard/focus، F1..F5 Save/Cancel/persistence، F3 pin-map/stepper modal، F4 authentication/lockout، F5 overlays، list purge/print/help و factory-reset/serial-import timeout. تغییر shared helper بدون سناریوی ورود، action، Cancel، خروج/بازگشت و ثبات `lock_screen`/focus قابل تأیید نیست.

### P2 — کارایی نمایش

- Main idle: CPU=10..11%، RAM=66%، redraw=0؛ redraw بیهوده دیده نشد.
- Settings idle: CPU=8..9%.
- تعویض tab: جهش کوتاه CPU تا F1=66%، F2=77%، F3=92%، F4=59% و F5=74%.
- F5 پس از ورود، redraw دوره‌ای با CPU حدود 36% و RF حدود 42% دیده شد؛ نیازمند impact-map دقیق clock/status است، نه حذف کور refresh.
- `check_ui_perf_guard.py` پاس است؛ guard نباید تضعیف شود.

### P2 — Web app: قابلیت‌های موجود و کمبودها

- فعال: Home، F1..F5، List، Update، Help/Confirm؛ status هر ثانیه read-only sync می‌شود؛ write با PIN و busy gate انجام می‌شود.
- Import فعلی فقط `.txt/.csv/.text` را client-side می‌خواند و تا 100 ردیف پر می‌کند؛ XLSX، export/download، history و preview خطا ندارد.
- import فاقد schema/version، line-level reject، valid/rejected summary و تصمیم صریح replace/merge است.
- optimized mode فقط status است؛ API برای plan/bars/pieces/waste/completion وجود ندارد. پیشنهاد: snapshot read-only و download فقط بعد از `complete=true`، بدون trigger motion/optimizer از وب.
- hamburger با عرض 36px و helpهای header بدون minimum touch size هستند (`screen_server_page.h:23,40`)؛ برای موبایل صنعتی باید حداقل 44px شوند.

## پیشنهاد محصول وب

1. مرحلهٔ اول: CSV/TXT نسخه‌دار با header ثابت `version,unit,length_mm,count`، preflight، شماره خط و علت رد، summary و commit PIN-gated.
2. Download current list با Blob در مرورگر؛ بدون file-store یا dependency روی ESP32.
3. API read-only برای optimized snapshot و download plan شامل stock/kerf/trim/bars/pieces/waste/complete.
4. XLSX مستقیم فعلاً اجرا نشود؛ XLSX یک ZIP/XML چندفایلی است. ابتدا تبدیل XLSX→CSV را راهنمایی کنید. parser مرورگری فقط با تأیید dependency/مجوز/حد فایل بررسی شود.
5. مشاهدهٔ state آزاد و PIN/session/TLS نیازمند سیاست صریح مالک است؛ به‌صورت ضمنی تغییر نکند.

الگوی انتخاب‌شده با مستندات recipe/import رسمی Siemens، Pro-face و Schneider هم‌راستاست: CSV ساختاریافته، validation قبل از overwrite و export/import snapshot کنترل‌شده.

## ترتیب اصلاح پیشنهادی پس از تأیید

1. Safety: command queue وب و revalidation در owner task؛ سپس داوری مستقل.
2. Security decision: PIN provisioning و سیاست signed OTA؛ بدون تصمیم مالک patch نشود.
3. Stability: stress measurement حافظه و F3/F5 redraw؛ اصلاح فقط با شواهد.
4. HMI: help clipping/F4 z-order/F2 input evidence/Main gear؛ هر صفحه با freeze-table جدا.
5. Scenario matrix: پوشش تمام screen/tab/modal/input flowها.
6. Web product: CSV preflight/export، سپس optimized snapshot/export؛ XLSX خارج از فاز اول.

## موارد تأییدنشده

- هیچ build/upload جدیدی در این ممیزی اجرا نشد.
- Web app live و responsive screenshot تأیید نشد.
- صفحهٔ Reference/Splash، PIN درست/غلط، touch/USB input، حرکت، برش، E-stop و stress تحت ماشین تست نشدند.
- SHOT واقعی فقط ظاهر لحظه‌ای را اثبات می‌کند؛ رفتار touch/persistence را اثبات نمی‌کند.
