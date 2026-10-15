# دانش فنی — متر دیجیتال + برش تک‌سر

| فیلد | مقدار |
|------|--------|
| **فریمور** | **V.1.5.20** (`VERSION` + `firmware/include/version.h`) |
| **LVGL در درخت** | **9.4.0** (`libraries/lvgl/lv_version.h`) — ارتقای جدا لازم نیست |
| **بورد / HMI** | ESP32-S3 DevKitC-1 + پنل Haffler ET-V3A — LCD **۴۸۰×۳۲۰** افقی |
| **مسیر فریمور** | `d:\metr and cutter1\firmware` |
| **آپلود برنامه** | COM4 (CP210x) @ **921600** (PlatformIO upload) |
| **دیباگ سریال / SHOT** | COM4 UART0 @ **115200** (`dtr=False`, `rts=False`) |
| **سناریوی رفتار** | `docs/SCENARIO_FA.md` |
| **برنامه توسعه** | `docs/MASTER_PLAN_FA.md` |
| **تغییرات / مانده** | `docs/CHANGELOG_AND_BACKLOG_FA.md` |
| **سیستم طراحی (اپل‌محور)** | `docs/HMI_DESIGN_SYSTEM_APPLE_FA.md` + `docs/APPLE_HIG_CRAFT_FA.md` |
| **مهارت اپل** | `.cursor/skills/apple-hig-design/` (پروژه + شخصی) |
| **حلقه عامل + قاضی** | `docs/assets/agent-design-loop-fa.png` + `.cursor/rules/70-agent-design-loop.mdc` |
| **تیم gstack** | `.cursor/skills/gstack*/` + `.cursor/rules/80-gstack-team.mdc` |
| **تست/changelog قدیمی** | `docs/TEST_REPORT_V1.5.0_FA.md` |
| **شبیه‌سازها** | `docs/SIMULATOR_FA.md` + این سند § شبیه‌ساز و تأیید UI |
| **فهرست اسناد** | `docs/INDEX_FA.md` |

---

## بیلد و آپلود (Cursor)

Workspace: `metr-cutter.code-workspace` (پوشهٔ ریشه). Tasks در **`.vscode/tasks.json`** ریشه:

