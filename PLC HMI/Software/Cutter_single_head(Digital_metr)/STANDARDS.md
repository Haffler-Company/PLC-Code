# استانداردهای اختصاصی دستگاه اره تک‌سر و متر دیجیتال (Cutter Single Head)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه شامل دستگاه اره تک‌سر برش پروفیل‌های UPVC و آلومینیوم مجهز به متر دیجیتال اتوماتیک است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Cutter HMI)
* **قهرمان صفحه (Hero Readout):** مقدار طول برش فعلی و هدف (`Target Length` و `Current Position`) باید بزرگترین و واضح‌ترین المان صفحه با فونت‌های درشت (`digit80` یا `digit100`) باشند.
* **کارت‌های تنظیمات:** تنظیمات کالیبراسیون و ضرایب پالس انکودر باید داخل کارت‌های شیک تیره با برچسب فارسی و کادر ۱ پیکسلی قرار گیرند.
* **کیپد لمسی (Numpad):** اهداف لمسی کلیدهای اعداد حداقل ۵۲×۴۴ پیکسل باشند تا ورود طول برش توسط اپراتور با دستکش بدون خطا انجام شود.
* **صفر لگ:** هیچ سایه نرم‌افزاری بلادرنگ روی ارقام متحرک انکودر مجاز نیست؛ نمایشگر باید با فریم‌ریت بالای ۵۰ هرتز موقعیت تیغه را دنبال کند.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **اینترلاک کلمپ و تیغه (Absolute Hardware Interlock):** تیغه اره هرگز نباید تا پیش از بسته شدن کامل فک کلمپ و رسیدن سنسور فشار به مقدار ایمن فعال شود.
* **تایم‌اوت حرکت فنس و اره:** برای حرکت فنس متر دیجیتال به موقعیت هدف و برای کورس برگشت تیغه، تایم‌اوت‌های مستقل تعریف شده است.
* **استاپ اضطراری:** قطع فوری تمام رله‌های Active-LOW حرکت تیغه و ترمز موتور اسپیندل در صورت فشردن E-STOP.

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
