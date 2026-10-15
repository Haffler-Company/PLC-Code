# استانداردهای اختصاصی دستگاه فرز کپی و شابلون‌زن (Copy Router HMI)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه برای فریمور و HMI دستگاه فرز کپی (سوراخ‌کاری جای دستگیره، قفل و شیار آب‌رو در پروفیل‌های UPVC و آلومینیوم) رزرو شده است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Copy Router)
* **انتخاب الگو و شابلون فرزکاری:** نمایش شماتیک جای قفل و دستگیره در کارت‌های مدرن تیره با لبه‌های گرد.
* **تنظیم دور موتور اسپیندل:** نمایش دور موتور با یک اسلایدر یا نوار پیشرفت شیک به رنگ آبی اپل (`#0A84FF`) همراه با عدد RPM درشت.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **اینترلاک کلمپ پنوماتیک پروفیل:** پیش از روشن شدن موتورهای پرسرعت فرز، کلمپ پروفیل باید فعال و سنسور فشار باد تایید دهد.
* **سیستم روغن‌کاری خودکار (Cooling / Lubrication Mist):** پاشش مه خنک‌کاری در حین فرزکاری و هشدار افت سطح مخزن روغن.

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
