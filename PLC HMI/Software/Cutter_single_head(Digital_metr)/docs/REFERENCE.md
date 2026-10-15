# Technical Reference

## Platform

| Item | Value |
|---|---|
| Controller/HMI | ESP32-S3, Arduino/PlatformIO, LVGL 9.4, 480x320 landscape |
| Build environment | `firmware/`, `esp32-s3-devkitc-1`, 8 MB flash, PSRAM |
| Display | ILI9341, TFT_eSPI-owned FSPI/SPI2, 40 MHz, `TFT_BGR`, rotation 3 |
| Touch | bb_captouch, SDA GPIO8, SCL GPIO9, orientation 270 |
| Upload/debug | COM8 is the primary HMI port at 921600 / 115200; SHOT must disable DTR/RTS. If COM8 is absent or unavailable, enumerate the connected serial ports and use the detected HMI port; do not assume COM4. |

Outputs are active LOW. Key pins: stepper PUL/DIR/EN = GPIO16/42/39; clamps = 48/40; spray/saw/lamp/lift = 15/35/36/38. Inputs: reference ADC = GPIO6; guard/E-stop ADC = GPIO5; blade-return/motor-fault ADC = GPIO4; Start1/Start2 = GPIO3/7; joystick T1/T2 = GPIO21/37 (digital active LOW with internal pull-up, disconnected = inactive HIGH, 30 ms debounce); F-keys/save-cancel = GPIO1/2.

## Firmware map

| File | Responsibility |
|---|---|
| `firmware/src/main.ino` | startup, globals, tasks, display/touch init |
| `firmware/include/app/GUI_Func.h` | callbacks, touch, display flush, settings/list handlers |
| `firmware/include/app/ui_extras.h` | runtime UI chrome, keypad, list/optimization helpers |
| `firmware/include/app/Pref_Func.h` | NVS/preferences |
| `firmware/include/tasks/Task2.h` | LVGL loop, UI signals, deferred navigation, SHOT |
| `firmware/include/tasks/Task3.h` | stepper, ISR, homing |
| `firmware/include/tasks/Task8.h` | cut flow and safety |
| `firmware/include/drivers/screenshot.h` | real-device framebuffer capture |

## Runtime rules learned from hardware

- UI callbacks schedule `GUI.pending_goto_scr`; they do not call `loadScreen()` directly.
- Keep UI work in Task2 and keep the stepper ISR free of display work.
- Large Preferences/list cleanup is queued or chunked in Task2, never bulk-written from an LVGL event.
- The accepted LCD baseline is 40 MHz FSPI/SPI2 with guarded DMA fallback. The old 80 MHz hardware-CS route froze the physical panel and must not return.
- Preserve partial/per-digit redraw. Removing fonts or images needs ELF/map evidence and a visual regression test.
- Persian labels use project Persian fonts; do not mix unsupported Latin tokens or ASCII digits into the Persian label.
- Persian BiDi & Latin isolation rule: Never mix Latin words, URLs, parentheses, or technical English terms directly inside Persian labels without setting `LV_BASE_DIR_RTL` on Persian text and `LV_BASE_DIR_LTR` on numeric/Latin values (e.g., IP addresses, URLs, timestamps `HH:MM:SS`). Mixed strings without explicit base direction cause severe BiDi word and parenthesis scrambling on the display.
- Modal & card layout symmetry: Modals must be horizontally centered on the 480px screen (`440-456px` wide) with symmetric margins and balanced, equal-sized action buttons. Never leave asymmetric empty columns or unrendered canvas areas.

## Owner acceptance patterns

