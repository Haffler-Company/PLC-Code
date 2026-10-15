# Code Review — Welding Single Head — 2026-08-29 (20-Agent Parallel, Read-Only)

> درخواست: ۲۰ ایجنت موازی فقط-خواندنی، بدون دست‌زدن به کد، هر نکته فنی و هر ۱٪ بهبود سرعت نمایشگر ثبت شود. اجرا: ۱۳ ایجنتِ مستقلِ موازی (۷ ایجنت باقی به دستور توقف متوقف شد). همه یافته‌ها با `file:line` و سطح ریسک.

## چکیده — اولویت‌بندی

| اولویت | دسته | ریسک | اثر سرعت |
|---|---|---|---|
| P0 | ایمنی حرارتی / E-STOP / اینترلاک المنت | CRITICAL | — |
| P1 | پایداری NVS / WiFi-TLS / Heap-PSRAM | HIGH | 2-8٪ |
| P2 | نمایشگر 50+ FPS / Dirty-Driven / سایه / بافر | MEDIUM | 1-5٪ |
| P3 | بیلد / پارتیشن / گیج‌ها | LOW | 1٪ |

---

## P0 — ایمنی (باید قبل از هر بهینه‌سازی سرعت رفع شود)

### 1) E-STOP با تاخیر — `include/tasks.h:2439,2717`
```c
// فعلی: اگر cycel_started==true و emergencyActive → فقط set_var_cycel_started(false); continue;
// خاموش‌کردنِ SSR/پرس‌ها به تیک بعدی app_task (حدود ۲۰ms) موکول می‌شود — نقض “immediately and unconditionally”.
```
**پیشنهاد (بدون ویرایش):** در همان `if (emergencyActive)` قبل از `continue` مستقیم `emergency_operation()` صدا زده شود (همان ۷ `digitalWrite(OFF)` که در `force_all_outputs_off_after_update:170` هست). `ota_return_sanitize_until_ms:2361` را به `300ms` محدود کن تا E-STOP واقعی ۲-۳ ثانیه ماسکه نشود.

### 2) اینترلاک المنت — `include/tasks.h:2533`
`Step6: digitalWrite(ELEMENT,ON)` بدونِ چکِ مجددِ `press_left_is_free / press_right_is_free` یا حالتِ خروجی. اگر `BACK` یا تایم‌اوتِ سنسور پرس‌ها را آزاد کند، المنت با پرسِ باز جلو می‌آید.
**پیشنهاد:** قبل از `ELEMENT ON` → `if(press_left_is_free||press_right_is_free) { emergency_operation(); break; }`.

### 3) اعتبارسنجی ترموکوپل — `include/tasks.h:941, 0x4`
`1024°C` فقط via `>1000` پوشش داده شده، `0x04` فقط غیرمستقیم از `max6675.cpp:46 return NAN` می‌آید. `0xFFFF` (قطع BUS) چک نمی‌شود.
**پیشنهاد:** `readRaw() &0x4 || raw==0xFFFF || raw==0` صریح قبل از `readCelsius()`.

---

## P1 — پایداری (ریسک هنگ/ریست)

### NVS — `include/storage.h:431,250,6944` و `src/main.cpp:6944`
- `save()` ۳۵ `put` بدون `begin()/end()` — تراکنش ناقص، ریسک پارگی در قطع برق.
- `saveInRowSilk:250` `end()` حذف شده (لیک هندل).
- `main.cpp:6944` `nvs_flash_erase()` با توکن `0xA2` کل `TpCal/TT/WT/CT/wifi` را پاک می‌کند — ناقض `Never wipe NVS`.
- `g_teflon_cycles:src/main.cpp:58` اصلاً در `storage.h` وجود ندارد (۰ نتیجه grep) — هر ریست صفر.
**پیشنهاد:** `save()` را `ensure_open();...;end();` تراکنشه کن؛ `wipe` را به `remove()` انتخابی تبدیل کن؛ `teflon_cycles:uint32` به `load()/save()` باتچ اضافه کن.

### WiFi/OTA/TLS — `include/tasks.h:1243,3087` و `src/display_port.cpp:140`
- ۶ جا `setInsecure()` — MITM؛ `lastfile` بدون `sha256` (درحالیکه `tools/copy_release_artifacts.py:122` هش می‌سازد).
- بافر نمایشگر `24` خط `DMA_INTERNAL` درحالیکه `WELDING_HMI_KNOWLEDGE:419` اجبار `PSRAM 48` برای `maxAlloc>100KB` TLS — تضاد `AGENTS.md:25` (۳۲ خط).
- `xMutex_Wifi` تایم‌اوت‌های ناهمسان `+2000/+4000/15000` و `vTaskSuspend(lvgl)+disableCore0WDT:3089` بلوکه طولانی.
**پیشنهاد:** `CA bundle` اضافه کن؛ `lastfile` را با `sha/size` ولیدیت کن؛ `display_port.cpp:132` را `PSRAM 32` اصلی و `DMA fallback 16` کن؛ تایم‌اوت‌ها را `5s` یکسان کن؛ `httpUpdate` را آسنکرون یا بدون `suspend` کن.

