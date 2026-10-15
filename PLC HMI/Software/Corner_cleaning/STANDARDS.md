# استانداردهای اختصاصی دستگاه گوشه‌تمیزکن UPVC (Corner Cleaning HMI)
## راهنمای طراحی Apple HIG و برنامه‌نویسی صنعتی زیمنس

این پروژه برای کنترلر و HMI دستگاه تمیزکن پلیسه و درز جوش پنجره‌های UPVC (سه‌کاره یا چهارکاره مجهز به تیغه‌ها و فرز سطحی) رزرو شده است.

### ۱. پیوند به مستندات بالادستی مخزن:
* [استاندارد طراحی HMI اپل و کتابخانه LVGL](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md)
* [استاندارد برنامه‌نویسی صنعتی و FSM زیمنس](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md)
* [راهنمای جامع معماری سخت‌افزاری مشترک](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md)
* [کتابچه ممیزی، کشف عیوب و صنعتی‌سازی کد](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md)

---

### ۲. قوانین طراحی رابط کاربری (Apple HIG for Corner Cleaner)
* **انتخاب نوع مقطع پروفیل (فریم / لنگه / مولیون):** کلیدهای چندگزینه‌ای مدرن با کنتراست بالا و انحنای لبه کارت‌ها.
* **داشبورد وضعیت تیغه‌های بالا، پایین و بیرونی:** نمایش شماتیک موقعیت سنسورهای خانه (Home) و انتهای کورس تیغه‌ها.

---

### ۳. الزامات ایمنی و برنامه‌نویسی صنعتی زیمنس
* **ترتیب دقیق ماشین وضعیت تمیزکاری:**
  1. کلمپ چهارجهته گوشه پنجره.
  2. حرکت چاقوهای تراش سطحی بالا و پایین.
  3. حرکت فرز تراش گوشه خارجی.
  4. برگشت کامل ابزارها به موقعیت هوم.
  5. رهاسازی کلمپ‌ها.
* **تایم‌اوت سخت‌گیرانه برای هر ابزار:** اگر براده UPVC مانع برگشت تیغه شود، سیستم متوقف و آلارم گیر مکانیکی صادر می‌شود.

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
