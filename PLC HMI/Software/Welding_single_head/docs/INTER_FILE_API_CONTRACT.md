# قرارداد API بین‌فایلی — Welding HMI (ESP32-S3)
**نسخه:** 1.0.0 | **تاریخ:** 1405-06-09 | **پروژه:** Welding_single_head

> این سند مرجع قطعی جریان داده و قراردادهای API بین فایل‌های اصلی پروژه است.  
> هر ایجنت باید **قبل از ویرایش هر فایل** این سند را مطالعه کند تا بدون خواندن کد منبع کامل، تصویر دقیقی از معماری داشته باشد.

---

## ۱. نقشه کلی جریان داده

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CORE 0  (System & Process)                   │
│                                                                       │
│   tasks.h ──set_var_*()──► vars.h/vars.c ◄──get_var_*()── screens.c │
│       │                        │                               │      │
│       │                   Shared State Bus                     │      │
│       │                  (EEZ Flow generated)                  │      │
│       │                                                        │      │
│       ▼                                                        ▼      │
│   GPIO / SSR / Pneumatics                              LVGL Widgets   │
│   MAX6675 Thermocouple                                 (Core 1 UI)    │
│   FreeRTOS Timers                                                     │
├─────────────────────────────────────────────────────────────────────┤
│                         CORE 1  (UI Thread)                          │
│                                                                       │
│   screens.c ──get_var_*()──► vars.h/vars.c                           │
│   screens.c ──set_var_element_temperature_target()──► vars.h/vars.c  │
│   screens.c ──(NVS read/write via storage.h)──► Flash NVS            │
└─────────────────────────────────────────────────────────────────────┘

