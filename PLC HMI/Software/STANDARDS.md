# استانداردهای کلان مهندسی نرم‌افزار و طراحی صنعتی هافلر (Master Haffler Engineering Standards)
## نظام‌نامه جامع طراحی HMI اپل، برنامه‌نویسی زیمنس، ایمنی MISRA C، فیلدباس صنعتی و خطای صفر درصد

این سند نقطه دسترسی، دایره‌المعارف و فهرست‌نامه جامع کلیه استانداردهای حاکم بر مخزن نرم‌افزارهای صنعتی هافلر در مسیر `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software` است.

---

## ۱. فهرست مستندات بنیادین مخزن

| مستند مرجع | موضوع و دامنه کاربرد | پیوند مستقیم |
|:---|:---|:---:|
| **نظام‌نامه ارتباط و ممنوعیت فلش** | گزارش‌دهی فوق‌خلاصه، رمزگشایی اصطلاحات کارفرما و ممنوعیت قطعی پروگرم بدون اجازه | [AGENT_COMMUNICATION_AND_PERSIAN_INTENT_RULES.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/AGENT_COMMUNICATION_AND_PERSIAN_INTENT_RULES.md) |
| **استاندارد طراحی Apple HIG** | طراحی چشم‌نواز، تایپوگرافی، پالت رنگ دارک، شبکه ۸ پیکسلی، ارگونومی لمسی و نقد زیبایی | [STANDARDS_APPLE_HIG_HMI.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md) |
| **کتابخانه ویجت‌های LVGL اپل** | کدهای آماده کیپد عددی، سلکتور چندحالته، بنر آلارم و کارت‌های تنظیمات | [APPLE_HIG_LVGL_COMPONENT_LIBRARY.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/APPLE_HIG_LVGL_COMPONENT_LIBRARY.md) |
| **استاندارد برنامه‌نویسی زیمنس** | معماری تعینی، FSM بر پایه PackML، برنامه‌نویسی تدافعی، سیستم آلارم ISA-18.2 و تفکیک FreeRTOS | [STANDARDS_SIEMENS_INDUSTRIAL_PLC.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md) |
| **قوانین ایمنی MISRA C** | انواع صریح داده، ایمنی اشاره‌گرها، حلقه‌های کران‌دار، فلگ‌های کامپایلر و خطای صفر درصد | [MISRA_C_INDUSTRIAL_SAFETY_RULES.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/MISRA_C_INDUSTRIAL_SAFETY_RULES.md) |
| **تاب‌آوری و بازیابی از کرش** | ثبت لاگ پانیک در RTC/NVS، صفحه ریکاوری اپراتور، رول‌بک هوشمند OTA و کنترل افت ولتاژ | [CRASH_RESILIENCE_AND_RECOVERY_GUIDE.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CRASH_RESILIENCE_AND_RECOVERY_GUIDE.md) |
| **مخابرات صنعتی و فیلدباس** | پروتکل Modbus RTU/TCP، زمان‌بندی خط RS485، کنترل DE/RE و تله‌متری وب‌سوکت | [INDUSTRIAL_COMMUNICATIONS_AND_FIELDBUS.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/INDUSTRIAL_COMMUNICATIONS_AND_FIELDBUS.md) |
| **دایره‌المعارف ۱۱ ماشین هافلر** | تحلیل فنی، معادلات انکودر، لقی دنده، PID دما، فشار بار، و دینامیک گاز برای ۱۱ ماشین | [MACHINE_DEEP_DIVE_ENCYCLOPEDIA.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/MACHINE_DEEP_DIVE_ENCYCLOPEDIA.md) |
| **راهنمای معماری سخت‌افزار** | مشخصات ESP32-S3، ال‌سی‌دی ILI9488، تاچ GT911، ماژول MAX6675، رله‌ها و نویز صنعتی | [HARDWARE_ARCHITECTURE_GUIDE.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HARDWARE_ARCHITECTURE_GUIDE.md) |
| **کتابچه ممیزی و صنعتی‌سازی** | گیت‌های ۷گانه ارزیابی کد، کشف عیوب آماتور و الگوهای ریفکتورینگ به کدهای فوق‌العاده مقاوم | [CODE_AUDIT_AND_HARDENING_PLAYBOOK.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CODE_AUDIT_AND_HARDENING_PLAYBOOK.md) |
| **استاندارد بسته OTA و لاگ تجمعی** | استاندارد باینری سرور، فایل lastfile، حذف بازر، پایه‌های M، فونت‌های بدون مربع و چنج‌لاگ رول‌آپ | [STANDARDS_HAFFLER_OTA_RELEASE_AND_ROLLUP_CHANGELOG.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_HAFFLER_OTA_RELEASE_AND_ROLLUP_CHANGELOG.md) |
| **استقرار سرور ابری و اعتبارسنجی OTA** | آدرس‌های سرور آروان‌کلاد، نگاشت ۱۲ ماشین، اعتبارسنجی سلامت لینک‌ها و ابزار خودکار | [STANDARDS_HAFFLER_OTA_SERVER_DEPLOYMENT.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_HAFFLER_OTA_SERVER_DEPLOYMENT.md) |
| **کتابچه جامع مهندسی مشترک ۱۱ ماشین** | اصول آزمون، معماری دو هسته‌ای، ارتقای دوگانه، ارگونومی کلیدها، رندرینگ ضد لگ و آلارم NVS | [CROSS_MACHINE_ENGINEERING_PLAYBOOK.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CROSS_MACHINE_ENGINEERING_PLAYBOOK.md) |
| **کنترل و مانیتورینگ ابری و اینترنت اشیا** | معماری پروتکل MQTT، گیت اتصال واقعی، تله‌متری سنسورها و ساختار فرامین اجرایی | [HAFFLER_CLOUD_CONTROL_AND_MONITORING_SPEC.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/HAFFLER_CLOUD_CONTROL_AND_MONITORING_SPEC.md) |
| **دستورالعمل جامع ایجنت‌ها** | قرارداد کلان رفتاری، خطوط قرمز و مأموریت ایجنت‌های هوش مصنوعی فعال روی پروژه‌ها | [AGENTS.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/AGENTS.md) |
| **میکرو-کوپایلوت ۲۰ کلید** | برون‌سپاری ریزتسک‌های کدنویسی زیر ۱ ثانیه با مصرف صفر توکن جلسه اصلی | [haffler_micro_copilot.py](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_micro_copilot.py) |
| **رادار تبادل دانش میان تمام پروژه‌ها** | جستجو و بازاستفاده آنی از کدهای تست‌شده در ۲۰ پروژه فدرال هافلر | [haffler_cross_radar.py](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_cross_radar.py) |
| **ماتریس تست ۴ لایه و بنچمارک رقبا** | آزمون ایمنی MISRA C، ارگونومی اپل، FSM زیمنس و مقایسه با استانداردهای آلمان | [haffler_test_matrix.py](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_test_matrix.py) |
| **ناظم بهداشت و پاکسازی مخزن** | تضمین مخزن تمیز، بدون فایل‌های موقت، زباله و منطبق با Supreme Mandate 0 | [haffler_repo_janitor.py](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_repo_janitor.py) |
| **کتابخانه ماژولار هسته صنعتی** | درایورهای ضدلگ رله‌ها، ضد فریز تاچ، تله‌متری OEE و ماشین حالت یکپارچه | [haffler_core](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/libraries/haffler_core) |
| **مرکز فرماندهی ناوگان** | مدیریت یکپارچه ۱۲ ماشین، خودترمیمی سیستم (Doctor) و لیدربورد امتیازات | [haffler_master.py](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_master.py) |
| **گارد تشخیص تداخل پین‌اوت** | کشف تداخل سخت‌افزاری پایه‌های ESP32-S3، پین‌های فلش و استرپینگ قبل از کامپایل | [haffler_pin_guard.py](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_pin_guard.py) |
| **داشبورد گرافیکی وضعیت ناوگان** | جدول بصری لایت و مینیمال وضعیت سلامت، گره‌های AST و بلوغ هر ۱۲ ماشین | [FLEET_STATUS_REPORT.html](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/FLEET_STATUS_REPORT.html) |



---

## ۲. سه خط قرمز تعاملی و عملیاتی
1. **لحن فوق‌خلاصه:** پاسخ‌ها تلگرافی، بولت‌پوینت، بدون مقدمه و با درصد پیشرفت (%).
2. **رمزگشایی اصطلاحات فارسی کارفرما:** درک دقیق اصطلاحات کارگاهی بدون حدس خطرناک.
3. **ممنوعیت مطلق پروگرم:** تا زمانی که کاربر صریحاً ننویسد «فلش کن» یا «پروگرم کن»، هیچ کدی روی سخت‌افزار آپلود نمی‌شود.
