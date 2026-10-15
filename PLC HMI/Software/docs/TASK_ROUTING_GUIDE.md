# راهنمای هوشمند مسیریابی وظایف هافلر (TASK_ROUTING_GUIDE.md)
## Haffler Intelligent Task Routing & Auto-Swarm Dispatch Protocol

> **الزام مطلق:** کلیه ایجنت‌ها MUST قبل از شروع هر تسک این سند را بخوانند و بر اساس ماتریس پایین، مدل و استراتژی اجرا را انتخاب کنند.

---

## ۱. قانون Auto-Swarm (فعال‌سازی خودکار موتور موازی)

### شرایط فعال‌سازی اجباری `haffler_swarm.py`:

| شرط | مثال |
|---|---|
| تسک شامل بیش از ۲ فایل | ممیزی کامل پروژه، ریفکتور UI |
| نوع تسک: ممیزی کد | «کد رو بررسی کن»، «عیب پیدا کن» |
| نوع تسک: بهینه‌سازی | «سریع‌تر کن»، «بهینه‌ترش کن» |
| نوع تسک: ترجمه چندزبانه | افزودن رشته به ui_lang |
| نوع تسک: تست استرس | فازینگ ورودی، سناریو خطا |
| نوع تسک: ممیزی چندپروژه | مقایسه بین ۱۱ ماشین |

### دستور اجرا:
```powershell
python "c:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\tools\haffler_swarm.py" --task "<توضیح تسک>"
```

---

## ۲. ماتریس مسیریابی — کدام مدل برای کدام تسک

| نوع تسک | مدل پیشنهادی | دلیل |
|---|---|---|
| کدنویسی C/C++ پیچیده | Claude Sonnet / Gemini Pro | استدلال عمیق، MISRA C |
| ممیزی سریع فایل‌ها | Gemini Flash Lite | سرعت بالا، هزینه کم |
| طراحی UI LVGL | Gemini Flash + Apple-HIG skill | تخصصی LVGL |
| ترجمه فارسی/ترکی/انگلیسی | Gemini Flash Lite (parallel) | موازی ۲۱ کلید |
| بهینه‌سازی FPS/DMA | Gemini Flash + lvgl-zero-lag skill | تخصصی رندرینگ |
| OTA/release package | Antigravity (current agent) | دسترسی ابزار کامل |
| تست نفوذ/security | Claude + strix-ot-security-pentest | تخصصی امنیت |
| عیب‌یابی هارد (جک/سنسور) | Gemini Flash + siemens-hardening skill | دانش PLC |
| پاسخ سریع کارفرما | هر مدل فعال | سریع‌ترین جواب |

---

## ۳. استراتژی اجرای سریع‌تر (Speed Optimization Rules)

### ۳.۱. پیش از اجرا — ۳۰ ثانیه اسکن:
1. خواندن `PROJECT_SNAPSHOT.md` همان پروژه (به‌جای اسکن ۵۰ فایل)
2. چک `git status --short` برای فایل‌های در حال ویرایش
3. بررسی `docs/WORKLOG.md` اگر موجود باشد

### ۳.۲. خواندن هدفمند — Zero Bulk-Read:
- **هرگز** کل فایل را بدون grep نخوانید.
- از `grep` برای پیدا کردن دقیق تابع/متغیر استفاده کنید.
- حداکثر ۵۰ خط در هر بار خواندن.

### ۳.۳. موازی‌سازی:
- ۳ ایجنت متخصص به‌طور همزمان با `haffler_swarm.py`:
  - ایجنت ۱: ممیزی MISRA C / ایمنی
  - ایجنت ۲: ممیزی Apple HIG / UI
  - ایجنت ۳: ممیزی زیمنس / FSM
- نتایج ادغام و تحویل می‌شوند.

---

## ۴. پروژه‌های هافلر — سریع‌دسترس

| پروژه | مسیر | وضعیت | نقطه ورود |
|---|---|---|---|
| ICE CREEAM | `ICE CREEAM/` | فعال/تولید | `PROJECT_SNAPSHOT.md` |
| Welding Single | `Welding_single_head/` | فعال/تولید | `docs/START.md` |
| Welding Double | `Welding_double_head/` | در انتظار کد | `PROJECT_SNAPSHOT.md` |
| Cutter Single | `Cutter_single_head(Digital_metr)/` | فعال/تولید | `docs/START.md` |
| Cutter Double | `Cutter double_head(Digital_metr)/` | در انتظار کد | `PROJECT_SNAPSHOT.md` |
| Bakery (Belt) | `Bakery/` | فعال/تولید | `docs/START.md` |
| Bakery Mixer | `bakery-mixer/` | فعال/توسعه | `PROJECT_SNAPSHOT.md` |
| Gas Filling | `Gas-filling/` | فعال/تولید | `PROJECT_SNAPSHOT.md` |
| Corner Cleaning | `Corner_cleaning/` | در انتظار کد | `PROJECT_SNAPSHOT.md` |
| Copy Router | `Copy_router/` | در انتظار کد | `PROJECT_SNAPSHOT.md` |
| Aluminum Punching | `Aluminum-punching/` | در انتظار کد | `PROJECT_SNAPSHOT.md` |

---

## ۵. الگوریتم تصمیم‌گیری ایجنت

```
دریافت تسک
    │
    ├── آیا PROJECT_SNAPSHOT.md موجود است؟
    │       YES → خواندن فوری (۳۰ ثانیه)
    │       NO  → git ls-files --others --exclude-standard | head -20
    │
    ├── آیا تسک شامل >2 فایل یا ممیزی است؟
    │       YES → فعال‌سازی haffler_swarm.py
    │       NO  → اجرای مستقیم single-agent
    │
    ├── آیا تسک شامل تغییر کد کنترل/ایمنی است؟
    │       YES → git status → confirm با کاربر → اجرا
    │       NO  → اجرای مستقیم
    │
    └── تحویل نتیجه با فرمت:
            ✅/❌ نتیجه
            📁 فایل‌های تغییریافته
            🔢 درصد پیشرفت
```

---

## ۶. قوانین غیرقابل نقض (RED LINES)

1. ❌ هرگز فلش/آپلود بدون «فلش کن» / «پروگرم کن» صریح کارفرما
2. ❌ هرگز فایل روی Desktop (`C:\Users\Haffler\Desktop`)
3. ❌ هرگز `git reset --hard` — فقط forward-fix
4. ❌ هرگز تغییر پولاریته پین بدون مدرک شماتیک
5. ❌ هرگز حدس و گمان — فقط خواندن مستقیم سورس
6. ✅ همیشه پاسخ فارسی درون `<div dir="rtl" style="text-align: right;">`
7. ✅ همیشه مهارت مادر `haffler` لود شده باشد
8. ✅ همیشه بعد از تغییر: `git status --short` + verify
