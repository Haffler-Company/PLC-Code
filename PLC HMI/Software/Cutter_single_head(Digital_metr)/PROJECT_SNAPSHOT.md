# PROJECT SNAPSHOT — Cutter Single Head (اره تک‌سر دیجیتال)
## نقطه ورود سریع ایجنت | فقط این سند را بخوان

---

## سخت‌افزار هدف
- **MCU:** ESP32-S3 DevKitC-1 — 8MB Flash، PSRAM، 240MHz dual-core
- **Display:** 480×320 ILI9341، TFT_eSPI/SPI2، 40MHz، rotation 3، TFT_BGR
- **Touch:** `bb_captouch`، SDA=GPIO8، SCL=GPIO9، orientation 270
- **Framework:** Arduino/PlatformIO + LVGL 9.4
- **Upload Port:** COM8 اصلی (921600/115200) — **هرگز COM3**

## پین‌اوت کامل (تمام Active-LOW)

| سیگنال | GPIO | عملکرد |
|---|---|---|
| PUL (Stepper Pulse) | 16 | پالس استپر متریک |
| DIR (Stepper Dir) | 42 | جهت استپر |
| EN (Stepper Enable) | 39 | فعال‌سازی درایور |
| Clamp 1 | 48 | گیره اول |
| Clamp 2 | 40 | گیره دوم |
| Spray | 15 | اسپری روانکار |
| Saw Motor | 35 | موتور اره |
| Lamp | 36 | چراغ کار |
| Lift | 38 | بالابر |
| Reference ADC (I0) | 6 | سنسور مرجع/هوم |
| Guard/E-Stop ADC (I6) | 5 | گارد + E-Stop |
| Blade-Return/Motor-Fault ADC | 4 | برگشت تیغه، خطای موتور |
| Start 1 | 3 | شستی استارت ۱ |
| Start 2 | 7 | شستی استارت ۲ |
| Joystick T1 | 21 | دیجیتال Pull-Up، 30ms debounce |
| Joystick T2 | 37 | دیجیتال Pull-Up |
| F-Keys / Save-Cancel | 1, 2 | Analog Ladder |

## معماری فریمور

| فایل | مسئولیت |
|---|---|
| `firmware/src/main.ino` | startup، globals، tasks spawn |
| `firmware/include/tasks/Task2.h` | LVGL loop، UI signals، SHOT |
| `firmware/include/tasks/Task3.h` | استپر، ISR، homing |
| `firmware/include/tasks/Task4.h` | **ممنوع‌الدسترس** — USB keyboard |
| `firmware/include/tasks/Task8.h` | cut flow FSM، safety |
| `firmware/include/app/GUI_Func.h` | callbacks، touch، display flush |
| `firmware/include/app/ui_extras.h` | UI chrome، keypad، list |
| `firmware/include/app/Pref_Func.h` | NVS/preferences |

## خطوط قرمز ایمنی — ABSOLUTE

- ❌ هرگز `Task4.h` (USB keyboard) بدون دستور صریح لمس نشود
- ❌ هرگز پالس استپر بدون بررسی home/ref تغییر کند
- ❌ هرگز backlash compensation به راست (فقط LEFT-only)
- ❌ E-Stop → قطع همه خروجی‌ها فوری <20ms
- ✅ UI callback → `GUI.pending_goto_scr` — هرگز مستقیم `loadScreen()`
- ✅ SRAM داخلی < 45% برای DMA پایدار

## وضعیت
- **Build:** `cd firmware && pio run`
- **Latest:** `artifacts/builds/digital-cutter-single-head-v1.7.62.metadata.json`
- **OTA Path:** ArvanCloud `ota-plc/upvc/cutter-single-head/`
- **مستندات:** `docs/REFERENCE.md` مرجع اصلی، `docs/WORKLOG.md` قفل‌های فعال
