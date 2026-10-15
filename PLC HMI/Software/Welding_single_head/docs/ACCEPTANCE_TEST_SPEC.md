# مشخصات پذیرش کار — Welding HMI (تعریف رسمی «تمام‌شده»)

> نسخه: **1.0** — آخرین بروزرسانی: ۱۴۰۴/۰۶/۰۹  
> مرجع: [`include/agent_hmi.h`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/include/agent_hmi.h) · [`include/tasks.h`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/include/tasks.h) · [`AGENTS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/AGENTS.md)

---

## ۰. هدف این سند

این فایل **تنها معیار رسمی برای پذیرش یا رد هر تسک** در پروژه جوش تک‌سر است.  
هر ایجنت، پیش از اعلام «تمام شد»، باید تمام چک‌لیست‌های متناسب با نوع تغییر خود را پاس دهد.  
هیچ تسکی تمام نیست مگر همه موارد زیر صادق باشند.

---

## ۱. پروتکل سریال تست (agent_hmi.h)

همه تست‌های سریال با این تنظیم اجرا می‌شوند:

| پارامتر | مقدار |
|---|---|
| بود | `115200` |
| پورت مجاز | `COM17` / `COM8` / `COM4` |
| پورت ممنوع | ❌ **هرگز COM3** (تله Intel AMT) |
| فرمت دستور | متن ASCII + `\n` (Enter) |

### دستورات موجود و پاسخ مورد انتظار

| دستور | پاسخ مورد انتظار | توضیح |
|---|---|---|
| `PING` | `PONG welding_hmi_v150` | بررسی اتصال و نسخه |
| `STAT` | `STAT FREE_INT=%u FREE_PSRAM=%u SSR=%d ELEM=%d TBL=%d PL=%d PR=%d` | وضعیت حافظه و GPIO |
| `MEM` | `MEM FREE_INT=%u MIN_INT=%u FREE_PSRAM=%u` | گزارش دقیق هیپ |
| `SAFE` | `OK SAFE_EXECUTED` | اجرای توقف ایمن؛ تمام GPIO باید HIGH شوند |
| `SHOT` | `SHOT_BEGIN 480 320 RGB565LE … SHOT_END` | عکس فریم‌بافر (Base64) |
| `TAP x y [ms]` | `OK TAP x y ms` | شبیه‌سازی لمس |
| `PRESS x y` | `OK PRESS x y` | لمس پایدار |
| `REL` | `OK REL` | رها کردن لمس |
| `GOTO <screen_id>` | `OK GOTO <id>` | رفتن به صفحه |
| `LANG 1` | `OK LANG 1` | زبان انگلیسی |
| `LANG 0` | `OK LANG 0` | زبان فارسی (پیش‌فرض) |

> **نکته خوانش STAT:**  
> مقدار `1` = خاموش (HIGH)، مقدار `0` = روشن (LOW)، مقدار `-1` = پین تعریف نشده.  
> در استارتاپ سالم، همه: `SSR=1 ELEM=1 TBL=1 PL=1 PR=1`.

---

## ۲. مراحل سیکل جوش (step_number 0–13)

این جدول از [`tasks.h`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/include/tasks.h) استخراج شده است. هر تغییر در منطق سیکل باید با این ترتیب تطابق داشته باشد:

| مرحله | جک/عملگر | اقدام | ایمنی بحرانی |
|:---:|---|---|---|
| **0** | SHABLON | حرکت به جلو (ON) — پس از بررسی باز بودن همه جک‌ها | اگر جکی باز نباشد → `emergency_operation()` |
| **1** | TABLE | حرکت به جلو (ON) | — |
| **2** | PRESS_RIGHT | فشار (ON) + انتظار زمان `press_right_raft` | — |
| **3** | PRESS_LEFT | فشار (ON) + انتظار زمان `press_left_raft` | — |
| **4** | TABLE | برگشت (OFF) + انتظار سنسور | — |
| **5** | SHABLON | برگشت (OFF) + انتظار سنسور | — |
| **6** | ELEMENT | پیشروی المنت داغ (ON) | ⚠️ **اینترلاک سخت**: اگر `PRESS_LEFT` یا `PRESS_RIGHT` درگیر نباشند → `emergency_operation()` |
| **7** | TABLE | پیشروی میز به سمت المنت (ON) | ⚠️ **پایش پیوسته**: اگر فک در این لحظه ول شود → `emergency_operation()` |
| **8** | — | دوره ذوب: `welding_time` ثانیه | ⚠️ **پایش پیوسته فک‌ها** + ثبت دما (min/max/avg) |
| **9** | TABLE | برگشت میز (OFF) | — |
| **10** | ELEMENT | برگشت المنت (OFF) | — |
| **11** | TABLE | پیشروی برای فشار جوش (ON) | — |
| **12** | ZARBEH1/2 | دوره خنک‌کاری: `cooling_time` ثانیه + ضربه‌های پروفیل | شمارش کیفیت DVS 2207-1 و ثبت چرخه تفلون |
| **13** | همه | برگشت همه جک‌ها به حالت اولیه | `fast_jack()` یا `step_by_step_jack()` طبق تنظیم |

---

## ۳. چک‌لیست‌های پذیرش (به ترتیب الزامی)

### سطح ۱ — بیلد موفق (اجباری برای همه تسک‌ها)

```powershell
# اجرا از پوشه پروژه:
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

معیارهای قبولی:

| معیار | مقدار قابل قبول | هشدار |
|---|---|---|
| Exit code | `0` | هر عدد دیگری = BLOCKER |
| RAM usage | `< 45%` | مقدار فعلی ~46.6% است — **هرگز افزایش ندهید** |
| Flash usage | `< 90%` | — |
| کامپایلر وارنینگ `-Werror` | صفر | — |

> ⚠️ اگر RAM بالای 45% رفت: **BLOCKER** — قبل از هر کار دیگری باید رفع شود (پایداری DMA در خطر است).

---

### سطح ۲ — سلامت فریمور (برای هر تغییر در .cpp / .h)

پس از آپلود:

```powershell
# آپلود:
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM17

# سپس سریال مانیتور را باز کنید (115200 baud) و این دستورات را بفرستید:
```

```
PING
STAT
MEM
SAFE
```

| دستور | پاسخ قابل قبول |
|---|---|
| `PING` | باید `PONG welding_hmi_v150` برگردد |
| `STAT` | `SSR=1 ELEM=1 TBL=1 PL=1 PR=1` در استارتاپ |
| `MEM` | `FREE_INT` باید `> 102400` (بیش از ۱۰۰ کیلوبایت) |
| `SAFE` | `OK SAFE_EXECUTED` + تأیید مجدد با `STAT` که همه =1 |

---

### سطح ۳ — تأیید UI (برای هر تغییر در screens.c / styles.c / vars.c / ui_localization)

```
SHOT
GOTO <screen_id>
TAP <x> <y>
LANG 1
LANG 0
```

| مرحله | معیار |
|---|---|
| `SHOT` | تصویر با `tools/hmi_screenshot.py` قابل بررسی است؛ متن‌های فارسی کامل و بدون قطع |
| `GOTO <id>` | صفحه هدف بدون crash یا freeze باز می‌شود |
| `TAP x y` | دکمه/ویجت واکنش مورد انتظار نشان می‌دهد |
| `LANG 1` → `LANG 0` | هیچ متنی overflow نمی‌کند؛ RTL/LTR بدون اشکال |
| FPS | بیش از ۳۰ فریم در ثانیه |
| سایه | هیچ `shadow_width > 4px` در UI دینامیک |

#### نمونه بررسی صفحه اصلی (screen_id=0):
```
GOTO 0
SHOT
TAP 240 160
```

---

### سطح ۴ — تأیید ایمنی بحرانی (برای هر تغییر در tasks.h، اینترلاک‌ها، GPIO)

> این سطح در ارتباط با مراحل **۶، ۷، ۸** سیکل جوش (اینترلاک المنت + فک‌ها) الزامی است.

```
STAT
SAFE
STAT
```

| مرحله | معیار |
|---|---|
| `STAT` اول | `SSR=1` در استارتاپ (ترموکوپل هنوز گرم نشده) |
| `SAFE` | باید `OK SAFE_EXECUTED` برگردد |
| `STAT` دوم | تمام مقادیر `SSR=1 ELEM=1 TBL=1 PL=1 PR=1` |
| WORKLOG | باید قید شود **کدام اینترلاک** تأیید شد (E-Stop / ELEMENT clamp / thermocouple NaN) |

**قوانین سخت‌گیرانه ایمنی (هرگز نقض نشوند):**
- `SSR_1` هرگز وقتی MAX6675 مقدار NaN / 1024°C / open-circuit می‌دهد ON نماند.
- ELEMENT هرگز در step ۶/۷ وارد زون جوش نشود مگر هر دو `PRESS_LEFT=0` و `PRESS_RIGHT=0` (= ON).
- E-Stop باید همه سولنوئیدها + SSR را **بدون تأخیر** HIGH کند.
- پین‌های GPIO و سطح فعال (`ON=LOW, OFF=HIGH`) تحت هیچ شرایطی تغییر نمی‌کنند.

---

### سطح ۵ — تأیید پایداری NVS (برای هر تغییر در storage.h / setting.h)

```powershell
# ۱. فلش فریمور
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM17

# ۲. دستگاه را کاملاً برق بزنید (power cycle واقعی، نه ریست)
# ۳. پس از بوت:
```

```
PING
STAT
```

| معیار | توضیح |
|---|---|
| `PING` پاسخ می‌دهد | فریمور بوت کرده |
| تنظیمات دما و پروفیل محفوظ مانده | با مقایسه دستی یا اسکرین‌شات قبل/بعد |
| `MAGIC_NUMBER` در WORKLOG | مستند شده که چرخه پاک‌سازی NVS رخ نداده |
| بروزرسانی‌های بچ‌بندی‌شده | هیچ‌گاه `pref.begin()` بدون `pref.end()` نماند |

---

## ۴. تعریف رسمی «تمام‌شده» (Definition of DONE)

یک تسک **فقط** زمانی تمام است که **همه** موارد زیر صادق باشند:

- [ ] **سطح ۱** بیلد موفق، exit code = 0
- [ ] سطح‌های **۲/۳/۴/۵** متناسب با نوع تغییر، پاس شده‌اند
- [ ] [`docs/WORKLOG.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/WORKLOG.md) با تاریخ، شرح، فایل‌های شواهد، و `status=VERIFIED` بروز شده
- [ ] هیچ قفل فعالی در `## Active Locks` WORKLOG برای این تسک باقی نمانده (status=RELEASED)
- [ ] اگر هر اینترلاک ایمنی لمس شده: جمله صریح در WORKLOG که «ایمنی تأیید شد»

---

## ۵. طبقه‌بندی شدت نقص

| شدت | تعریف | اقدام فوری |
|---|---|---|
| **BLOCKER** 🔴 | بیلد fail یا E-Stop کار نمی‌کند | **توقف کامل؛ هیچ کار دیگری تا رفع** |
| **CRITICAL** 🔴 | رگرسیون در ایمنی حرارتی یا پنوماتیک | **رول‌بک فوری + رفع + تأیید سطح ۴** |
| **HIGH** 🟠 | RAM > 45%، از دست رفتن NVS، crash UI | رفع در همان جلسه |
| **MEDIUM** 🟡 | FPS < 30، سایه بیش از 4px، تأخیر ISR | رفع در جلسه بعد |
| **LOW** 🟢 | تایپو، خطای بصری جزئی | ثبت در WORKLOG و defer |

---

## ۶. دستورات سریع مرجع PowerShell

```powershell
# بیلد
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run

# آپلود به COM17
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM17

# مانیتور سریال (Ctrl+C برای خروج)
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor --port COM17 --baud 115200

# اسکرین‌شات از فریم‌بافر
python tools/hmi_screenshot.py --port COM17 --out screenshot_$(Get-Date -Format 'yyyyMMdd_HHmmss').png
```

```
# تست سریع سلامت (پس از هر آپلود):
PING
STAT
MEM
SAFE
STAT
```

---

## ۷. پیوندهای مرجع

| فایل | هدف |
|---|---|
| [`include/agent_hmi.h`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/include/agent_hmi.h) | پروتکل سریال — منبع اصلی دستورات تست |
| [`include/tasks.h`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/include/tasks.h) | سیکل جوش step 0-13 و اینترلاک‌های ایمنی |
| [`docs/WORKLOG.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/WORKLOG.md) | ثبت شواهد + قفل‌های فعال |
| [`docs/REFERENCE.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/REFERENCE.md) | پین‌های سخت‌افزار، نقشه GPIO |
| [`docs/AGENT_PLAYBOOK_FA.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/AGENT_PLAYBOOK_FA.md) | ارکان ۹‌گانه اجرایی ایجنت‌ها |
| [`AGENTS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/AGENTS.md) | قرارداد کامل ایجنت‌ها |