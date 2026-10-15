# مستندات و راهنمای پروژه HAFFLER Config Launcher & Hardware Test

این پروژه بر اساس استانداردهای سخت‌گیرانه صنعتی زیمنس (Siemens Industrial Rigor) و ظرافت‌های کاربری اپل (Apple HIG Craftsmanship) توسعه و مدیریت می‌یابد.

---

## اسناد فعال و مراجع حقیقت (Sources of Truth)

| سند | کاربرد و محتوا |
|---|---|
| [`AGENTS.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/AGENTS.md) | **قرارداد جامع ایجنت‌ها:** سیاست Zero-Lag 50 FPS، خطوط قرمز ایمنی SSR و رله‌ها، انضباط توکن، سقف استک FreeRTOS و پکیج سرور. |
| [`TOUCHLESS_OPERATION.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/TOUCHLESS_OPERATION.md) | **ناوبری ۱۰۰٪ بدون تاچ:** کاربری کامل لانچر و تستر سخت‌افزار با ۱۰ کلید جلو پنل (F1..F5 و F6..F10). |
| [`HARDWARE_IO_PINOUT.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/HARDWARE_IO_PINOUT.md) | **مرجع جامع پایه‌ها و پریفرال‌ها:** مپ کامل پین‌ها، رله‌های Q0..Q9، سنسورهای MAX6675، افزاینده MCP23017 و پرینتر RS485. |
| [`MULTI_MACHINE_OTA_SPEC.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/MULTI_MACHINE_OTA_SPEC.md) | **موتور نصاب چندماشینه OTA:** پروتکل دانلود، اعتبارسنجی هش SHA-256 و فلش فریمور ماشین‌های برش، جوش، گاز و... |
| [`HWTEST_SUITE_SPEC.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/HWTEST_SUITE_SPEC.md) | **محیط تست جامع سخت‌افزار (رمز ۰۰۰۰):** متدولوژی آزمون رله‌ها، هیترهای SSR، سنسورها، تاچ و چاپ کارنامه QC. |
| [`WEB_PORTAL_SPEC.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/WEB_PORTAL_SPEC.md) | **وب‌اپلیکیشن صنعتی:** هات‌اسپات `192.168.4.1`، پایش تله‌متری زنده و آپلود فریمور از طریق مرورگر. |
| [`DAY_LOCK_LEASING_SPEC.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/DAY_LOCK_LEASING_SPEC.md) | **مدیریت قفل روزشمار و لیزینگ:** الگوریتم چالش/پاسخ آفلاین، تولید مسترکدها و حفاظت از NVS. |
| [`START.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/START.md) | نقطه ورود، مسیریابی سریع تسک‌ها، لیست مهارت‌ها و مدیریت بهینه منابع. |
| [`OPERATIONS.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/OPERATIONS.md) | قوانین پذیرش، خطوط قرمز ایمنی، استانداردهای ساخت/فلش، هرم شواهد و سیاست ۵۰ فریم. |
| [`REFERENCE.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/REFERENCE.md) | مشخصات قطعی سخت‌افزار، معماری فریمور، پین‌های برد HT-V3A و تسک‌های FreeRTOS. |
| [`WORKLOG.md`](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/config-test/docs/WORKLOG.md) | تنها مرجع ثبت قفل‌های فعال، چک‌پوینت وضعیت نشست‌ها، مالکیت وظایف و شواهد راستی‌آزمایی. |

---

## جعبه‌ابزار مهارت‌های تخصصی پروژه (.agents/skills/)

هر تسک پیش از اجرا از طریق مسیریاب `find-skills` به مهارت مربوطه متصل می‌شود:

| نام مهارت | کاربرد و وظیفه |
|---|---|
| `find-skills` | **مسیریاب اصلی:** انتخاب کمترین و دقیق‌ترین مهارت مورد نیاز برای تسک جدید بدون بارگذاری بیهوده سایر مهارت‌ها. |
| `lvgl-design` | طراحی و پیاده‌سازی حرفه‌ای رابط کاربری در LVGL 9 (فونت‌ها، استایل‌ها، RTL فارسی، هندسه صفحه‌نمایش ۴ اینچی ۴۸۰x۳۲۰، سیاست ۵۰ فریم). |
| `complete-hmi-screen` | طراحی یا اصلاح یک سناریوی کامل UI (صفحات لانچر، مدال‌های رمز ۱۲۳ و ۰۰۰۰، گرید تست سخت‌افزار و کارکرد بدون تاچ). |
| `safe-layout-refinement` | پالایش و تراز دقیق المان‌ها (پیکسل، فاصله‌بندی و تایپوگرافی) بدون هیچ‌گونه تغییر در رفتار یا منطق صفحه. |
| `impeccable` | استاندارد نهایی پذیرش بصری و کاربری (تایپوگرافی فارسی، تراز دقیق، هندسه متعادل). |
| `design-taste` | قضاوت و زیباشناسی بصری در چیدمان و هماهنگی المان‌ها. |
| `verify-before-done` | خودارزیابی پیش از اتمام تسک: تطبیق دقیق ادعا با شواهد عینی، عدم نشت تغییرات، بررسی فاصله و زبان. |
| `strict-hmi-firmware-judge` | قضاوت مستقل و سخت‌گیرانه: صدور رأی قطعی (APPROVE / REJECT / BLOCKED) بر اساس شواهد واقعی قبل از تحویل. |
| `super-power` | گردش کار فوق‌سخت‌گیرانه برای بخش‌های حساس و پرخطر (المنت‌های SSR، رله‌ها، فلش چندماشینه، حفاظت NVS). |
| `fast-path` | مسیر سریع برای رفع ایرادات جزیی، اصلاح متن‌ها، خواندن لاگ‌ها یا پاسخ به سوالات ساده. |
| `claude-mem` | مدیریت تداوم نشست‌ها و چک‌پوینت وضعیت در `WORKLOG.md`. |
| `agent-orchestrator` / `task-observer` | برنامه‌ریزی، تفکیک حوزه‌های کاری و نظارت بر دستیاران موازی و پایش سقف توکن. |
| `token-budget` | خواندن محدود (باند شده) فایل‌ها و حفظ بهینگی مصرف منابع. |
| `product-brainstorm` | ایده‌پردازی و توسعه فیچرهای جدید و ارزش‌افزا برای اپراتور و کارخانه. |
| `apple-industrial-hmi-design` | ارزیابی، نقد و خلق صفحات آرام و مدرن LVGL منطبق بر استانداردهای Apple HIG. |
| `apple-lvgl-component-pack` | تولید مستقیم کیپد، سلکتور چندحالته و بنر آلارم به سبک اپل. |
| `siemens-industrial-hardening` | ممیزی کدهای خام، منطق fail-safe زیمنس و مدیریت آلارم ISA-18.2. |
| `misra-zero-defect-c` | رعایت ایمنی خطای صفر درصد در حافظه، پوینترها و حلقه‌ها بر پایه MISRA C:2012. |
| `lvgl-zero-lag-engine` | بهینه‌سازی فنی رندرینگ و تضمین نرخ فریم ۵۰-۶۰ هرتز بدون لگ پردازنده. |

---

## دستورالعمل سریع اجرای تسک‌ها

1. ابتدا `AGENTS.md` و `docs/START.md` و مهارت مربوطه را ارزیابی کنید.
2. فقط بخش مورد نیاز از `REFERENCE.md` یا اسناد اختصاصی را به صورت باند شده (۵۰ الی ۱۵۰ خط) بخوانید.
3. قبل از هر تغییر، بکاپ چرخشی بگیرید: `python tools/backup_manager.py create "<desc>"`.
4. از دستورات باند شده `grep_search` و `view_file` استفاده کرده و تغییرات را به صورت جراحی اعمال نمایید.
5. هرگز بدون دستور مستقیم کارفرما آپلود سخت‌افزاری انجام ندهید؛ پورت مجاز منحصراً CP210x (COM4 یا COM8) است و **هرگز نباید از COM3 استفاده شود**.
