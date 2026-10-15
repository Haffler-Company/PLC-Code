# نقشه‌راه و راهنمای جامع پروژه — Welding HMI

این فایل ورودی اول هر ایجنت (Aider / EverFern / Cursor / Ollama) است. قبل از گشتن در کل پروژه، اول همین فایل را بخوان تا مستقیم سراغ فایل درست بروی.

پروژه: HMI جوش تک‌سر روی ESP32-S3 + LVGL 9.4 + EEZ Studio (UI generator) + PlatformIO.

## 0) قبل از هر کاری

- **قانون بتنی بدون استثنا (استانداردهای زیمنس و اپل):** بدون هیچ‌گونه استثنایی، کلیه کارها، معماری‌ها، کدهای سخت‌افزاری، طراحی وب‌اپلیکیشن و رابط کاربری باید منحصراً با استانداردهای قطعیت و ایمنی صنعتی زیمنس (Siemens Industrial Determinism) و ظرافت و تجربه کاربری اپل (Apple HIG) پیش برود و کیفیت کار با نهایت دقت، بازرسی خط به خط و بیلد موفق `pio run` اعتبارسنجی و اثبات شود.
- **نسخه فعال فریم‌ور:** تولید خودکار در بیلد توسط `include/build_time.h` و `version.json` (ماکروی `VERSION`).
- **دستورالعمل جامع و نظام‌نامه اجرایی ایجنت‌ها (ارکان ۹‌گانه):** [`docs/AGENT_PLAYBOOK_FA.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/AGENT_PLAYBOOK_FA.md) — **الزامی برای مطالعه قبل از شروع هر تسک.**
- **قرارداد اصلی هوش مصنوعی:** [`AGENTS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/AGENTS.md)