### PSRAM Heap — `include/lv_conf.h:43` و `src/lvgl_heap_esp32.c:17`
`LV_STDLIB_BUILTIN` است، پس `lvgl_heap_esp32.c` (PSRAM-prefer) dead code — LVGL الان `64KB` داخلی را می‌خورد.
**پیشنهاد:** `LV_STDLIB_CUSTOM` کن تا `~60KB` داخلی آزاد شود (≈۱۸٪).

### FreeRTOS — `src/display_port.cpp:72` و `src/main.cpp:7113`
`touch_read_cb` روی Core1 تا `50ms` `rtcMutex` می‌گیرد — ۵۰ms بلوکه UI. `stack_monitor_task` خاموش.
**پیشنهاد:** `tryTake 0ms` + صفِ نمونه از Core0؛ `HighWaterMark` را در `MEM-DIAG:7138` لاگ کن.

---

## P2 — نمایشگر 50+ FPS

### Dirty-Driven
- `src/screens.c:2139` `set_style_text_color` بدون گارد؛ `tasks.h:513` `ota_ui_apply_pct_widgets` بدون `if_changed`.
**پیشنهاد:** `last_target_col` و `strcmp/get_value` کش اضافه کن.

### سایه
- `tasks.h:582` `shadow 18` روی `g_ota_overlay` و `screens.c:1025` `shadow 16` روی `jack_time_error_panel` — حد مجاز مودال `4px`.
**پیشنهاد:** `≤4px` یا `0` + کش.

### بافر
- `display_port.cpp:132` `24` خط → `13.3` فلاش/فریم؛ `32` خط → `10` فلاش (-۳۰٪ `setAddrWindow`).
**پیشنهاد:** اگر `largest_free_block>65KB` باشد `32` را اول امتحان کن.

### گیج‌ها
- **دما:** `screens.c:7924` throttle `50ms` کلِ `tick_screen_screen_menu` را به `20FPS` محدود می‌کند.
  **پیشنهاد:** `temp` را از گِیتِ کلی جدا و `16-30ms` کن.
- **زمان:** `screens.c:1923` OOB `value=40` روی آرایه `40`؛ `1943` `i<=value` یک تیک اضافی؛ `main_time_dots` ۸۰ پوینتر استفاده نشده.
  **پیشنهاد:** `clamp 39` و `i<value`؛ آرایهٔ مرده را حذف کن.

### تاچ
- `agent_hmi.h:31` `g_agent_touch_*` غیر `volatile` (race)؛ `bb_captouch.cpp:248` تایم‌اوت `1000ms`.
**پیشنهاد:** `volatile` کن؛ تایم‌اوت `150ms`؛ `Wire.setClock(400000)` را بعدِ هر `Wire.begin` دوباره assert کن.

---

## P3 — بیلد/پارتیشن

- `partitions.csv:5` گپ `64KB` `0xE10000-0xE20000`؛ `REFERENCE.md` می‌گوید `8MB PSRAM` ولی `platformio.ini:17` `2MB`.
**پیشنهاد:** `spiffs` را به `0xE10000/0x1F0000` ببر؛ `psram_type` و داک را یکی کن؛ `-mfix-esp32-psram-cache-issue` اضافه کن.

---

## روش اجرا

- ۱۳ ایجنتِ `explore` فقط-خواندنی، هرکدام `Grep` + `Read` محدود (≤۸۰ خط)، بدون `Edit/Write`، به‌صورت موازی (مدیر `agent-orchestrator`).
- هیچ فایلی تغییر نکرد — همه موارد فوق پیشنهادِ مستند است.
- مبنا: `AGENTS.md` (Siemens/Apple Pillars) + `docs/WELDING_HMI_KNOWLEDGE_FA.md` + `HEAD=6b64f8d`.

## گام بعدی پیشنهادی (بدون اجرا)

1. P0 ایمنی (۲ مورد) → یک `WORKLOG lock` + پَچِ جراحیِ `tasks.h:2439,2533`.
2. P1 NVS/TLS/PSRAM → `storage.h` + `display_port.cpp` + `lv_conf.h`.
3. P2 نمایشگر → `screens.c` سایه/گارد/بافر و `main.cpp` رفعِ بلوکهٔ تاچ.

> این سند با ۱۳ گزارشِ مستقلِ موازی ساخته شد؛ ۷ ایجنتِ باقی به دستور توقف متوقف شد.
