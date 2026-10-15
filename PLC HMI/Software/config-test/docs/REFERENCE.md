# مرجع فنی — Config Launcher (HT-V3A)

## سخت‌افزار

- MCU: ESP32-S3 (board: esp32-s3-devkitc1-n16r2)، فلاش 16MB، flash_mode `dio`، memory_type `dio_qspi` — **qio_opi باعث هنگ بوت‌لودر می‌شود** (اصلاح جلسه‌ی ۲)
- نمایشگر: TFT_eSPI، 480×320، flush بلاک‌کننده روی SPI
- تاچ: bb_captouch روی I2C — کلاک I2C باید 400kHz باشد (در 100kHz خواندنِ هر فریم نخ LVGL را بلاک می‌کند و اسکرول می‌لرزد)
- پریفرال‌های تست: RTC (RTClib)، MAX6675 (0..2 عدد)، MCP23017، ADC، RS485/Serial2
- **آی‌سی افزاینده پایه‌ها (MCP23017-E/SO روی I2C 0x20):**
  - ۶ خروجی: `GPA0` (OUT0), `GPA1` (OUT1), `GPA2` (OUT2), `GPA3` (OUT3), `GPA4` (OUT4), `GPA5` (OUT5) — اکتیو-LOW
  - ۲ ورودی: `GPA7` (IN0), `GPA6` (IN1) — مجهز به پول‌آپ داخلی ۱۰۰k
  - پایه‌های پورت B (`GPB0..GPB7`): بدون اتصال (NC)
  - در صورت حضور سخت‌افزار، پنل تست داینامیک `+ MCP23017 (6O/2I)` در صفحه GPIO فعال می‌شود.
- پورت‌ها: دستگاه روی **COM4 یا COM8** (هر دو Silicon Labs CP210x — شماره می‌تواند عوض شود)؛ **COM3 مربوط به Intel AMT مادربرد است — دستگاه نیست** (دام رایج)؛ دیباگ UART0
- **فلش کامل با esptool** (جلسه‌ی ۲): `pio run -t upload` فقط app را در 0x10000 می‌نویسد — وقتی فلاش جدول پارتیشن خارجی دارد ناکافی/غلط است. سه ناحیه نوشته شود (فایل‌ها از `.pio/build/ht_v3a/`):
  `esptool.py --chip esp32s3 --port COM8 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin`

## معماری فریم‌ور

- `firmware/config_hwtest.ino` — نقطه‌ی ورود: setup، تاکس‌ها، Wi-Fi/OTA actions (از 1.0.1 در ریشه‌ی firmware — الزام PlatformIO؛ نه در src/)
- تسک‌ها (FreeRTOS):
  - `LVGL Tick` (core 1, prio 3)
  - `lvgl_task` (core 1, prio 2) — lv_timer_handler + ui_tick + hwtest_loop
  - `app_task` (core 0, prio 1) — Wi-Fi scan/connect، دانلود OTA
  - `buzzer_task` (core 0, prio 1)
- بافرهای رندر: partial-mode، دو بافر DMA در SRAM داخلی؛ اندازه به‌صورت تطبیقی از {48,38,32,24,16} خط (PSRAM روی QSPI کند است — استفاده نشود)
- UI: LVGL 9 + EEZ Flow (صفحات/متغیرها با get_var_*/set_var_*)

## سیستم پرینتر حرارتی و ممیزی آزمون کیفی سخت‌افزار (QC Test Audit Protocol)

- پورت ارتباطی: `Serial2` روی پین‌های `GPIO18` (RX) و `GPIO17` (TX) با خط کنترل فرستنده `GPIO46` (DE/RE - اکتیو HIGH برای ارسال).
- باودریت پرینتر حرارتی: **19200 baud (8N1)** با درایور استاندارد ESC/POS ۵۸ میلی‌متری (عرض ۳۲ کاراکتر).
- پایش و ممیزی رویدادهای تست (`HwTestAudit`):
  - ثبت زمان حضور در آزمون از بدو ورود تا زمان فشردن کلید چاپ.
  - ثبت بیت‌ماسک تک‌تک کلیدهای فشرده‌شده جلو پنل (`F1..F10`) و فوت‌سوییچ‌ها (`FS1..FS2`).
  - ثبت بیت‌ماسک رله‌های تست‌شده (`Q1..Q10`) و ورودی‌های سنسور (`I0..I7`).
  - ثبت حضور ماژول‌ها و ورودی/خروجی‌های افزاینده MCP23017.
  - ثبت دمای سنسورهای MAX6675 و تست المنت SSR.
  - ثبت تعداد نقاط رسم‌شده روی صفحه لمسی (Touch Drawing) و صفحات رنگی بازدیدشده.
  - ثبت نتیجه اسکن و اتصال WiFi و بلوتوث.
  - محاسبه امتیاز بازرسی کیفی (QC Score 0..100%) و چاپ کارنامه کامل همراه با نام و امضای اپراتور و بازرس.

## معماری پایدار NVS و اشتراک تنظیمات (Shared NVS & Calibration Persistence)

