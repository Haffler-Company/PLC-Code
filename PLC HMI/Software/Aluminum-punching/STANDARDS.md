# استانداردهای اختصاصی دستگاه پانچ آلومینیوم (Aluminum Punching HMI)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه برای کنترلر و نمایشگر دستگاه‌های پانچ هیدرولیک و پنوماتیک پروفیل‌های آلومینیومی درب و پنجره، نمای کرتین‌وال و قطعات صنعتی اختصاص دارد.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Punching HMI)
* **انتخاب قالب پانچ (Die Selection Carousel):** نمایش گرافیکی انواع قالب‌های پانچ در یک رابط انتخاب کارت‌محور با آیکون‌های وکتور تمیز.
* **شمارنده ضربه‌ها:** شمارنده بزرگ تعداد ضربات قالب و هشدار سرویس تیزکاری سنبه و ماتریس در کارت‌های مجزا.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **فرمان دو شستی ایمن (Two-Hand Safety Relay Guard):** کورس پانچ پرقدرت هیدرولیک نیازمند نگه داشتن دو شستی همزمان با حداکثر ۵۰۰ میلی‌ثانیه اختلاف زمانی است تا دست اپراتور زیر سنبه قرار نگیرد.
* **تایم‌اوت کورس برگشت جک:** در صورت گیرپاژ قالب در پروفیل، قطع شیر فشار و صادر شدن آلارم گیر مکانیکی.

---

### ۴. مهارت‌های هوش مصنوعی فعال در این پروژه
در مسیر `.agents/skills/` مهارت‌های زیر آماده فراخوانی هستند:
* `apple-industrial-hmi-design`
* `siemens-industrial-hardening`
* `lvgl-zero-lag-engine`

---

### ۵. قوانین حیاتی تعامل و ایمنی سخت‌افزار
1. **پاسخ‌گویی فوق‌خلاصه:** ایجنت باید گزارش‌ها را کاملاً خلاصه، تلگرافی و بدون کلمات اضافه ارائه دهد.
2. **درک اصطلاحات فارسی کارفرما:** مطابق با دستورالعمل [AGENT_COMMUNICATION_AND_PERSIAN_INTENT_RULES.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/AGENT_COMMUNICATION_AND_PERSIAN_INTENT_RULES.md).
3. **ممنوعیت مطلق فلش/پروگرم بدون اجازه صریح:** هرگز هیچ دستوری برای آپلود فریمور روی برد فیزیکی (pio run -t upload، esptool، upload.bat) اجرا نکنید مگر اینکه کارفرما مستقیماً نوشته باشد: **«فلش کن»** یا **«پروگرم کن»**.
