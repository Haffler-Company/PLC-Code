# استانداردهای اختصاصی پروژه تست و عیب‌یابی سخت‌افزار (Config Test Rig)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه برای عیب‌یابی، اعتبارسنجی کارخانه‌ای (FAT)، تست سلامت نمایشگر، تست تاچ خازنی، و کنترل رله‌ها و سنسورهای سخت‌افزار استاندارد هافلر طراحی شده است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Test Rig HMI)
* **داشبورد وضعیت پین‌ها (I/O Status Grid):** نمایش ماتریس ورودی‌ها و خروجی‌ها به صورت کلیدهای با لبه‌های گرد؛ ورودی‌های فعال با رنگ سبز درخشان (`#30D158`) و غیرفعال با خاکستری تیره (`#21262D`).
* **تست رنگ و پیکسل نمایشگر (RGB Screen Calibration):** تست صفحات تمام‌رنگ مشکی، سفید، قرمز، سبز و آبی برای کشف پیکسل سوخته یا خطای انتقال SPI.
* **کالیبراسیون تاچ خازنی:** صفحه تست ترسیم نقطه لمس (Touch Crosshair) با فیدبک آنی موقعیت پیکسل `(X, Y)`.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **حالت تست دستی امن (Interlocked Manual Test):** رله‌ها در حالت تست نمی‌توانند بیش از ۵ ثانیه مداوم روشن بمانند تا از داغ شدن بوبین‌ها در کارگاه جلوگیری شود.
* **پایش سلامت تغذیه و نویز:** بررسی مداوم ریست‌های ناخواسته ناشی از نویز (Brownout Detector).

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