- F4 administrator layout: treat the black hardware-status strip and F-key column as frozen unless explicitly named. The top three cards retain their card order and centered numeric values; the timer unit `S` stays inside its numeric field with a deliberate gap from the right border.
- F4 PIN/date layout: the two PIN fields are equal-size mirrored fields with localized Persian placeholders, centered text, readable contrast, and titles that do not touch the card border. The date/time card is a separate group with a clear vertical gap; its five captions share a baseline, the colon aligns with the hour/minute fields, and no lower action card may be clipped at the initial viewport.
- F4 lower actions: `تصحیح طول`, `جویستیک`, `ذخیره پیش فرض کارخانه`, and `کالیبره عدد استپر` form a two-column grid. Both rows share the same left/right column edges and an 8px column gap; retain the existing semantic colors and event wiring. The correction numeric box and its `mm` unit move with their parent button.
- F4 scrolling: reserve a visible scrollbar that never crosses the Save/header controls or F-key column. Increasing its thickness must preserve its x-position and the content's safe right padding.
- F5 history: hide page count and newer/older controls while all records fit on one page; use vertical scroll first, then show paging only after another page exists.
- F5 clock/calendar: the analog clock and its text form one vertical group. Upward placement must remain below the fixed header, with no clipped face, labels, or close control. Weekday output includes a comma and the active calendar month (Solar Hijri for Persian, Gregorian for English).
- Memory reporting: distinguish internal heap/RAM from PSRAM. Do not claim a target percentage or reduce task stacks without measured free-memory and task high-water evidence; keep LVGL allocations in PSRAM where configured.
- Stack/RAM measurement: use the UART0 `MEM` diagnostic after idle and again after the relevant stress path. ESP-IDF reports task high-water in bytes. Do not reduce a task stack from a single idle reading, and do not change Task3/Task8 motion/cut stacks or Task4 USB stack without explicit owner authority.
- Display performance: full-frame LCD throughput is not the UI target. The 22-FPS goal applies to dirty/interactive regions, so use bounded partial redraw and measure with `PERF`; never re-enable an unsafe SPI/DMA configuration merely to improve a benchmark.
- F1 speaker: enabling the key-sound option is silent. When enabled, a normal independent key press gives one bounded beep; commands received during that tone are not replayed as a continuous beep. Persistence remains tied to Save/cancel semantics.
- Panel-only navigation: MODE cycles every enabled control in the active modal or settings tab, including controls initially below the scroll viewport; focus must scroll into view. START confirms or increases an eligible control, STOP decreases it, and CANCEL backs out one level. Navigation is unavailable during motion/cutting and must not bypass safety or PIN rules.
- Mobile screen server: Wi-Fi association alone is not proof that the API is reachable. The page must remain usable if its initial settings API request fails, visibly show offline state, and keep retrying; a successful API response restores live values and online state.
- Web cut-list import is a review pipeline, never a machine command: decode and normalize Persian/Arabic digits and labels, detect a bounded text table, require unambiguous length/quantity mapping and units, show every conversion/rejection against the live device range, then apply only to the editable browser form. The operator must still perform the authenticated list Save. Ambiguous columns, unitless data, overflow, or unsupported binary/PDF/CNC files must not be guessed or silently truncated.
- Window-design/CNC compatibility is rule-set and machine specific. Neutral CSV/TSV/TXT cut lists may be supported under the reviewed import contract; do not claim native LogiKal, SchueCal, Klaes, NCX/NCW/ECW/ENC or machining compatibility without the exact licensed schema, real sample files, profile/joint rules, and machine safety validation.

## Machine behavior and verification

- Motion defaults: 6400 PPR, 20 mm pitch, 0.5 mm backlash, 30 us fast homing, 200 us creep, 30 s stage timeout.
- List UI: `LIST_MAX=100`, five visible rows/page; bounded/virtualized rows are required.
- Real device commands: `SHOT`, `SCR1`, `SCR2`, `SCR3`, `PERF`, and `LCDB`. `SCR*` only schedule deferred navigation.
- Simulator source stays in `simulator/` and `pc_sim/`. It is a design/logic aid; hardware SHOT is final evidence.

## Current validation risks

- Validate the accepted LCD path during normal motor-motion use after an approved upload.
- Hardware-verify five-key navigation and active settings/list/optimization flows after relevant changes.
- Keep USB hot-plug verification separate from soft-keyboard/touch work.

## LCD Performance Contract (قرارداد ثابت کدنویسی)

قوانین زیر باید در تمام ایجنت‌ها و توسعه‌دهندگان رعایت شوند تا از regression عملکرد نمایشگر جلوگیری شود:

### سخت‌افزار و تنظیمات پایه
- SPI_FREQUENCY = 80000000 (ILI9341 practical max). اگر آرتفکت بصری رخ داد → 55000000.
- CONFIG_SPIRAM_SPEED_80M=y در sdkconfig.defaults — باید همیشه باقی بماند.
- Touch I2C clock باید 400000 (400kHz Fast-Mode) باشد.

### LVGL Flush و DMA
- هیچ ets_printf، Serial.print، یا بلاک‌کننده‌ای در my_disp_flush یا my_disp_flush_wait مجاز نیست.
- DISPLAY_DMA_MIN_PIXELS = 256 — این آستانه تغییر نکند.
- dual_fast_line_try[] = {32, 24, 16} — بافر بزرگ‌تر از 32 خط نیاز به بررسی SRAM دارد.

### لیبل و رندر
- قبل از هر lv_label_set_text باید strcmp با مقدار فعلی انجام شود (label guard).
- ui_display_refr_period_ms = 20ms حین حرکت موتور، 25ms در حالت ایستا.

### سایه (Shadow)
- حداکثر shadow_width برای modal روی صفحه اصلی: **6px**
- حداکثر shadow_width برای کارت و badge: **4px**
- هیچ shadow بالای 8px بدون دلیل مستند قابل اضافه کردن نیست.

### NVS
- فقط یک session (pref.begin ... pref.end) برای flush لیست مجاز است.
- هیچ save_* جداگانه پس از save_all() نباشد.

### قرارداد طراحی پنجره‌ها و ورودی‌های HMI (Apple/Siemens Rigor)
1. **جداسازی کامل لیبل واحد از کادر عددی:** هرگز شیء لیبل واحد (`pc_unit` یا واحد اندازه‌گیری مانند `mm` / `um`) نباید به عنوان فرزند مستقیم کادر ورودی (`lv_textarea`) تعریف شود تا با کاراکترهای تایپ‌شده تداخل نکند. لیبل واحد باید روی والد (Card/Dialog) و با فاصلهٔ متقارن در کنار کادر قرار گیرد.
2. **چیدمان طبیعی راست‌به‌چپ (RTL):** در تمام دیالوگ‌های سه‌کلیده:
   - سمت راست: عملیات مثبت و اصلی (آبی / تأیید / کیوآرکد).
   - وسط: عملیات مهم یا کنترلی (قرمز / قطع / تغییر حالت).
   - سمت چپ: عملیات خنثی یا بستن (خاکستری / لغو).
3. **ناوبری سراسری ۵ کلید پنل (5-Key Navigation):** کلید MODE در تمام صفحات (صفحه ۱ اصلی، صفحه ۲ تنظیمات F1-F5، صفحه ۳ لیست برش و تمام مودال‌ها) کادر نشانگر آبی را حرکت می‌دهد؛ کلید START انتخاب/ورود می‌کند؛ کلید CANCEL لغو/بازگشت به عقب انجام می‌دهد؛ کلید SAVE تغییرات را در حافظه ثبت می‌کند و STOP توقف قطعی می‌دهد.

## پروتکل ناوبری و احراز هویت سریال (UART0 Authenticated Protocol)

ارتباط سریال از طریق پورت UART0 (پایه‌های TX0/RX0 متصل به پل CP2102) با نرخ تبادل پیش‌فرض **921600 باود** (با فال‌بک 115200) به عنوان درگاه کنترل و تست از راه دور بدون نیاز به لمس فیزیکی تاچ عمل می‌کند.

### مدل امنیتی چالش-پاسخ (Challenge-Response Authentication)
دستورات نظارتی و تست (مانند عکس‌برداری، گزارش حافظه، تغییر صفحه و زبان) نیازی به احراز هویت ندارند تا تست‌های خودکار بدون وقفه انجام شوند. اما تمامی دستورات نوشتنی، حساس یا تغییردهنده حافظه NVS مستلزم احراز هویت چالش-پاسخ SHA-256 با رمز ادمین دستگاه هستند:
1. کامپیوتر یا اسکریپت تست ارسال می‌کند: `AUTH?`
2. فرم‌ور یک نانس تصادفی ۱۶ کاراکتری هگز برمی‌گرداند: `NONCE=<16_hex>`
3. کامپیوتر هش SHA-256 ترکیب نانس و رمز ادمین (`NONCE + admin_pin`) را محاسبه کرده و ۱۶ کاراکتر اول آن را ارسال می‌کند: `AUTH=<16_hex>`
4. فرم‌ور هش را بررسی کرده و در صورت صحت، یک پنجره سرویس ۶۰ ثانیه‌ای باز می‌کند (`OK svc window 60s`) و نانس را باطل می‌سازد (ضد Replay Attack).
5. ۵ بار ارسال پاسخ غلط، سرویس سریال را به مدت ۶۰ ثانیه قفل سخت‌افزاری می‌کند (`ERR: service locked, wait 60s`).

