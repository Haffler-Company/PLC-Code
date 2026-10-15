# شبیه‌ساز نمایشگر (هدف اصلی)

**فایل اصلی:** `simulator/display.html`  
پیکسل‌ها = SHOT واقعی از LCD تو (`simulator/refs/ref_*.png`).

تازه کردن بعد از آپلود کد:
```powershell
python tools/refresh_display_refs.py --port COM8
```
بعد Ctrl+F5 روی `display.html`.

پورت CP210x معمولاً **COM8** است (COM3 = Intel AMT، اشتباه است). Baud دیباگ: **115200**.

---

# شبیه‌ساز — حقیقت تلخ و ابزار درست

## این پروژه سه لایه دارد (با هم قاطی نکن)

| لایه | فایل | چه کار می‌کند | چه کار نمی‌کند |
|------|------|----------------|-----------------|
| **۱) منطق/سناریو** | `tools/machine_sim.py` + `simulator/scenario_lab.html` | بندهای `SCENARIO_FA`، لقی، لیست، FFD، گیت تنظیمات، E-stop | ظاهر پیکسلی LCD |
| **۲) مرجع عکس** | `simulator/hmi_hub.html` + `refs/*.png` | کلیک روی عکس SHOT قدیمی | کد فریمور را اجرا نمی‌کند |
| **۳) حقیقت پیکسل** | `SHOT` برد (`tools/hmi_screenshot.py`) | ظاهر واقعی مانیتور تو | بدون برد ممکن نیست |

**قانون:** ادعای «مثل مانیتور من» فقط با SHOT تازه. ادعای «سناریو درست است» با `machine_sim --test` سبز.

```powershell
python tools/machine_sim.py --test
# مرورگر:
# simulator/scenario_lab.html
python tools/sim_check.py
```

---
# شبیه‌ساز UI برای Metr & Cutter

رزولوشن هدف: **۴۸۰×۳۲۰**. دانش فنی فعال: `docs/TECHNICAL_KNOWLEDGE.md`.

---

## قانون عامل (اول این)

**قبل از آپلود برای تست منطق/لیست/جاگ/لقی:**

```powershell
python tools/sim_check.py
```

اختیاری عکس hub:

```powershell
python tools/sim_check.py --shot
# یا
python tools/hub_shot.py
```

| رتبه | ابزار | چه چیزی | برد؟ |
|------|--------|---------|------|
| **۰** | `sim_check.py` | وجود فایل‌ها + تست لیست + تست حرکت | خیر |
| **۱** | `SHOT` واقعی | پیکسل LCD | بله |
| **۲** | `hmi_hub.html` | تعامل Main/List/F1/F3 + لاگ حرکت/لیست | خیر |
| **۳** | طراح F1/List/Main HTML | لی‌اوت قبل از پورت LVGL | خیر |
| **۴** | `list_ui_sim.py` / `motion_logic_sim.py` | منطق خالص | خیر |

ادعای «لی‌اوت درست شد» فقط با **SHOT برد** — hub/طراح مرجع قبل از فلش است.

---

## Hub کامل (twin واقعی) — v3

فایل‌ها: `simulator/hmi_hub.html` + `hmi_hub.js` + `hmi_hub.css`

**روش:** پس‌زمینه = SHOT واقعی LCD (`simulator/refs/ref_*.png`) + hit-zone روی مختصات `ui_extras` + overlay زنده فقط وقتی داده عوض شود.

| صفحه | مرجع / رفتار |
|------|----------------|
| Home/Ref | `refs/ref_home.png` + Start Ref / RefFail |
| اصلی | `refs/ref_main.png` — عدد زنده فقط بعد از انتخاب ردیف/حرکت/برش |
| لیست | `refs/ref_list.png` — ۵ ردیف، سوایپ، +/−، سبز→اصلی |
| F1 | idle / نامپد؛ stock/kerf/web |
| F2 | سرعت٪، init، start/end mm، جهت پایه |
| F3 | تست دستی I/Q (مثل طراح) |
| F4 | پورتال/PIN |
| F5 | ریست شمارنده شیفت |
| بهینه | FFD + Prev/Next شاخه + Move/Skip |

کنترل‌ها: Logic، Key2، Sensor، Help، Hits، Compare، Jog L/R، Stop، Cut OK، Next، Unref، RefFail.  
هوک: `window.__hmiSim.selfTest()` / `.goto('list')` / `.runFFD()`.  
اگر ظاهر با دستگاه فعلی فرق داشت → SHOT تازه بگیر و `refs/` را عوض کن.

---

## ۰) اسکرین‌شات واقعی از دستگاه

| فرمان | کار |
|--------|-----|
| `SHOT` | دامپ فریم‌بافر → PNG |
| `SCR1` / `SCR2` / `SCR3` | اصلی / F1 / لیست |

```powershell
python tools/hmi_screenshot.py --port COM3
```

- `dtr=False`, `rts=False` — وگرنه ریست
- بعد از آپلود ~۲۰ s صبر
- خروجی: `simulator/shots/*.png`

---

## ۱) طراح‌های HTML (بدون برد)

| فایل | نقش |
|------|-----|
| `simulator/settings-f1-designer.html` | طراح کامل F1 (درگ/اجرا) — بازیابی‌شده |
| `simulator/settings-f3-designer.html` | F3 تست دستی |
| `simulator/list-screen-designer.html` | مرجع بصری لیست ۵ ردیف |
| `simulator/main-screen-designer.html` | مرجع بصری صفحه اصلی |
| `simulator/hmi_hub.html` | شبیه‌ساز تعاملی همه‌صفحه |

```powershell
python tools/designer_f1_shot.py
python tools/hub_shot.py
```

---

## ۲) سیم منطق (بدون برد)

```powershell
python tools/list_ui_sim.py --test
python tools/motion_logic_sim.py --test
python tools/sim_check.py
```

`motion_logic_sim`: لقی چپ ۵mm، fine ÷۶، stop روی رها، حفظ موقعیت روی شکست رفرنس.

---

## ۳) SDL / QEMU / Wokwi

روزمره لازم نیست. LVGL درخت پروژه **۹.۴.۰** بماند.

---

## جریان کار پیشنهادی

1. باگ لیست/سطل/صفحه/جاگ-لقی؟ → `sim_check.py` سبز
2. ایده لی‌اوت؟ → hub یا طراح HTML
3. پورت به `ui_extras.h` (نه EEZ کامل)
4. فقط وقتی کاربر آپلود خواست → فلش
5. تأیید پیکسل → `SHOT` واقعی

سیستم طراحی: `docs/HMI_DESIGN_SYSTEM_APPLE_FA.md`