1. **فضای نام اشتراکی وای‌فای و زبان (`"chone"` / `"haffler_net"`):**
   - کلیدهای `wc` (تعداد)، `ws0..ws15` (نام‌های SSID)، `wp0..wp15` (رمزهای عبور) و `language` (زبان فعال: 0=EN, 1=TR, 2=FA, 3=DE, 4=RU).
   - تمام پروژه‌ها (Config Launcher, Cutter, Welder, Gas-filling) از این ساختار یکسان استفاده می‌کنند تا با جابجایی بین برنامه‌ها نیازی به ورود مجدد رمز وای‌فای یا تغییر زبان نباشد.
2. **عدم بازنویسی پارتیشن NVS در OTA:**
   - جدول پارتیشن‌ها (`partitions.bin`) پارتیشن `nvs` را در آدرس `0x9000` قرار داده است. با فلش یا به‌روزرسانی OTA روی `app0` یا `app1`، حافظه NVS کاملاً دست‌نخورده باقی می‌ماند و کالیبراسیون دستگاه‌ها (طول گام، آفست‌ها، ضرایب انکودر و زمان‌بندی‌ها) هرگز پاک نمی‌شود.
3. **حفاظت ضد بازگشت و بررسی سلامت بوت (Anti-Rollback & Boot Verification):**
   - پس از هر آپدیت OTA، اجرای سالم برنامه از طریق `esp_ota_mark_app_valid_cancel_rollback()` در `setup()` تأیید می‌شود.
   - چنانچه برنامه جدید به علت نقص فنی در حلقه ریست بیفتد، بوت‌لودر سخت‌افزاری ESP32-S3 به صورت خودکار پارتیشن را برگردانده و سیستم را به برنامه سالم قبلی بازمی‌گرداند.

## سرور OTA (از نسخه 1.0.2 — 2026-08-23)

- Base host: `https://ota.haffler-infi.ir` — باکت سازگار با S3 به نام `ota-plc` روی ArvanCloud (region ir-thr-at1).
- کلید آبجکت‌ها URL-encoded است (جداشکننده‌ی `/` → `%2F`):
  `https://ota.haffler-infi.ir/upvc%2F<project>%2F<file>`
  - `file` دقیقاً یکی از دو مقدار: `firmware` (باینری؛ **بدون پسوند .bin**) یا `lastfile` (متن ساده‌ی نسخه، مثل `1.0.2`).
- پروژه‌ها (دسته‌ی همه `upvc`): welding-single-head، welding-double-head، digital-cutter-single-head، digital-cutter-double-head، gas-filling، corner-cleaning، copy-router، aluminum-punching، config-test.
  - نمونه‌ی این پروژه: `https://ota.haffler-infi.ir/upvc%2Fconfig-test%2Ffirmware` و `https://ota.haffler-infi.ir/upvc%2Fconfig-test%2Flastfile`