agent_hmi.h ── Serial UART 115200 ──► SAFE / SHOT / TAP / STAT / GOTO
```

**قانون آهنین:** `tasks.h` هرگز مستقیم با LVGL تعامل نمی‌کند. `screens.c` هرگز `digitalWrite()` صدا نمی‌زند.

---

## ۲. vars.h / vars.c — اتوبوس وضعیت مشترک

> فایل‌های تولیدشده توسط EEZ Flow. **دستی ویرایش نکنید.**  
> تمام دسترسی بین ماژول‌ها فقط از طریق جفت‌های getter/setter زیر انجام می‌شود.

### ۲.۱ متغیرهای چرخه جوشکاری

| متغیر | Getter | Setter | نوع | نویسنده | خواننده | توضیح |
|--------|--------|--------|------|---------|---------|-------|
| `step_number` | `get_var_step_number()` | `set_var_step_number(int32_t)` | `int32_t` | `tasks.h` | `screens.c` | شماره مرحله فعال (۰–۱۳) |
| `step_process` | `get_var_step_process()` | `set_var_step_process(bool)` | `bool` | پدال / `tasks.h` | `tasks.h` | فلگ شروع مرحله؛ با فشار پدال `true`، در هر مرحله توسط `tasks.h` پاک می‌شود |
| `cycel_started` | `get_var_cycel_started()` | `set_var_cycel_started(bool)` | `bool` | `tasks.h` | `screens.c` | نشان‌دهنده E-Stop فعال بودن چرخه برای نمایش UI |

### ۲.۲ متغیرهای حرارتی

| متغیر | Getter | Setter | نوع | نویسنده | خواننده | توضیح |
|--------|--------|--------|------|---------|---------|-------|
| `element_temperature` | `get_var_element_temperature()` | `set_var_element_temperature(double)` | `double` | حلقه حرارتی `tasks.h` | گیج `screens.c` | دمای لحظه‌ای ترموکوپل (°C) |
| `element_temperature_target` | `get_var_element_temperature_target()` | `set_var_element_temperature_target(double)` | `double` | `screens.c` (UI) | PID `tasks.h` | دمای هدف از NVS — نوشتن باید atomic باشد |
| `thermocouple_error` | `get_var_thermocouple_error()` | `set_var_thermocouple_error(bool)` | `bool` | `tasks.h` | پنل خطای `screens.c` | خطای سنسور؛ با `true` SSR_1 خاموش می‌شود |

> **ایمنی حرارتی:** اگر `get_var_thermocouple_error()` مقدار `true` داشته باشد،  
> `SSR_1` باید بلافاصله HIGH (OFF) شود. هیچ ایجنتی مجاز به دور زدن این منطق نیست.

### ۲.۳ تایمرهای زمانی

| متغیر | Getter | Setter | نوع | نویسنده | خواننده | توضیح |
|--------|--------|--------|------|---------|---------|-------|
| `welding_time` | `get_var_welding_time()` | `set_var_welding_time(int32_t)` | `int32_t` | `screens.c` / NVS | `tasks.h` | زمان دوره جوش (ثانیه) |
| `cooling_time` | `get_var_cooling_time()` | `set_var_cooling_time(int32_t)` | `int32_t` | `screens.c` / NVS | `tasks.h` | زمان خنک‌شدن (ثانیه) |
| `josh_is_working` | `get_var_josh_is_working()` | — | `bool` | `tasks.h` | `screens.c` | فلگ فاز جوش فعال |
| `cool_is_working` | `get_var_cool_is_working()` | — | `bool` | `tasks.h` | `screens.c` | فلگ فاز خنک‌سازی فعال |
| `josh_counter` | `get_var_josh_counter()` | `set_var_josh_counter(int32_t)` | `int32_t` | `tasks.h` | نمایشگر شمارش `screens.c` | ثانیه‌های باقی‌مانده جوش |
| `cooling_counter` | `get_var_cooling_counter()` | `set_var_cooling_counter(int32_t)` | `int32_t` | `tasks.h` | نمایشگر شمارش `screens.c` | ثانیه‌های باقی‌مانده خنک‌سازی |

### ۲.۴ متغیرهای UI / وضعیت سیستم

| متغیر | Getter | نوع | نویسنده | توضیح |
|--------|--------|------|---------|-------|
| `test_mode` | `get_var_test_mode()` | `bool` | `tasks.h` / NVS | حالت تست مدیر — قابلیت‌های اضافه |
| `is_wifi_connected` | `get_var_is_wifi_connected()` | `bool` | `tasks.h` WiFi task | اتصال WiFi برای UI |
| `download_panle_state` | `get_var_download_panle_state()` | `int32_t` | `tasks.h` OTA | وضعیت پنل دانلود OTA |
| `waitig_panle_state` | `get_var_waitig_panle_state()` | `int32_t` | `tasks.h` OTA | وضعیت پنل انتظار OTA |
| `frame_numbers` | `get_var_frame_numbers()` | `int32_t` | `tasks.h` | شماره قطعه فعلی در دسته |
| `total_frame_numbers` | `get_var_total_frame_numbers()` | `int32_t` | `screens.c` / NVS | تعداد کل قطعات دسته |

---

## ۳. storage.h — لایه پایداری NVS

**Namespace:** `"welding"` (از طریق `Preferences.h` آردوینو)  
**Magic Number:** ثابت `MAGIC_NUMBER` — در هر راه‌اندازی اول بررسی می‌شود؛ اگر مطابقت نداشت، NVS ریست می‌شود.

> **قانون مطلق:** تمام عملیات `save()` باید درون یک تراکنش باز `preferences.begin() ... preferences.end()` قرار بگیرند.  
> **هرگز** خارج از تراکنش `put` صدا نزنید.

### ۳.۱ پروفایل‌های کاربری (۸ پروفایل)

| کلید NVS | نوع | محدوده مجاز | توضیح |
|----------|------|------------|-------|
| `"WT1"` .. `"WT8"` | `int32_t` | [1, 120] ثانیه | زمان جوش هر پروفایل |
| `"CT1"` .. `"CT8"` | `int32_t` | [1, 120] ثانیه | زمان خنک‌سازی هر پروفایل |
| `"TT1"` .. `"TT8"` | `double` | [100.0, 350.0] °C | دمای هدف هر پروفایل |
| `"SeUser"` | `int32_t` | [1, 8] | ایندکس پروفایل انتخاب‌شده |

### ۳.۲ نگاشت پین GPIO (صفحه IO Remap)

| کلید NVS | عملکرد | جهت | توضیح |
|----------|---------|------|-------|
| `"pTABLE"` | TABLE solenoid | خروجی | میز پنوماتیک |
| `"pPRESSL"` | PRESS_LEFT solenoid | خروجی | گیره چپ |
| `"pPRESSR"` | PRESS_RIGHT solenoid | خروجی | گیره راست |
| `"pELEMENT"` | ELEMENT solenoid | خروجی | پیشروی المنت — **فقط با گیره‌های Engaged** |
| `"pSHABLON"` | SHABLON solenoid | خروجی | شابلون |
| `"pSSR1"` | SSR_1 relay | خروجی | رله حرارتی — `ON=LOW`, `OFF=HIGH` |
| `"pZARBEH1"` | ZARBEH1 solenoid | خروجی | ضربه ۱ |
| `"pZARBEH2"` | ZARBEH2 solenoid | خروجی | ضربه ۲ |
| `"pSNELEM"` | sensor ELEMENT | ورودی | سنسور موقعیت المنت |
| `"pSNSHBL"` | sensor SHABLON | ورودی | سنسور موقعیت شابلون |
| `"pSNTABLE"` | sensor TABLE | ورودی | سنسور میز |
| `"pEMERG"` | EMERGENCY pin | ورودی | E-Stop — قطع فوری همه خروجی‌ها |
| `"pPEDAL"` | PEDAL pin | ورودی | پدال شروع چرخه |

> **ایمنی پین:** هرگز `ON=LOW` / `OFF=HIGH` را بدون تأیید صریح صاحب پروژه تغییر ندهید.

### ۳.۳ کلیدهای سیستمی

| کلید NVS | نوع | توضیح |
|----------|------|-------|
| `"magic"` | `uint32_t` | بررسی نسخه Schema NVS — اگر مطابقت نداشت: ریست پین‌ها |
| `"fwBld"` | `String` | آخرین Build UID — تغییر آن ریست نگاشت پین را اجرا می‌کند |
| `"version"` | `String` | رشته نسخه ذخیره‌شده |
| `"firmware_partion_select"` | `int32_t` | انتخاب پارتیشن OTA |

---

## ۴. tasks.h ↔ screens.c — جریان داده یک‌طرفه

### ۴.۱ جهت نوشتن/خواندن

```
tasks.h  ──[set_var_*()]──►  vars.h/vars.c  ◄──[get_var_*()]──  screens.c
   │                                                                  │
   │  فقط GPIO مستقیم                               فقط LVGL widgets  │
   │  فقط Core 0                                    فقط Core 1        │
   └──────────────────────────────────────────────────────────────────┘
