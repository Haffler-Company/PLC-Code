# استانداردهای اختصاصی دستگاه جوش تک‌سر (Welding Single Head)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه فریمور دستگاه جوش تک‌سر در و پنجره UPVC با کنترل دمای فک حرارتی و چرخه پنوماتیک ذوب و فشرده‌سازی است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Welder HMI)
* **قهرمان صفحه (Hero Readout):** دمای لحظه‌ای فک جوش (سنسور MAX6675) و تایمر زمان ذوب/جوش به عنوان بزرگترین شاخص‌ها نمایش داده شوند.
* **کد رنگ وضعیت حرارتی:** 
  - دمای زیر ۲۰۰°C (در حال گرمایش): زرد/نارنجی ملایم (`#FF9F0A`).
  - دمای نرمال کاری (۲۴۰°C - ۲۶۰°C): سبز پایدار (`#30D158`).
  - دمای بالاتر از ۲۷۰°C (اضافه دما): قرمز خطرناک (`#FF453A`).
* **شمارنده تولید:** نمایش شمارنده روزانه شیفت داخل یک کارت کپسولی مدرن در هدر صفحه.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **حفاظت قطع سنسور حرارتی (Open Thermocouple Detection):** در صورت قطع فیش یا خرابی چیپ MAX6675، رله کنترل المنت باید در کمتر از ۵۰ میلی‌ثانیه قطع شده و آلارم صادر شود.
* **ماشین وضعیت ۵ مرحله‌ای استاندارد جوش:**
  1. `CLAMP_PROFILE`: مهار پروفیل با جک‌های افقی و عمودی.
  2. `HEATER_ENTER`: ورود اتوی داغ به بین دو سر پروفیل.
  3. `MELT_DWELL`: زمان ذوب پروفیل تحت فشار ملایم.
  4. `HEATER_EXIT`: خروج سریع اتو.
  5. `JOIN_COOL`: فشرده‌سازی نهایی و خنک‌کاری جوش.
* تمامی مراحل ملزم به داشتن تایم‌اوت سخت‌افزاری مستقل هستند.

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