### مستندات فنی تخصصی (بخوان قبل از لمس فایل مربوطه)
| مستند | زمان مطالعه | کاربرد |
|---|---|---|
| [`docs/GIT_CHECKPOINT_PROTOCOL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/GIT_CHECKPOINT_PROTOCOL.md) | **قبل از هر کار ریسکی یا بازگردانی** | **پروتکل بازگردانی جراحی (Surgical Rollback) و جلوگیری از حذف کارهای چند روز قبل** |
| [`docs/CHANGE_IMPACT_MATRIX.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/CHANGE_IMPACT_MATRIX.md) | قبل از هر تغییر چندفایله | ماتریس وابستگی فایل‌ها و زنجیره‌های خطای آبشاری |
| [`docs/WELDING_CYCLE_FSM.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/WELDING_CYCLE_FSM.md) | قبل از هر تغییر در `tasks.h` | نقشه کامل ۱۴ گام چرخه جوش، اینترلاک‌ها، E-Stop |
| [`docs/INTER_FILE_API_CONTRACT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/INTER_FILE_API_CONTRACT.md) | قبل از هر تغییر بین‌فایلی | قرارداد دقیق جریان داده بین `screens.c`, `tasks.h`, `vars.h`, `storage.h` |
| [`docs/OPERATOR_VOCABULARY.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/OPERATOR_VOCABULARY.md) | هنگام خواندن پرامپت کاربر | واژه‌نامه تخصصی اصطلاحات کارگاهی (لنگه، چهارچوب، فک، ذوب و...) |
| [`docs/UI_SCREEN_INVENTORY.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/UI_SCREEN_INVENTORY.md) | قبل از هر تغییر در UI | فهرست صفحات، شناسه‌های EEZ، ویجت‌ها و لایه‌بندی مودال‌ها |
| [`docs/ARCHITECTURE_DECISIONS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/ARCHITECTURE_DECISIONS.md) | قبل از هر ریفکتور ساختاری | سوابق تصمیمات معماری (ADR) و چرایی انتخاب الگوها |
| [`docs/PROMPT_EXAMPLES_LIBRARY.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/PROMPT_EXAMPLES_LIBRARY.md) | برای فهم سبک گفتار کاربر | ۱۵ نمونه واقعی از بسط پرامپت‌های کوتاه به پیاده‌سازی کامل |
| [`docs/KNOWN_ISSUES_AND_DEBT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/KNOWN_ISSUES_AND_DEBT.md) | قبل از هر bugfix یا refactor | بدهی‌های فنی شناخته‌شده و workaroundهای آگاهانه که نباید لمس شوند |
| [`docs/HARDWARE_IO_PINOUT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/HARDWARE_IO_PINOUT.md) | قبل از هر تغییر پین یا سنسور | جدول جامع پایه‌ها، ترمینال‌ها و شیرهای پنوماتیک برد HAFFLER ETV3A |
| [`docs/ACCEPTANCE_TEST_SPEC.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/ACCEPTANCE_TEST_SPEC.md) | قبل از اعلام پایان هر تسک | ۵ سطح تست پذیرش، Definition of DONE، طبقه‌بندی نقص |

- **ایندکس و نقشه راه کلیه مستندات فنی:** [`docs/README.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/README.md)
- **معماری چت‌بات هوشمند صنعتی و فیزیک اسکرین‌سیور:** `docs/AI_CHATBOT_AND_SCREENSAVER_SPEC_FA.md`
- **دانشنامه و مرجع فنی جامع پروژه:** [`docs/WELDING_HMI_KNOWLEDGE_FA.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/WELDING_HMI_KNOWLEDGE_FA.md) — ⚠️ **توجه اکید:** این فایل دیکشنری مرجع است و بیش از ۲۵۰۰ خط دارد؛ **هرگز نباید به‌صورت کامل باز یا لود شود.** فقط کلمه یا مفهوم مورد نیاز را با `grep_search` یا جستجوی محدود ۲۰ الی ۵۰ خطی بخوانید.
- **README کلی پروژه:** `README.md`
- **دفترچه سوابق مهندسی و قفل‌های فعال:** `docs/WORKLOG.md`


## 1) بیلد / آپلود / ابزار

- تنظیمات بیلد: `platformio.ini` (env: `esp32-s3-devkitc-1`, platform espressif32@6.9.0, LVGL 9.4, PSRAM فعال)
- **دستور بیلد (فقط کامپایل محلی):**
  ```powershell
  & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
  ```
- **دستور آپلود (⚠️ اکیداً فقط با دستور صریح و مستقیم کارفرما):**
  ```powershell
  & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM8
  ```
  *(آپلود خودسرانه اکیداً ممنوع است. هرگز روی COM3 که پورت تله Intel AMT است آپلود نکنید).*
- **بکاپ چرخشی و کامیت گیت (الزامی بعد از هر ۲ الی ۳ اصلاح):**
  ```powershell
  python tools/backup_manager.py commit "توضیح اصلاح انجام شده"
  ```

## 2) نقشهٔ کد اصلی (src/ و include/)

⚠️ در `src/` فایل‌های `ui_image_*.c` و `ui_font_*.c` دادهٔ باینری تولیدشده توسط EEZ Studio هستند — هرگز برای فهمیدن منطق باز نکن؛ فقط اگر خودِ همان asset باید عوض شود سراغش برو. این فایل‌ها در `.aiderignore` نادیده گرفته شده‌اند.

### منطق اصلی برنامه
| بخش | فایل | توضیح |
|---|---|---|
| نقطهٔ ورود / setup / loop | `src/main.cpp` | راه‌اندازی، تسک‌ها، حلقهٔ اصلی |
| منطق UI / صفحات / ویجت‌ها | `src/screens.c` | رفتار صفحات، گیج‌ها، دما، سیکل جوش |
| هدر متناظر screens | `include/screens.h` | prototype‌ها و struct صفحات |
| استایل‌های UI | `src/styles.c`, `include/styles.h` | |
| متغیرهای global UI | `src/vars.c`, `include/vars.h` | |
| رشته‌های چندزبانه | `src/ui_localization.cpp`, `include/ui_localization.h` | پیام‌های وضعیت/خطا |
| درایور LCD | `src/display_port.cpp`, `include/display_port.h` | `my_disp_flush` |
| تسک‌های FreeRTOS | `include/tasks.h` | منطق حرارتی و کنترل رله/SSR |
| ذخیره‌سازی | `include/storage.h`, `include/setting.h` | NVS/تنظیمات |

## 3) چک‌لیست ۹‌گانه ایجنت‌ها قبل از اقدام

1. **سرعت بالا:** استفاده از Bounded Reads (فقط خطوط هدف با grep)، پرهیز از اسپان بی‌مورد ساب‌ایجنت.
2. **کیفیت زیمنس:** قطعیت و بدون حدس و گمان؛ عدم نفی اینترلاک‌های ایمنی (E-Stop، المنت و فک‌ها).
3. **بسط پرامپت:** ترجمه پرامپت کوتاه فارسی به اسپک کامل انگلیسی و پوشش تمام زوایای سخت‌افزاری.
4. **ثبت دانش:** به‌روزرسانی فوری مستندات و دانشنامه در صورت کشف هر رفتار یا پین جدید.
5. **قفل همزمانی:** بررسی `git status --short` و بخش قفل‌های فعال در `docs/WORKLOG.md` برای جلوگیری از تداخل چت‌ها.
6. **عدم شلوغ‌کاری:** باز نکردن فایل‌های باینری، رندر و فونت‌های سنگین (`ui_image_*.c`).
7. **ایده‌پردازی عمیق:** پیشنهاد ایده‌های پخته با بنچ‌مارک صنعتی، بررسی مصرف RAM/Flash و ایمنی.
8. **پاسخ ساده و فارسی:** توضیحات شفاف، بدون تکلف و با لینک مستقیم به فایل‌ها.
9. **تحویل ۳۶۰ درجه بدون باگ:** پیاده‌سازی کامل (UI + منطق + NVS + زبان + ایمنی) و اعتبارسنجی با بیلد موفق.