| Task | کار |
|------|-----|
| `MC: Build` | فقط کامپایل |
| `MC: Fast Flash` | فلش `firmware.bin` موجود روی COM4 |
| `MC: Build + Upload` | بیلد + آپلود |
| `MC: Monitor` / `MC: Clean` | مانیتور / پاک‌سازی |

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "d:\metr and cutter1\firmware"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM4 --project-dir "d:\metr and cutter1\firmware"
# فقط فلش سریع:
firmware\fast_flash.bat
```

اگر `pio` در PATH نبود، همیشه از `penv\Scripts\pio.exe` استفاده کن.

بعد از آپلود اگر UI عوض نشد → **قطع برق برد** (نه فقط SoftReset). بعد از ریست صبر ~۲۰ s برای آماده شدن UART دیباگ.

### درس‌های بیلد (V1.5.20 / پاک‌سازی دیسک)

- LVGL vendored را می‌توان لاغر کرد (حذف `.git` / demos / docs / tests) تا دیسک و کپی `file://` سبک شود.
- سرعت بیلد با `libraries/lvgl/library.json` → `srcFilter` (حذف `drivers`، gltf، gif، svg، فونت‌های Montserrat غیرفعال، …).
- **هرگز کل پوشهٔ `osal/` را از srcFilter حذف نکن.** با `LV_USE_OS LV_OS_NONE` باید `lv_os.c` + `lv_os_none.c` بمانند؛ وگرنه لینک: `lv_os_init` / `lv_lock` / `lv_unlock`.
- فونت: فقط سایزهای روشن در `firmware/config/lv_conf.h` قابل ارجاع‌اند. `lv_font_montserrat_10` بدون `#define LV_FONT_MONTSERRAT_10 1` → خطای کامپایل `main.ino` (در 1.5.20 به `montserrat_14` عوض شد).
- کش ناقص بعد از kill بیلد → `cannot find ...gif.c.o`؛ `clean` یا حذف `.pio/build`.
- آپلود گیر روی Connecting → esptool/Monitor را بکش؛ COM4 آزاد؛ دوباره `fast_flash`.
- مسیر با فاصله روی ویندوز گاهی دردسرساز است؛ آینهٔ اختیاری بدون فاصله: `C:\fwbuild\` (قبل از بیلد با درخت اصلی sync کن). Long Path ویندوز را در صورت امکان فعال کن.
- skill بیلد: `.cursor/skills/metr-cutter-build-flash/SKILL.md`

### پنل Problems با «1K+» (هشدار، نه هزار باگ)

تقریباً همه از الگوی عادی LVGL است: `LV_PART_MAIN | LV_STATE_DEFAULT`. روی GCC 14 به‌صورت `deprecated-enum-enum-conversion` گزارش می‌شود. در `platformio.ini` این‌ها خاموش شده‌اند:

`-Wno-deprecated-enum-enum-conversion` ، `-Wno-deprecated-volatile`

فایل‌های `.clangd` (ریشه و `firmware/`) هم برای IntelliSense همین را suppress می‌کنند. خطای قدیمی `montserrat_10` اگر هنوز در Problems دیده می‌شود، کش تشخیص است — در کد به `montserrat_14` عوض شده؛ یک‌بار **Clangd: Restart language server** یا Reload Window.

---

## معماری فایل (نقشهٔ سریع)

| مسیر | نقش |
|------|-----|
| `src/main.ino` | globals، `LIST_MAX`/`OPT`/`GUI`/`STP`/`AUT`، ترتیب ساخت تسک |
| `include/version.h` + ریشهٔ `VERSION` | ورژن نمایشی — با هم bump کن |
| `include/app/GUI_Func.h` | هندلرهای لمس — **سبک نگه دار** |
| `include/app/ui_extras.h` | توست، نامپد، صفحه‌بندی/سوایپ، سطل، FFD، اسپلش، **کروم F1** (بدون EEZ rewrite) |
| `include/app/Pref_Func.h` | NVS / PEND / factory reset |
| `include/tasks/Task2.h` | `lv_timer_handler` + `process_ui_signals` + `pending_goto` + poll/dump `SHOT` |
| `include/tasks/Task3.h` | استپر / ISR |
| `include/tasks/Task8.h` | برش / E-Stop / پیشروی لیست / مود بهینه |
| `include/drivers/webportal.h` | AP + تنظیمات + OTA + ایمپورت فایل |
| `include/drivers/serial_import.h` | UART0 سایزها + فرمان‌های متنی |
| `include/drivers/screenshot.h` | **SHOT** فریم‌بافر + **SCR1/2/3** ناوبری deferred |
| `src/ui/screens.c` | EEZ — **UTF-8**؛ cp1256 نکن |
| `src/ui/ui.c` | `loadScreen` → `lv_screen_load` بدون fade |
| `src/ui/actions.c` | استاپ NC/NO، تست پرینتر، زبان |

---

## شبیه‌سازها و تأیید UI (جعبه ابزار کامل)

رزولوشن هدف همهٔ مسیرها: **۴۸۰×۳۲۰** (LCD افقی ۴ اینچ).

### اولویت تأیید (از دقیق به تقریبی)

| رتبه | ابزار | چه چیزی را ثابت می‌کند | وابستگی به برد |
|------|--------|-------------------------|----------------|
| **۱** | `SHOT` از خود HMI | پیکسل‌های واقعی روی LCD | بله — COM4 |
| **۲** | طراح F1 HTML | لی‌اوت بصری تنظیمات عمومی قبل از پورت به LVGL | خیر |
| **۳** | `list_ui_sim.py` | منطق لیست ۵۰/۱۰، صفحه، سطل، سوایپ | خیر |
| **۴** | SDL LVGL / QEMU / Wokwi | ویجت LVGL یا کرنل — در این پروژه روزمره نیست | خیر / سنگین |

**قانون عامل:** ادعای «لی‌اوت درست شد» فقط با **SHOT واقعی** معتبر است — نه mockup alone. طراح HTML مرجع طراحی است؛ فریمور باید به همان نزدیک شود (`ui_extras.h`)، نه EEZ کامل از نو.

---

### ۱) اسکرین‌شات واقعی از برنامه روی برد (PLC/HMI)

فریمور از حدود V.1.5.4 فرمان‌های دیباگ روی **UART0 / COM4 @ 115200** دارد (همان پورت CP2102؛ نه لزوماً همان baud آپلود).

| فرمان | اثر |
|--------|------|
| `SHOT\n` | دامپ فریم‌بافر LVGL (RGB565 LE، ۴۸۰×۳۲۰) → PC به PNG |
| `SCR1\n` | رفتن به صفحهٔ اصلی (`pending_goto_scr=1`) |
| `SCR2\n` | رفتن به تنظیمات / F1 (`pending_goto_scr=2`) |
| `SCR3\n` | رفتن به لیست (`pending_goto_scr=3`) |

پاسخ دستگاه به `SHOT`:

```text
SHOT_BEGIN 480 320 RGB565LE
<base64 یک خط به‌ازای هر ردیف — ۹۶۰ بایت خام>
...
SHOT_END
```

**ابزار PC:**

```powershell
# فقط عکس از صفحهٔ فعلی
python "d:\metr and cutter1\tools\hmi_screenshot.py" --port COM4

