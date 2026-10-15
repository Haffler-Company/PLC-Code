# استانداردهای اختصاصی دستگاه اره دوسر و متر دیجیتال همگام (Cutter Double Head)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه برای فریمور دستگاه اره دوسر برش همزمان پروفیل‌های آلومینیوم و UPVC با متر دیجیتال و دو سر برش زاویه‌دار رزرو شده است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Double Cutter)
* **نمایشگر موقعیت دو سر اره:** سر ثابت (Fixed Head) و سر متحرک (Movable Head) در دو کارت قرینه با ارقام بزرگ نمایش داده شوند.
* **انتخاب زاویه برش (۴۵° / ۹۰° / ۱۳۵°):** کلیدهای چندحالته کپسولی مدرن شبیه Segmented Controls در iOS.
* **لیست برش هوشمند:** نمایش لیست ۵۰ تایی برش‌ها با اسکرول روان و هایلایت ردیف جاری به رنگ آبی سیستم اپل.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **اینترلاک همزمان دو تیغه:** حرکت همزمان دو تیغه مستلزم تایید دو دست اپراتور (Two-Hand Control Interlock) جهت جلوگیری از قطع عضو.
* **محافظت در برابر برخورد دو سر اره:** فاصله نرم‌افزاری و میکروسوئیچ سخت‌افزاری حداقل مجاز برای جلوگیری از برخورد فیزیکی دو کالسکه.

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
