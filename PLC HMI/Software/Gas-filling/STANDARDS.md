# استانداردهای اختصاصی دستگاه تزریق گاز آرگون (Gas Filling HMI)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه مربوط به کنترلر و نمایشگر دستگاه تزریق اتوماتیک گاز آرگون در شیشه‌های دوجداره صنعتی است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Gas Filling HMI)
* **قهرمان صفحه (Hero Readout):** درصد خلوص گاز آرگون تزریق‌شده (مثلاً ۹۰٪ یا ۹۵٪) و فشار سنسور گاز به عنوان عدد غول‌پیکر در وسط صفحه با فونت‌های سری `digit`.
* **نمایشگر گرافیکی پیشرفت پروسه (Process Progress Ring / Bar):** نوار پر شدن گاز با رنگ سبز نئونی مات سیستم اپل (`#30D158`) با به‌روزرسانی روان بر مبنای Dirty-Flag بدون لرزش تصویر.
* **کارت‌های ورودی و خروجی گاز:** نمایش مقادیر فلومتر و وضعیت ولوهای ورودی/خروجی در دو کارت مستقل متقارن.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **حفاظت اضافه فشار (Overpressure Safety):** اگر فشار داخل اسپیسر شیشه از حد مجاز فراتر رود، شیر تخلیه فوری فعال و تزریق گاز بلافاصله قطع شود تا شیشه منفجر نشود.
* **تایم‌اوت عدم خلوص (Purity Timeout Guard):** اگر بعد از زمان مجاز (مثلاً ۶۰ ثانیه) درصد گاز به حداقل تعیین‌شده نرسید، اعلام خطای نشتی درزبندی (Leakage Error) و توقف فرآیند.
* **قطع ایمن شیرهای برقی:** شیرهای برقی تزریق گاز به صورت پیش‌فرض بسته (Normally Closed) بوده و رله‌ها در حالت ریست سیستم کاملاً بی‌برق بمانند.

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
