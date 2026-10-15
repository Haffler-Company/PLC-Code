# استانداردهای اختصاصی دستگاه جوش دوسر UPVC (Welding Double Head)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه برای فریمور دستگاه جوش دوسر همزمان کلاف در و پنجره UPVC با کنترل مستقل دو فک حرارتی و زاویه ۹۰ درجه رزرو شده است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [نظام‌نامه بسته انتشار OTA، نقشه پایه‌ها و لاگ تجمعی](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_HAFFLER_OTA_RELEASE_AND_ROLLUP_CHANGELOG.md)
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Double Welder)
* **کنترل دوگانه دمای هیترها:** نمایش دمای فک چپ و فک راست در دو کارت مستقل با گیج‌های دمایی مدرن.
* **داشبورد گرافیکی وضعیت جک‌ها:** نمایش نمادین زاویه جوش و استیت جک‌های کلمپ و اتو با نمادهای وکتور ساده و بدون شلوغی.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **همگام‌سازی زمان ذوب (Melt Synchronization):** دو سر پروفیل باید در کسری از ثانیه همزمان به صفحه داغ تماس پیدا کرده و جدا شوند تا کیفیت جوش یکنواخت حاصل شود.
* **کنترل دوگانه ترموکوپل‌های MAX6675:** پایش مستقل دو کانال ترموکوپل و قطع اضطراری هر دو در صورت بروز اضافه دما در یکی از فک‌ها.

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
4. **استاندارد پکیج سرور و چنج‌لاگ تجمعی بدون کاراکتر مربعی:** تولید خودکار پکیج کامل سرور (`firmware.bin`، `spiffs.bin`، `bootloader.bin`، `partitions.bin`، `pinmap.txt`، `lastfile`) با هش SHA-256، حذف قطعی بازر، پشتیبانی پایه‌های M و چنج‌لاگ تجمعی بدون کاراکتر مربعی مطابق [نظام‌نامه بسته انتشار و لاگ تجمعی](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_HAFFLER_OTA_RELEASE_AND_ROLLUP_CHANGELOG.md).