# رفتن به تنظیمات سپس عکس
python -c "import serial,time; s=serial.Serial(); s.port='COM4'; s.baudrate=115200; s.timeout=1; s.dtr=False; s.rts=False; s.open(); time.sleep(1); s.write(b'SCR2\n'); s.flush(); time.sleep(2); s.close()"
python "d:\metr and cutter1\tools\hmi_screenshot.py" --port COM4 --timeout 120 --out "d:\metr and cutter1\simulator\shots\board_f1.png"
```

| نکتهٔ ایمنی | جزئیات |
|-------------|--------|
| باز کردن پورت | حتماً `dtr=False`, `rts=False` وگرنه برد ریست می‌شود |
| بعد از آپلود/ریست | صبر ~۲۰ s (SerialImportTask دیرتر بالا می‌آید؛ قبلش Task2 خودش UART را poll می‌کند) |
| زمان دامپ | حدود **۳۵–۴۰ s** @ 115200 |
| بافر | کپی در **PSRAM** هنگام flush نمایش (`screenshot.h` + hook در `my_disp_flush`) |
| مسیر امن ناوبری | `SCR*` فقط `GUI.pending_goto_scr` می‌زند — **هرگز** `loadScreen` از سریال/لمس مستقیم |

خروجی پیش‌فرض: `simulator/shots/*.png`.

فایل‌های مرتبط:

- `firmware/include/drivers/screenshot.h`

---

### تست سرعت سریال برای SHOT / SCR

تاریخ تست: 2026-07-14.

نتیجه روی فریمور فعلیِ فلش‌شده روی دستگاه:

| Baud | نتیجه |
|---:|---|
| 115200 | جواب داد؛ SHOT کامل گرفته شد. فایل نمونه: `simulator/shots/current_cut_list_115200_verified_after_speed_test.png` |
| 230400 | جواب نداد؛ timeout و `0 rows` |
| 921600 | جواب نداد؛ timeout و `0 rows` |

نکته مهم: این نتیجه مربوط به فریموری است که فعلاً روی دستگاه فلش شده و داخل کد آن `Serial.begin(115200)` و `Serial0.begin(115200)` فعال بوده است. در سورس جدید، baud دیباگ/SHOT/import با ثابت `SERIAL_DEBUG_BAUD` یک‌دست شد و مقدار آن برای آپلود بعدی روی `921600` تنظیم شده است. بعد از آپلود نسخه جدید باید دوباره با:

```powershell
python tools/hmi_screenshot.py --port COM4 --baud 921600 --out simulator/shots/shot_921600_verify.png --timeout 30
```

تست شود. تا وقتی SHOT کامل در 921600 گرفته نشود، سرعت تاییدشده عملیاتی همان 115200 است.
- `tools/hmi_screenshot.py`
- پردازش در حلقهٔ Task2 (arm/dump)

---

### ۲) طراح بصری F1 — `settings-f1-designer.html`

| مورد | مقدار |
|------|--------|
| مسیر | `simulator/settings-f1-designer.html` |
| رزولوشن بوم | ۴۸۰×۳۲۰ |
| نقش | طراحی/ویرایش لی‌اوت «تنظیمات عمومی» + حالت ▶ اجرا برای سناریوهای لمس/MODE/کیبورد |
| مرجع مختصات | آرایهٔ `initial[]` داخل همان HTML (خانه، ابزار چپ، مقادیر، نامپد، F1–F5) |
| عکس مرجع طراح | `python tools/designer_f1_shot.py` → `simulator/shots/designer_f1_ref.png` |

قابلیت‌های طراح (وضعیت فعلی):

- درگ/ریسایز آزاد المان‌ها (بدون قفل مرز اجباری در حالت ویرایش)
- حالت اجرا: وای‌فای/وب، کیبورد مجازی، بازر، پرینتر، E-STOP (N.C پررنگ / N.O کم‌رنگ)، مقادیر + نامپد، i18n FA/EN، پنل ۱۰ کلید سخت‌افزاری، MODE فوکوس
- خروجی PNG/JSON، ذخیرهٔ localStorage (کلیدهای قدیمی پاک می‌شوند)

**پورت به برد:** کروم F1 را در `ui_extras.h` (`f1_reflow_eez_chrome` / `vkb_settings_build`) بساز — `screens.c` را برای جابه‌جایی جزئی بازنویسی نکن. بعد آپلود + `SCR2` + `SHOT` و مقایسه با `designer_f1_ref.png`.

---

### ۳) سیم منطق لیست — `list_ui_sim.py`

```powershell
python "d:\metr and cutter1\tools\list_ui_sim.py" --test
python "d:\metr and cutter1\tools\list_ui_sim.py"
```

پنجرهٔ ۴۸۰×۳۲۰؛ همان قواعد `LIST_MAX=50`، ۱۰ ردیف/صفحه، +/−، سطل، سوایپ. برای باگ کادر/صفحه/سطل **قبل از نتیجهٔ نهایی** روی برد.

---

### ۴) گزینه‌های سنگین‌تر (کم‌استفاده روزمره)

| ابزار | کاربرد | محدودیت این پروژه |
|--------|--------|-------------------|
| SDL + LVGL رسمی | ویجت LVGL واقعی روی PC | کل آردوینو (استپر/NVS/WiFi) اجرا نمی‌شود |
| QEMU ESP32-S3 | نزدیک کرنل/هسته‌ها | پروژه Arduino/PIO است → انتقال سنگین |
| Wokwi | اسکچ کوچک | پنل TFT+LVGL سنگین پوشش ضعیف |

جزئیات بیشتر: `docs/SIMULATOR_FA.md`.

---

### ۵) ایندکس ابزارهای `tools/`

| اسکریپت | کار |
|---------|-----|
| `hmi_screenshot.py` | `SHOT` → PNG از LCD واقعی |
| `designer_f1_shot.py` | عکس ۴۸۰×۳۲۰ از طراح HTML (Edge headless) |
| `list_ui_sim.py` | شبیه‌ساز منطق لیست + `--test` |
| `settings_f1_layout.py` / `settings_f1_preview.py` / `settings_f1_mirror.py` | مختصات/پیش‌نمایش F1 |
| `f1_layout_check.py` | چک لی‌اوت F1 |
| `list_designer_shot.py` / `render_screen.py` | کمک طراح/رندر |

پوشهٔ خروجی مشترک عکس‌ها: **`simulator/shots/`** (`user_f1_*.png` هدف کاربر، `board_*.png` از LCD، `designer_*.png` از HTML).

اسکریپت‌های فریمور: `firmware/fast_flash.bat`، `do_build_upload.bat`، `quick_build_flash.bat`.

---

---

## خطر بزرگ: متن فارسی مربع (□) / tofu — خواندن اجباری قبل از هر ویرایش رشته

**علائم روی دستگاه:** عنوان/متن دیالوگ، توست، برچسب LVGL به صورت **مربع خالی** یا علامت‌های غلط دیده می‌شود؛ انگلیسی همان فریم درست است.

### سه علت رایج (همه واقعی در همین پروژه)

1. **رمزگذاری فایل خراب شد (شایع‌ترین با Cursor Write / StrReplace روی ویندوز)**  
   ابزار ادیت گاهی فایل هدر فارسی را به‌صورت UTF-16 یا با mojibake ذخیره می‌کند. بایت‌های UTF-8 فارسی (`می‌خواهید…`) می‌شکنند → LVGL همان بایت‌های خراب را نشان می‌دهد یا گلیف ندارد → مربع.  
   **قانون:** برای هر تغییر که رشتهٔ فارسی دارد، پچ را با **Python `Path.write_text(..., encoding="utf-8")`** بزن؛ بعد از ادیت، در ترمینال چک کن:
   ```powershell
   python -c "from pathlib import Path; t=Path(r'firmware\include\app\admin_ui.h').read_text(encoding='utf-8'); assert 'تعداد برش' in t"
   ```
   اگر assert شکست → فایل خراب است؛ از git/نسخهٔ سالم برگردان و دوباره با Python پچ کن. **هرگز «فرمت متن درست شد» را بعد از Write کورکورانه ادعا نکن بدون این چک و بدون SHOT.**

2. **فونت غلط روی ویجت فارسی**  
   `lv_font_montserrat_*` گلیف فارسی ندارد. پیش‌فرض `lv_msgbox` هم اغلب Montserrat/تم است → عنوان «تأیید» و متن دیالوگ **همه مربع**.  
   **قانون:** هر `lv_msgbox_add_title` / `add_text` / دکمهٔ فارسی باید بعد از ساخت، فونت `ui_font_naz*` بگیرد (`lv_msgbox_get_title` / content labels / footer labels). انگلیسی → Montserrat OK.

3. **دو زبانهٔ همزمان در توست صفحات F**  
   قبلاً توست هم FA هم EN نشان می‌داد. درخواست محصول: یک زبان مطابق `GUI.system_language` (۱=فارسی، ۲=انگلیسی). `ui_toast` فقط یک خط نشان می‌دهد.

### چک‌لیست عامل قبل از «تمام» روی هر متن UI

| # | چک |
|---|-----|
| 1 | فایل هنوز UTF-8 است و زیررشتهٔ فارسی `in` فایل True است |
| 2 | ویجت فارسی → `ui_font_naz*` نه فقط Montserrat |
| 3 | msgbox فارسی → فونت naz روی title/body/footer اعمال شد |
| 4 | توست F → یک زبان |
| 5 | در صورت امکان `SHOT` واقعی از دیالوگ فارسی |

**الگوی اشتباه تکراری AI:** متن را می‌نویسد → روی برد مربع می‌شود → کاربر می‌گوید فرمت/فونت → تازه درست می‌کند. این بخش برای قطع همان حلقه است: **اول UTF-8 + فونت naz، بعد ادعا.**



### الگوی پیغام تأیید (Confirm Dialog Template) — F5و هر پیغام بعدی

**هدف:** هر دیالوگ تأیید/خیر یک قالب و یک رفتار داشته باشد (نه lv_msgbox خام با عنوان خالی/مربع).

**قوانین:**
1. **بدون عنوان جدا** — فقط متن پیغام (بدنه). کادر خالی «تأیید» بالا ممنوع.
2. **رنگ دکمه‌ها:** خیر / No = قرمز #FF3B30؛ بله/تأیید / Yes = سبز #34C759.
3. **فونت:** فارسی → ui_font_naz*؛ انگلیسی → Montserrat.
4. **GUI.lock_screen = true فقط وقتی دیالوگ زنده است.** هر مسیر بستن (بله، خیر، تایماوت، خطا) **باید** lock_screen = false کند و ابژه را حذف کند — وگرنه صفحه کم‌رنگ/قفل می‌ماند.
5. **نوار زمان ۶ ثانیه** زیر پیغام (ترک خالی → پر). اگر تأیید نشد، پیغام خودکار بسته می‌شود.
6. **نگه‌داشتن انگشت** روی پیغام/کارد → تایمر متوقف؛ بعد رها کردن دوباره شمارش ادامه می‌یابد.
7. **پیاده‌سازی مرجع:** 5_user_reset_cb / 5_confirm_tick / 5_close_confirm در dmin_ui.h (نه lv_msgbox_create).

**ضد UTF-8:** متن فارسی را فقط با Python UTF-8 به فایل بنویس؛ بعد از Write/محرر فایل 
ulls==0 را چک کن.

مرجع سبک نوار اطلاعات / چهارچوب: نرم‌افزار جوش `D:\New folder\V.1.5.0` (`get_ui_frame_prefix` / `Total Frames:` / برچسب با `:`).

## قوانین حیاتی (از باگ‌های واقعی V1.4→1.5+)

### UI / LVGL — علت غالب «هنگ»
1. **هرگز `loadScreen` / `lv_screen_load` داخل event لمس نزن.** فقط `GUI.pending_goto_scr` → `prcs_pending_goto_screen()` در Task2 (قبل از `lv_timer_handler`).
2. **NVS/Preferences داخل event ننویس** (مخصوصاً حلقهٔ چندده‌تایی). فلگ بزن؛ در Task2 تکه‌تکه بنویس (سطل: ۱۰ کلید/تیک).
3. کار سنگین UI را **یک تیک بعد از load** انجام بده (`pending_reinit_settings`).
4. `lock_screen` را بعد از بستن msgbox حتماً `false` کن.
5. انیمیشن FADE برای تعویض صفحه = bleed — غیرفعال مانده.
6. لیست صفحه‌بندی‌شده: parent **LVGL-scrollable نباشد**؛ با تغییر `list_page` لیبل‌ها را refresh کن.
7. کروم جدید F1/لیست را در **`ui_extras.h`** اضافه کن؛ EEZ (`screens.c`) را برای جابه‌جایی جزئی بازنویسی نکن.
7b. **هنگ تنظیمات / تعویض زبان (1.5.20):** `vTaskDelay` و hide/show ده‌ها لیبل داخل event زبان ممنوع؛ `GUI.flag_apply_language` + `apply_system_language_ui()` در Task2. ذخیرهٔ لیست/SP با بیت‌آرایهٔ pending، نه NVS در هر commit لمس. `vkb_settings_sync` نباید هر بار `f1_reflow` کامل بزند.

### حافظه / بوت سیاه
8. Task2 را **بلافاصله بعد از `init_widgets`** بساز. استک: 8192→6144→5120.
9. اگر `T2 create=-1` → صفحه سیاه دائمی بعد از `tft.fillScreen`.
10. بافر نمایش: INTERNAL|DMA با fallback خطوط 24→16→12→8.
11. WebPortal/SerialImport را با تأخیر ~۳s بساز.

### منطق دامنه
12. `LIST_MAX=50`، `LIST_ROWS_PER_PAGE=10`، ۵ صفحه.
13. ردیف جدید (+) همیشه **سایز=۰، تعداد=۰**.
14. پیش‌فرض NVS متراژ: **۰**.
15. **kerf/عرض تیغه** فقط packing بهینه‌سازی؛ جبران positioner نیست.
16. ورود تنظیمات ممنوع اگر: برش / اره روشن / گیره درگیر / تأیید معلق / موتور در حرکت.

### سخت‌افزار
17. ADC: همه `analogRead` با `adcSemaphore`.
18. خروجی‌ها **Active LOW** (HIGH ≈ ایمن/خاموش برای بیشتر خروجی‌ها).
19. پین‌ها / TFT_eSPI / نسخهٔ LVGL را بدون درخواست صریح کاربر عوض نکن.
20. ورژن فقط در `version.h` + `VERSION` ریشه.

---

## صفحات و ناوبری UI

| صفحه EEZ / منطقی | `pending_goto` / SCR | محتوا |
|------------------|----------------------|--------|
| اصلی | 1 / `SCR1` | اندازه‌گیری / کار روزمره |
| تنظیمات (tabview F1…) | 2 / `SCR2` | F1 تنظیمات عمومی + F2–F5 |
| لیست | 3 / `SCR3` | ۵۰ قطعه، ۱۰/صفحه |

F1 زنده: ترکیب EEZ پایه + کروم `ui_extras` (ستون ابزار، مقادیر، نامپد، وای‌فای→وب‌اپ). مرجع بصری: طراح HTML.

---

## قابلیت‌های محصول (نقشهٔ دسترسی)

| قابلیت | ورود |
|--------|------|
| لیست ۵۰تایی + ◀▶ + سوایپ | main_3 / SCR3 |
| سطل (صفر تعدادها) | هدر لیست — قرمز |
| بهینه برش FFD | هدر لیست — نارنجی |
| عرض تیغه / شاخه | F1 مقادیر (نامپد اگر vkb روشن) / پنل بهینه / وب |
| کیبرد مجازی on/off | F1 چیپ کیبورد |
| بازر on/off | F1 چیپ بلندگو |
| پرینتر تست | F1 چیپ پرینتر |
| E-STOP NC/NO | F1 بلوک استاپ (پررنگ/کم‌رنگ) |
| وب‌اپ + OTA | F1 وای‌فای یا SAVE پنل ۵s → START → AP `ESP32-UPLOAD`/`12345678` → `http://192.168.4.1` |
| ریست فکتوری | تنظیمات → CANCEL پنل ۵s |
| ایمپورت سریال | UART0 115200 متن `2500X10,...` |
| دیباگ عکس/صفحه | UART0: `SHOT`, `SCR1..3` |

PIN وب پیش‌فرض: `1234`.

---

## I/O خلاصه

| GPIO | نقش |
|------|-----|
| 16/42/39 | استپر PUL/DIR/EN |
| 48/40 | گیره بالا/جلو |
| 15/35/36/38 | اسپری/اره/لامپ/بالابر |
| 6/4/5 | ADC: رفرنس، جوی‌استیک، E-Stop+گارد |
| 3/7 | Start1/Start2 |
| 1/2 | F-keys / Save-Cancel پنل |
| 8/9 | تاچ I2C |

---

## تسک‌ها (هسته‌ها)

| تسک | هسته | کار |
|-----|------|-----|
| Task2 | 1 | LVGL + فلگ‌های UI + SHOT dump |
| Task3 | 1 | استپر ISR |
| Task8 | 1 | برش / ایمنی |
| Task1 | 1 | tick LVGL (millis-based) |
| Task4 | 1 | تاچ (مستقیم LVGL) |
| Task5/9/10 + پرینتر/وب | 0 | بازر، F-keys، پنل‌هلد، WiFi |

---

## پارامترهای motion

- PPR 6400 | گام 20mm | حداقل تایمر 8µs
- رمپ 450–2000ms | بک‌لش 0.5mm
- هومینگ: fast 30µs → creep 200µs | timeout 30s
- گارد تیغه timeout 5s | شروع برش: دو-استارت
- stock پیش‌فرض 6000 | kerf 3 | trim 0

---

## NVS (کلیدهای مهم)

`glb_spd`, `init_spd`, `jog_spd`, `vis_raws`, `sp_val_N`, `count_val_N`, `stock_len`, `kerf`, `trim`, `buz_stat`, `vkb_en`, `web_pin`, `tot_cuts`, `cut_timer`, `base_dir`, `stop_nc`, …

**ذخیره نمی‌شود:** `global_lowSpeedTime` (= همیشه 1000ms).

---

## الگوی کار عامل روی UI

```
1) باگ لیست/سطل/صفحه؟ → list_ui_sim.py --test
2) طراحی F1؟ → settings-f1-designer.html (مختصات) → ui_extras.h
3) بیلد → آپلود COM4
4) صبر بوت → SCR2 (یا صفحهٔ موردنظر) → hmi_screenshot.py
5) مقایسه PNG برد با طراح / انتظار کاربر
6) اگر فرق داشت → فقط ui_extras (یا باگ واقعی) را فیکس کن؛ mockup را «تموم» اعلام نکن
```

چک‌لیست هنگ:

```
1) loadScreen داخل event؟ → request_goto_screen
2) pref داخل event/حلقه؟ → flag + Task2 chunked
3) lock_screen گیر؟ → بعد msgbox false
4) Task2 create شکست؟ → سریال T2 create= / heap
5) reinit+load هم‌زمان؟ → یک تیک تأخیر
```

کاربر فارسی صحبت می‌کند؛ پاسخ کوتاه و عملی.

---

## بدهی / backlog آگاهانه

منبع زنده و اولویت‌دار: **`docs/CHANGELOG_AND_BACKLOG_FA.md`** (نسخهٔ فعلی V.1.5.17).

خلاصهٔ کوتاه:
- همگام‌سازی کامل پیکسلی طراح F1 ↔ فریمور (قارچ استاپ، F-keys، زیرنویس آیکن)
- یکدست‌سازی Main/List با سیستم طراحی اپل‌محور + حلقهٔ قاضی
- طراح HTML جدا برای Main و List (ایده)
- کلید MODE سخت‌افزاری بدون تاچ، راهنمای per-screen، STA WiFi
- تسک‌ها در header → coupling بالا؛ `String` در مسیر داغ؛ آستانه ADC سخت‌کد

---

## حلقهٔ طراحی عامل (قاضی‌ها)

تصویر رشتهٔ کار: `docs/assets/agent-design-loop-fa.png`

1. هدف را خیلی واضح بنویس.
2. نسخه بساز → خودبازبینی و لیست ضعف → اصلاح → تکرار ۳–۵ دور.
3. پنج قاضی: رنگ، چیدمان، لمس/سایز، سایه/آیکن، SHOT واقعیت.
4. فقط بهترین نسخهٔ نهایی را به کاربر بده.
5. سیستم طراحی: `docs/HMI_DESIGN_SYSTEM_APPLE_FA.md`  
   مهارت: `.cursor/skills/metr-cutter-hmi-design/SKILL.md`

---

## فلش کارخانه

```text
esptool.py --chip esp32s3 --port COM4 --baud 921600 write_flash ^
  0x0 bootloader.bin  0x8000 partitions.bin  0x10000 firmware.bin
```

خروجی بیلد: `firmware/.pio/build/esp32-s3-devkitc-1/`

---

## اخلاق عامل / تحویل کار

برای عامل بعدی: `docs/AGENT_HANDOFF_FA.md` و skill  
`.cursor/skills/metr-cutter-agent-handoff/SKILL.md`  
(اخلاق مالک، تعریف Done، شرایط تست، فهرست افزونه‌های ساخته‌شده در پروژه).

---

## اسناد و skill مرتبط

| مسیر | موضوع |
|------|--------|
| `docs/AGENT_HANDOFF_FA.md` | تحویل به عامل بعدی (اخلاق / تست / ابزار) |
| `docs/SCENARIO_FA.md` | رفتار محصول |
| `docs/MASTER_PLAN_FA.md` | برنامه توسعه (تاریخی) |
| `docs/CHANGELOG_AND_BACKLOG_FA.md` | تغییرات + ماندهٔ زنده |
| `docs/HMI_DESIGN_SYSTEM_APPLE_FA.md` | اصول رنگ/چیدمان/سایه اپل‌محور |
| `docs/assets/agent-design-loop-fa.png` | رشتهٔ حلقهٔ عامل |
| `docs/INDEX_FA.md` | فهرست همهٔ اسناد |
| `docs/PROJECT_NOTES.md` | نکات فنی سریع |
| `docs/SIMULATOR_FA.md` | جزئیات شبیه‌سازها |
| `.cursor/skills/metr-cutter-hmi/SKILL.md` | قواعد عامل فریمور |
| `.cursor/skills/metr-cutter-build-flash/SKILL.md` | بیلد / فلش / LVGL srcFilter / COM4 |
| `.cursor/skills/metr-cutter-agent-handoff/SKILL.md` | اخلاق مالک + تست + Done |
| `.cursor/skills/metr-cutter-hmi-design/SKILL.md` | حلقه طراحی + قاضی |
| `.cursor/skills/gstack*/SKILL.md` | تیم مجازی gstack (CEO…Release) |
| `.cursor/rules/metr-cutter.mdc` | alwaysApply پروژه |
| `.cursor/rules/60-hmi-design-apple.mdc` | اصول طراحی |
| `.cursor/rules/70-agent-design-loop.mdc` | لوپ قاضی |
| `.cursor/rules/80-gstack-team.mdc` | مسیریابی نقش‌های gstack |

---

## یادداشت فنی 2026-07-13 — هنگ تایید و صفحه اصلی

- هنگ «بعد از تایید و هنگام ورود به صفحه اصلی» روی برد با فرمان موقت `REF1` بازتولید شد. نتیجهٔ دیباگ خطی: `loadScreen(SCREEN_ID_MAIN_1)` برمی‌گشت، اما اولین `lv_timer_handler()` بعد از لود صفحه قفل می‌کرد.
- علت عملی روی سخت‌افزار: کانتینر تمام‌صفحهٔ EEZ در صفحه اصلی، یعنی `objects.obj1` داخل `main_1`، رندر را قفل می‌کرد. مخفی‌کردن فرزندان کافی نبود؛ وقتی خود `obj1` مخفی شد، `lv_timer_handler()` رد شد.
- راه‌حل فعلی: `ensure_safe_main1()` در `Task2.h`، `objects.obj1` را مخفی می‌کند و صفحه اصلی سبک را مستقیم روی `objects.main_1` می‌سازد.
- از shadow بزرگ و آبجکت تمام‌صفحهٔ اضافه در فریم اول صفحه اصلی پرهیز شود. فونت‌های `ui_font_digit60` و `ui_font_digit100` روی صفحه سبک تست شدند و قفل نکردند؛ `digit160`/رندر سنگین برای فریم اول پرریسک است.
- مسیر تایید نباید `STP.referenced` را قبل از لود صفحه اصلی فعال کند. الگوی امن: callback فقط `pending_reference_confirm` و `GUI.pending_goto_scr=1` را ست کند؛ Task2 اول صفحه را لود کند و بعد از `GUI.scr_id=1`، referenced را فعال کند.
- برای جلوگیری از jog ناخواسته بعد از تایید، `reference_jog_inhibit_until_ms` و sync وضعیت joystick در Task11 لازم است.
- کیبورد USB: `Task4` نباید مستقیم `lv_*`، `numpad_commit` یا commitهای UI را صدا بزند. کلیدها فقط state/flag را تغییر بدهند؛ Enter با `keyboard_pending_enter` در Task2 مصرف شود.
- فرمان موقت `REF1` فقط برای دیباگ بود و بعد از تست حذف شد. در نسخه تولیدی نباید فرمان سریالی برای bypass کردن reference/homing باقی بماند.
## یادداشت فنی 2026-07-14 — ریشه هنگ صفحه اصلی

- ریشه مشکل فقط مختصات نبود. تابع `main_reflow_position_disk()` هنوز داخل `objects.obj1` قدیمی آبجکت دیسک و shadow می‌ساخت. `obj1` همان container تولیدشده‌ی EEZ بود که روی LCD واقعی در اولین render صفحه اصلی قفل ایجاد می‌کرد.
- راه‌حل پایدار: صفحه اصلی در `ensure_safe_main1()` وارد `main1_safe_mode` می‌شود؛ `objects.obj1` فقط برای سازگاری pointerها مخفی می‌ماند و هیچ آبجکت/سایه جدیدی داخل آن ساخته نمی‌شود.
- shadow داخلی LVGL برای دیسک صفحه اصلی حذف شد. سایه دور دایره با آبجکت ساده‌ی پشت‌زمینه ساخته می‌شود تا CPU/render path سنگین نشود.
- تست بعد از اصلاح: پنج بار فرمان `SCR1` پشت‌سرهم OK برگشت و `SHOT` کامل گرفته شد؛ یعنی load صفحه اصلی و render فریم کامل قفل نکرد.