```

### ۴.۲ قراردادهای ایمنی نخ (Thread Safety)

| مورد | قرارداد |
|------|---------|
| `set_var_element_temperature_target()` | باید atomic double write باشد (یا mutex محافظت‌شده) — `screens.c` از Core 1 و `tasks.h` از Core 0 هر دو به آن دسترسی دارند |
| ISR → UI | ISR فقط `volatile bool` فلگ تنظیم می‌کند. `lv_*` هرگز از ISR صدا زده نمی‌شود |
| `emergency_operation()` | از ISR یا اولین لبه فعال E-Stop صدا زده می‌شود — باید همه خروجی‌ها را بلافاصله HIGH کند |

### ۴.۳ خلاصه تبادل داده حلقه اصلی

```
[Core 0 / tasks.h loop]                    [Core 1 / screens.c tick]
─────────────────────                       ─────────────────────────
1. read MAX6675                             1. get_var_element_temperature()  -> گیج
2. set_var_element_temperature()            2. get_var_step_number()          -> برچسب
3. run PID with target                      3. get_var_josh_counter()         -> شمارنده
4. set_var_thermocouple_error()             4. get_var_thermocouple_error()   -> پنل خطا
5. run step machine                         5. get_var_cycel_started()        -> حالت E-Stop
6. set_var_step_number()
7. set_var_josh_counter()
8. set_var_cooling_counter()
9. set_var_cycel_started()
```

---

## ۵. agent_hmi.h — دستورات اتوماسیون سریال

**پورت:** UART0 — **Baud:** 115200 — **پایان‌بند:** `\n`

### ۵.۱ جدول دستورات

| دستور | فرمت ورودی | پاسخ | توضیح |
|--------|-----------|------|-------|
| `PING` | `PING\n` | `PONG Welding_HMI VER=x.x.x` | زنده‌بودن سیستم |
| `HELP` | `HELP\n` | فهرست همه دستورات | راهنما |
| `STAT` | `STAT\n` | `STAT FREE_INT=%u FREE_PSRAM=%u SSR=%d ELEM=%d TBL=%d PL=%d PR=%d` | وضعیت GPIO + Heap |
| `MEM` | `MEM\n` | گزارش تفصیلی Heap | حافظه داخلی و PSRAM |
| `SAFE` | `SAFE\n` | `OK SAFE_EXECUTED` | همه ۷ خروجی HIGH (OFF) فوری |
| `SHOT` | `SHOT\n` | `SHOT_BEGIN 480 320 RGB565LE` ... `SHOT_END` | Framebuffer 480x320 به‌صورت Base64 RGB565 |
| `PRESS x y` | `PRESS 240 160\n` | `OK PRESS 240 160` | شبیه‌سازی لمس (0–479, 0–319) |
| `REL x y` | `REL 240 160\n` | `OK REL 240 160` | شبیه‌سازی رها کردن لمس |
| `TAP x y [ms]` | `TAP 240 160 120\n` | `OK TAP 240 160` | لمس کوتاه (پیش‌فرض 120ms) |
| `GOTO id` | `GOTO 3\n` | `OK GOTO 3` | انتقال به صفحه با EEZ Screen ID |
| `LANG id` | `LANG 0\n` | `OK LANG 0` | تغییر زبان (0=FA 1=EN 2=TU 3=RU) |
| *(ناشناخته)* | هر دستور دیگر | `ERR UNKNOWN_COMMAND` | خطای دستور نامعتبر |

### ۵.۲ قراردادهای مهم agent_hmi

- دستور `SAFE` اولویت مطلق دارد — نباید به هیچ Mutex وابسته باشد.
- `SHOT` بافر را از Framebuffer اصلی می‌گیرد و ممکن است تا 300ms طول بکشد.
- دستورات `PRESS`/`REL`/`TAP` به Core 1 منتقل شده و توسط LVGL event loop پردازش می‌شوند.
- اعداد در `STAT` از `ESP.getFreeHeap()` و `ESP.getFreePsram()` و `digitalRead(pin)` خوانده می‌شوند.

---

## ۶. قوانین ثابت برای همه ایجنت‌ها

| # | قانون |
|---|-------|
| 1 | `tasks.h` هرگز `lv_*` صدا نمی‌زند — فقط `volatile flag` تنظیم می‌کند |
| 2 | `screens.c` هرگز `digitalWrite()` صدا نمی‌زند |
| 3 | `storage.h` save همیشه درون `preferences.begin()/end()` |
| 4 | `set_var_element_temperature_target()` از UI (Core 1) باید atomic باشد |
| 5 | اگر `get_var_thermocouple_error()==true` → SSR_1 باید HIGH (OFF) بماند |
| 6 | E-Stop → همه ۷ خروجی فوراً HIGH؛ هیچ تأخیری مجاز نیست |
| 7 | پین‌ها: `ON=LOW`, `OFF=HIGH` — هرگز بدون تأیید مالک تغییر نده |
| 8 | ELEMENT solenoid فقط وقتی PRESS_LEFT و PRESS_RIGHT هر دو Engaged هستند فعال می‌شود |

---

*آخرین به‌روزرسانی: 1405-06-09 | مرجع: کد منبع `vars.h`, `storage.h`, `tasks.h`, `screens.c`, `agent_hmi.h`*
