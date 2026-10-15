# استانداردهای اختصاصی دستگاه بستنی‌ساز صنعتی (ICE CREAM Machine)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه فریمور صنعتی و HMI دستگاه‌های بستنی‌ساز قیفی صنعتی (شرکت ماشین‌سازی آستیاژ / هافلر) است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Ice Cream HMI)
* **قهرمان صفحه (Hero Readout):** گیج یا عدد درصد سفتی/قوام بستنی (Hardness / Consistency Percentage) و دمای مخزن شیر (Hopper) و سیلندر خنک‌کننده به صورت دو کارت تفکیک‌شده مدرن.
* **زیبایی‌شناسی شیک و لوکس:** استفاده از تم دارک مدرن اپل، آیکون‌های مینیمال برای پمپ هوا، همزن، شستشو و چرخه اتوماتیک؛ کارت‌های با لبه‌های گرد ۱۲ پیکسلی.
* **فیدبک اهرم خروج بستنی:** تغییر آنی استیت صفحه هنگام کشیدن اهرم خروج بستنی (Microswitch Front) با انیمیشن روان و بدون افت فریم.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **حفاظت ضد یخ‌زدگی سیلندر (Cylinder Freeze-Up Protection):** کنترل فشار بار موتور همزن (Beater Motor Overload)؛ اگر تسمه گیر کند یا بستنی بیش از حد سفت شود، کمپرسور بزرگ فوراً خاموش و همزن معکوس/قطع شود.
* **تایمر تاخیر استارت کمپرسور (Anti-Short Cycle Timer):** کمپرسور سرمایش هرگز نباید زودتر از ۳ دقیقه پس از خاموش شدن مجدداً استارت شود (محافظت در برابر فشار برگشتی گاز مبرد).
* **حالت شستشو (Wash Mode Safe Interlock):** در حالت شستشو، کمپرسورها حتماً قفل سخت‌افزاری و نرم‌افزاری باشند تا آب داخل سیلندر یخ نزند.

---

### ۴. مهارت‌های هوش مصنوعی فعال در این پروژه
در مسیر `.agents/skills/` مهارت‌های پنج‌گانه زیر فعال و تثبیت شده‌اند:
* `apple-industrial-hmi-design`: ارگونومی و طراحی رابط کاربری مدرن لمسی
* `apple-lvgl-component-pack`: ویجت‌ها، کیپدها و سلکتورهای استاندارد اپل
* `siemens-industrial-hardening`: انطباق ماشین حالت، دیاگ و تایمرهای زیمنس
* `misra-zero-defect-c`: استانداردهای ایمنی حافظه، پوینتر و کران حلقه‌ها
* `lvgl-zero-lag-engine`: بهینه‌سازی رندرینگ بدون لگ و مدیریت DMA/PSRAM

---

### ۵. قوانین حیاتی تعامل و ایمنی سخت‌افزار
1. **پاسخ‌گویی فوق‌خلاصه:** ایجنت باید گزارش‌ها را کاملاً خلاصه، تلگرافی و بدون کلمات اضافه ارائه دهد.
2. **درک اصطلاحات فارسی کارفرما:** مطابق با دستورالعمل [AGENT_COMMUNICATION_AND_PERSIAN_INTENT_RULES.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/AGENT_COMMUNICATION_AND_PERSIAN_INTENT_RULES.md).
3. **ممنوعیت مطلق فلش/پروگرم بدون اجازه صریح:** هرگز هیچ دستوری برای آپلود فریمور روی برد فیزیکی (pio run -t upload، esptool، upload.bat) اجرا نکنید مگر اینکه کارفرما مستقیماً نوشته باشد: **«فلش کن»** یا **«پروگرم کن»**.