- **طرح قدیمی `haffler.com/ota/plc_hmi/...` با فایل `firmware.bin` کاملاً حذف شده است — استفاده نشود.**
- پوشه انتشار داخلی برای آپلود روی سرور: `release\ota\upvc\config-test\` شامل دو فایل `firmware` + `lastfile` (طبق قانون ممنوعیت مطلق دسکتاپ).
- قابلیت «ورود لینک» (1.0.2): دکمه در کارت اول (زیر متن HAFFLER HTV3A) → مودال textarea + کیبورد کامل LVGL → دانلود/نصب OTA از URL دلخواه http/https (scheme اعتبارسنجی می‌شود؛ فقط http/https).

### وضعیت واقعی سرور (بررسی 2026-08-25 — نیازمند اقدام مالک)
- سرور فعلاً فقط این آبجکت‌ها را دارد: `upvc/digital-cutter_single_head/{firmware.bin,lastfile}` — نام‌گذاری با underscore + پسوند `.bin` یعنی **با قراردادِ دستگاه (`upvc%2F<model>%2Ffirmware` بدون پسوند) MISMATCH دارد**.
- `upvc/welding-single-head/lastfile` → **403**؛ کارت‌های همه‌ی دستگاه‌ها فعلاً **404** می‌دهند.
- **اقدام:** مالک باید فایل‌ها را با کلیدهای صحیحِ قرارداد دوباره آپلود کند — مرجعِ استاندارد: قرارداد 1.0.2 در `docs/WORKLOG.md`.

## تشخیص خطا

| علامت | علت رایج | راه‌حل |
|---|---|---|
| اسکرول لرزش/لگ | کلاک I2C تاچ 100kHz | `Wire.setClock(400000)` بعد از آخرین Wire.begin |
| صفحه‌ی خالی بعد از بوت | شکست تخصیص بافر رندر | لاگ `Draw buffer alloc FAILED` — زنجیره‌ی خطوط کوچکتر |
| دانلود OTA کند/قطع | modem-sleep فعال | `WiFi.setSleep(false)` و سیگنال را بررسی کن |
| تاچ بعد از خروج از تست خراب است | کلاک/زنجیره‌ی I2C (نه GPIO39/40 — آن پایه‌ها در تست استفاده نمی‌شوند و `hwtest_exit` عمداً init مجدد نمی‌کند) | قاعده‌ی ۲: `Wire.setClock(400000)` بعد از آخرین `Wire.begin` |

## نقشه‌ی فایل‌های کلیدی

| فایل | نقش |
|---|---|
| `firmware/config_hwtest.ino` | setup، تاکس‌ها، اکشن‌های Wi-Fi/OTA/رمز |
| `firmware/src/t_*.{c,h}` | UI تست سخت‌افزار (EEZ-generated؛ هرگز حذف نشوند) |
| `firmware/src/hwtest.cpp` + `firmware/include/app/hwtest.h` | ماژول تست سخت‌افزار (پورت از پروژه‌ی Test با LVGL 8) |
| `firmware/include/app/tasks.h` | بدنه‌ی تاکس‌ها (Wi-Fi/OTA/buzzer) |
| `firmware/include/app/storage.h` | NVS: Wi-Fi‌های ذخیره‌شده، زبان |
| `firmware/include/app/lang.{c,h}` | چندزبانگی (fa/en/de/ru/tr) + i18n_apply |
| `firmware/include/ui/eez-flow.*` | موتور صفحه‌ها/متغیرهای EEZ |
| `firmware/config/partitions*.csv` | جدول پارتیشن (8MB/16MB) |

## قواعد فنی (افزوده 2026-08-23 — نسخه 1.0.1)

1. **فایل‌های `t_*` هرگز حذف نشوند** — حتی اگر «بلااستفاده» به نظر برسند؛ `hwtest.cpp` به کل مجموعه نیاز دارد (HWTEST_UI_READY=1). در بازساختور 663b627 به‌اشتباه حذف شدند و در 1.0.1 از `663b627^` برگشتند.
2. **قاعده‌ی نخ LVGL و `g_lvgl_mutex`**: خوشه‌های `set_var_*/get_var_*` در `app_task` باید زیر `g_lvgl_mutex` باشند؛ `lvgl_task` آن را دور کل iteration خودش می‌گیرد. **هرگز `g_lvgl_mutex` را از کانتکست نخ LVGL (actions، `ui_tick`، hwtest) نگیر — deadlock.** استثنا: `config_start_wifi_scan()` عمداً بدون قفل است (از هر دو کانتکست صدا زده می‌شود).
3. **فایل‌های تولیدی EEZ** (`screens.c`، `t_screens.c`، `ui.c`): فقط ویرایش جراحی (Edit با رشته‌ی یکتا) — هرگز regenerate/overwrite نکن؛ ورودی‌های `actions[]` مرده را 0 کن تا شاخص‌های flow پایدار بمانند.
4. **Build آفلاین**: lib_deps محلی‌اند — lvgl 9.4.0 از `../Welding_single_head/lib/lvgl`، TFT_eSPI/bb_captouch/RTClib/Adafruit_BusIO از `Software/libraries`، MAX6675 از `config-test/libraries/MAX6675_library`. اسکچ `.ino` باید در ریشه‌ی `firmware/` باشد (`src_dir = "."`، `build_src_filter = +<config_hwtest.ino.cpp> +<src/> +<include/app/*.c> +<include/ui/*>`، `-I . -I config`). با آنلاین‌شدن می‌توان به specهای رجیستری برگشت. lv_conf.h: LV_USE_SPAN=0 (باگ lv_span.c در 9.4.0).
5. **OTA HTTPS**: دانلود OTA با `WiFiClientSecure.setInsecure()` — trade-off شناخته‌شده (بدون اعتبارسنجی گواهی). ریدایرکت ۳۰۱ مربوط به سرور قدیمی haffler.com بود (مرده).
6. **روال نسخه‌دهی**: همیشه هر چهار با هم — `firmware/include/app/defines.h`، فایل `VERSION` ریشه، یک خط در `firmware/include/app/fw_changelog.h` (≤۱۳ کلمه؛ در پاپ‌آپ اطلاعات ورژن روی دستگاه نمایش داده می‌شود) و ثبت در `docs/WORKLOG.md`. هر فلش = ورژن جدید.
7. **اسکرین‌شات (فعال از V1.0.3 - قانون سخت‌گیرانه عدم ثبت خودکار)**: درایور `firmware/include/drivers/screenshot.h`، دستور `SHOT` روی Serial/Serial0 (COM8)؛ ابزار: `python tools/hmi_screenshot.py --port COM8 --baud 115200 --out out.png`. **ممنوعیت مطلق اسکرین‌شات خودکار:** عکس‌برداری فقط و فقط با دستور صریح «عکس بگیر» کارفرما اجرا می‌شود تا ایشان ابتدا به صفحه موردنظر ناوبری نمایند.
8. **lastfile**: هر ریلیز: lastfile با tools/gen_lastfile.py بازتولید شود (استاندارد docs/LASTFILE_STANDARD.md)؛ فریمور Config آن را می‌خواند و توضیحات را در دیالوگ نصب نشان می‌دهد.