### جدول دستورات ناوبری، تلمتری و تست (Headless Test Matrix)

| دستور | نوع | عملکرد و پاسخ فرم‌ور |
|---|---|---|
| `SHOT` | عمومی | استریم غیرهمگام فریم‌بافر 480x320 RGB565LE با فرمت Base64 زیر ۴ ثانیه بدون لگ |
| `MEM` | عمومی | گزارش مصرف حافظه داخلی، PSRAM و High Watermark استک تمام ۱۱ تسک FreeRTOS |
| `PERF` | عمومی | سنجش FPS و عملکرد رندرینگ LVGL و ردگیری زمان رسم نواحی کثیف |
| `LCDB` | عمومی | بنچمارک سخت‌افزاری باس نمایشگر ILI9341 |
| `QSTA` | عمومی | استعلام پارامترهای اصلی ماشین (صفحه فعلی، زبان، سرعت، ضخامت تیغه و ...) |
| `SCR1` | عمومی | هدایت و پرش کنترل‌شده UI به صفحه ۱ (صفحه اصلی برش / Manual & Auto) |
| `SCR2` | عمومی | هدایت و پرش کنترل‌شده UI به صفحه ۲ (صفحه تنظیمات F1 تا F5) |
| `SCR3` | عمومی | هدایت و پرش کنترل‌شده UI به صفحه ۳ (صفحه جدول لیست برش) |
| `TAB1` .. `TAB5` | عمومی | تغییر مستقیم تب در صفحه تنظیمات (F1 کاربری، F2 کالیبراسیون، F3 زاویه، F4 ادمین، F5 گزارش) |
| `LFA` | عمومی | تغییر زبان سامانه به فارسی (`OK LFA`) با چیدمان کامل راست‌به‌چپ (RTL) |
| `LEN` | عمومی | تغییر زبان سامانه به انگلیسی (`OK LEN`) با چیدمان چپ‌به‌راست (LTR) |
| `QH1` .. `QH5` | عمومی | باز کردن دیالوگ راهنمای گرافیکی متناظر با تب‌های F1 تا F5 |
| `QLH` | عمومی | باز کردن پنجره مودال راهنمای صفحه لیست برش (`OK QLH`) |
| `QOH` | عمومی | باز کردن پنجره مودال راهنمای پنل بهینه‌سازی برش (`OK QOH`) |
| `QF2` | عمومی | باز کردن کیبورد مجازی خط شروع کالیبراسیون F2 |
| `QDL` | عمومی | بارگذاری ۵ ردیف داده دمو در جدول برش جهت تست‌های پایه‌ای |
| `QST` | عمومی | بارگذاری ۲۰ ردیف داده استرس در جدول برش جهت تست اسکرول و حافظه |
| `OPT` | نیازمند AUTH | باز کردن / تغییر وضعیت پنل بهینه‌سازی برش (`opt_panel_open`) |
| `SAVE` | نیازمند AUTH | اعمال و ذخیره تمامی متغیرهای در حال تعلیق در حافظه NVS |
| `BON` / `BOFF` | نیازمند AUTH | فعال یا غیرفعال کردن بیزر کیبورد |
| `RBT` | نیازمند AUTH | راه‌اندازی مجدد ایمن میکروکنترلر ESP32-S3 |
| `FRST` | نیازمند AUTH | بازنشانی تنظیمات به پیش‌فرض‌های کارخانه (Factory Reset) |
| `<LENGTH>X<COUNT>,...` | نیازمند AUTH | ایمپورت مستقیم لیست قطعات از طریق پورت سریال و نمایش دیالوگ تأیید روی HMI |
