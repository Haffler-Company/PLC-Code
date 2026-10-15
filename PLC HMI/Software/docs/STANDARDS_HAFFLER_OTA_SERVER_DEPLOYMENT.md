# نظام‌نامه جامع استقرار ابری فریم‌ورها، مسیرهای سرور OTA و اعتبارسنجی آنلاین لینک‌ها
## استاندارد مرجع مهندسی انتشار و به‌روزرسانی از راه دور ماشین‌آلات هافلر (Master Standard)

> **دامنه شمول:** کلیه پروژه‌ها، فرمورها، وب‌اپلیکیشن‌ها، توسعه‌دهندگان و ایجنت‌های هوش مصنوعی فعال در مسیر `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software` بدون هیچ‌گونه استثنا.

---

## ۱. اهداف و معماری زیرساخت ابری (Cloud Infrastructure)

سامانه به‌روزرسانی آنلاین (OTA) ماشین‌آلات صنعتی شرکت هافلر، امکان ارتقای فریم‌ور، اعمال اصلاحات نرم‌افزاری و همگام‌سازی قابلیت‌های جدید را به صورت بی‌سیم و از راه دور فراهم می‌سازد.

* **سرویس‌دهنده ابری:** فضای ابری آروان‌کلاد (Object Storage سازگار با استاندارد جهانی S3)
* **منطقه عملیاتی (Region):** `ir-thr-at1` (ایران - تهران)
* **صندوقچه مرجع (Bucket):** `ota-plc`
* **آدرس اندپوینت مستقیم S3:** `https://s3.ir-thr-at1.arvanstorage.ir`
* **دامنه CDN عمومی دستگاه‌ها:** `https://ota.haffler-infi.ir`
* **نوع دسترسی فایل‌ها (ACL):** الزماً `public-read` (دستگاه‌های صنعتی مانند ESP32 بدون نیاز به کلید یا لاگین باید فایل را مستقیماً دانلود نمایند).
* **نوع محتوا (Content-Type):** الزماً `application/octet-stream` برای فایل‌های باینری و متنی.

---

## ۲. اقلام الزامی هر بسته انتشار (Release Artifacts)

پس از کامپایل موفق و تست سورس‌کد، هر ریلیز منحصراً از ۲ فایل تفکیک‌ناپذیر تشکیل می‌شود که باید در مسیر اختصاصی دستگاه روی سرور قرار گیرند:

1. **`firmware.bin`:** باینری خام فریم‌ور کامپایل‌شده اپلیکیشن (`app0` در افست `0x10000`).
2. **`lastfile`:** فایل متادیتای تک‌منبعی (Single Source of Truth) شامل اطلاعات شرکت، شماره نسخه، سایز بایت، هش SHA-256، چنج‌لاگ سه‌زبانه، نقشه پایه‌ها و دستورات فلش.

> ⚠️ **خط قرمز مهندسی و ممنوعیت دسکتاپ:**
> ۱. هیچ فریم‌وری نباید بدون فایل `lastfile` معتبر و هماهنگ روی سرور آپلود شود؛ کلاینت HMI دستگاه ابتدا `lastfile` را خوانده، نسخه و هش را مقایسه می‌کند و تنها در صورت تطابق قطعی باینری را دانلود می‌نماید.
> ۲. **ممنوعیت قطعی دسکتاپ:** هیچ فایلی نباید روی دسکتاپ کاربر (`C:\Users\Haffler\Desktop`) قرار داده شود. فایل‌های انتشار منحصراً درون پوشه `release/` هر پروژه ایجاد شده و مستقیماً به سرور ابری آروان‌کلاد منتقل می‌شوند.

---

## ۳. جدول جامع نگاشت پروژه‌ها به پوشه‌های سرور و لینک‌های OTA

| ردیف | نام و مدل ماشین | پوشه محلی سورس | مسیر ذخیره‌سازی ابری (`ota-plc`) | آدرس دانلود فراداده (`lastfile`) | آدرس دانلود باینری (`firmware.bin`) |
|:---:|:---|:---|:---|:---|:---|
| ۱ | **جوش تک‌سر UPVC** | `Welding_single_head` | `upvc/welding-single-head/` | `https://ota.haffler-infi.ir/upvc/welding-single-head/lastfile` | `https://ota.haffler-infi.ir/upvc/welding-single-head/firmware.bin` |
| ۲ | **برش دیجیتال تک‌سر** | `Cutter_single_head(Digital_metr)` | `upvc/digital-cutter_single_head/` | `https://ota.haffler-infi.ir/upvc/digital-cutter_single_head/lastfile` | `https://ota.haffler-infi.ir/upvc/digital-cutter_single_head/firmware.bin` |
| ۳ | **تزریق گاز اسپیسر** | `Gas-filling` | `upvc/gas-filling/` | `https://ota.haffler-infi.ir/upvc/gas-filling/lastfile` | `https://ota.haffler-infi.ir/upvc/gas-filling/firmware.bin` |
| ۴ | **جوش دو‌سر UPVC** | `Welding_double_head` | `upvc/welding-double-head/` | `https://ota.haffler-infi.ir/upvc/welding-double-head/lastfile` | `https://ota.haffler-infi.ir/upvc/welding-double-head/firmware.bin` |
| ۵ | **برش دیجیتال دو‌سر** | `Cutter double_head(Digital_metr)` | `upvc/digital-cutter double_head/` | `https://ota.haffler-infi.ir/upvc/digital-cutter double_head/lastfile` | `https://ota.haffler-infi.ir/upvc/digital-cutter double_head/firmware.bin` |
| ۶ | **تمیزکننده گوشه UPVC** | `Corner_cleaning` | `upvc/corner-cleaning/` | `https://ota.haffler-infi.ir/upvc/corner-cleaning/lastfile` | `https://ota.haffler-infi.ir/upvc/corner-cleaning/firmware.bin` |
| ۷ | **کپی روتور UPVC** | `Copy_router` | `upvc/copy-router/` | `https://ota.haffler-infi.ir/upvc/copy-router/lastfile` | `https://ota.haffler-infi.ir/upvc/copy-router/firmware.bin` |
| ۸ | **پانچ آلومینیوم** | `Aluminum-punching` | `upvc/aluminum-punching/` | `https://ota.haffler-infi.ir/upvc/aluminum-punching/lastfile` | `https://ota.haffler-infi.ir/upvc/aluminum-punching/firmware.bin` |
| ۹ | **تست کانفیگ مشترک** | `config-test` | `upvc/config-test/` | `https://ota.haffler-infi.ir/upvc/config-test/lastfile` | `https://ota.haffler-infi.ir/upvc/config-test/firmware.bin` |
| ۱۰ | **چانه‌گیر نانوایی** | `Bakery` | `other/bakery-changir/` | `https://ota.haffler-infi.ir/other/bakery-changir/lastfile` | `https://ota.haffler-infi.ir/other/bakery-changir/firmware.bin` |
| ۱۱ | **میکسر قنادی/نانوایی** | `Bakery` | `other/bakery-mixer/` | `https://ota.haffler-infi.ir/other/bakery-mixer/lastfile` | `https://ota.haffler-infi.ir/other/bakery-mixer/firmware.bin` |
| ۱۲ | **دستگاه بستنی‌ساز** | `ICE CREEAM` | `other/ice-cream/` | `https://ota.haffler-infi.ir/other/ice-cream/lastfile` | `https://ota.haffler-infi.ir/other/ice-cream/firmware.bin` |

---

## ۴. ساختار الزامی فایل متادیتا (`lastfile`)

فایل `lastfile` باید به فرمت متن ساده UTF-8 بدون کاراکتر BOM بوده و منحصراً دارای بخش‌های هشت‌گانه زیر باشد:
1. `[company]`: نام شرکت، برند، وب‌سایت و ایمیل پشتیبانی هافلر.
2. `[program]`: نام تجاری پروژه، کلید پروژه روی سرور (`project=...`)، پلتفرم سخت‌افزاری و زبان‌های فعال.
3. `[version]`: شماره نسخه دقیق عددی (`code=X.Y.Z`)، فرمت نمایش (`display=VX.Y.Z`)، تاریخ بیلد و ساعت بیلد.
4. `[binary]`: نام فایل باینری (`file=firmware.bin`)، حجم به بایت (`size_bytes`)، هش SHA-256 (حروف بزرگ)، وضعیت ساین و سازگاری حافظه فلش.
5. `[description_fa]`: لاگ تجمعی تغییرات به زبان فارسی (حداکثر ۱۲ سطر با الگوی `lineN=VX.Y.Z | YYYY-MM-DD | توضیح`).
6. `[description_en]`: لاگ تجمعی تغییرات به زبان انگلیسی.
7. `[description_tr]`: لاگ تجمعی تغییرات به زبان ترکی استانبولی (پالایش‌شده و بدون کاراکترهای مربعی).
8. `[inputs]` و `[outputs]`: نقشه اتصالات با حذف قطعی بازر (`BZ`) و درج الزامی پایه‌های کمکی دوطرفه `M0..M7`.

---

## ۵. چک‌لیست ۴ مرحله‌ای اعتبارسنجی سلامت سرور (Verification Protocol)

هر زمان فایلی در سرور قرار گرفت، توسعه‌دهنده یا ایجنت موظف است ۴ آزمون زیر را اجرا و تایید کند:

1. **آزمون دسترسی آنلاین (HTTP 200 Check):**
   درخواست `HEAD` یا `GET` به هر دو آدرس `lastfile` و `firmware.bin` باید کد وضعیت `200 OK` برگرداند.
2. **آزمون یکپارچگی حجم (Content-Length Match):**
   مقدار `Content-Length` فایل باینری روی سرور باید بایت‌به‌بایت با مقدار کلید `size_bytes` در فایل `lastfile` برابر باشد.
3. **آزمون تطابق هش (SHA-256 Verification):**
   هش SHA-256 فایل باینری روی سرور باید دقیقاً با کلید `sha256` در `lastfile` همخوانی داشته باشد.
4. **آزمون کلاینت سخت‌افزار (User-Agent Simulation):**
   دانلود با هدر `User-Agent: ESP32-HTTP-Client` نباید توسط CDN مسدود، ریدایرکت یا کپچا شود.

---

## ۶. دستورات اجرایی ابزار انتشار و اعتبارسنجی (`tools/haffler_ota_publisher.py`)

برای راحتی کار، ابزار متمرکز هافلر کلیه این مراحل را به صورت خودکار انجام می‌دهد:

* **بررسی سلامت تمامی لینک‌های ماشین‌های سرور:**
  ```bash
  python tools/haffler_ota_publisher.py --verify-all
  ```
* **بررسی سلامت لینک یک پروژه خاص:**
  ```bash
  python tools/haffler_ota_publisher.py --verify Welding_single_head
  ```
* **تولید خودکار پکیج، آپلود و تست آنلاین یک پروژه:**
  ```bash
  python tools/haffler_ota_publisher.py --publish Welding_single_head
  ```

---

## ۷. هفت گیت ایمنی حیاتی و پیشگیری از خطرات صنعتی (Pre-Flight Safety Gates)

پیش از ارسال هر باینری به سرور، رعایت موارد زیر برای پیشگیری از آسیب به ماشین‌آلات الزامی است:

1. **ممنوعیت مطلق انتشار سرخود (Human Confirmation Mandate):**
   هیچ ایجنتی نباید به محض اتمام ادیت کد، خودکار فایل را روی سرور عمومی قرار دهد؛ انتشار سرور به منزله استقرار آنی در کارخانه‌هاست و باید منحصراً با تایید یا درخواست صریح کارفرما («بفرست رو سرور» یا «آپلود کن») انجام گیرد.
2. **افزایش اجباری شماره پچ نسخه (Version Bump):**
   اگر کد تغییر کند اما شماره نسخه ارتقا نیابد، دستگاه‌ها آپدیت را دانلود نخواهند کرد.
3. **تطابق هش و سایز قبل از ارسال (Local Hash Guard):**
   سایز و هش SHA-256 فایل باینری باید پیش از ارسال دقیقاً با متادیتای `lastfile` تطابق داشته باشد (ابزار `haffler_ota_publisher.py` به صورت خودکار مانع آپلود فایل‌های ناهماهنگ می‌شود).
4. **ترتیب آپلود اتمیک (Atomic Sequence):**
   ابتدا فایل باینری (`firmware.bin`) و پس از اتمام آن، فایل متادیتا (`lastfile`) آپلود می‌شود تا دستگاه‌ها با باینری ناقص مواجه نشوند.
5. **پشتیبان‌گیری خودکار از نسخه قبلی (Rollback Insurance):**
   قبل از جایگزینی هر فایل روی سرور، نسخه قبلی در `backups/ota_server_previous/` ذخیره می‌شود تا در صورت بروز نقص، امکان بازگشت فوری وجود داشته باشد.
6. **ایمنی قطعی رله‌ها و پولاریتی سخت‌افزار (Active-LOW Verification):**
   خروجی رله‌ها در تمام کنترلرهای هافلر Active-LOW هستند؛ هرگونه تغییر ناخواسته در پولاریتی می‌تواند باعث تحریک ناگهانی جک‌ها و المنت‌ها در کارخانه شود.
7. **حفاظت از حافظه پایدار NVS:**
   پارتیشن‌بندی و متغیرهای ذخیره‌شده نباید کالیبراسیون انکودر، میلی‌متر و رمزهای عبور دستگاه را پس از به‌روزرسانی پاک کنند.

---

## ۸. استاندارد موتور فلش و استریم فوق‌سریع کلاینت (High-Speed Client Engine)

برای پیشگیری قطعی از لگ ۴ ثانیه‌ای، دانلود دوگانه، و سکته‌های متوالی ناشی از قفل سخت‌افزاری SPI فلش در حین دریافت TCP، کلیه فریم‌ورها باید از معماری ۵ مرحله‌ای موتور استریم هافلر پیروی کنند:
1. **پاک‌سازی پیش‌گسترانه پس‌زمینه (Early Pre-Erase):** راه‌اندازی در ثانیه ۵ پس از بوت و رویداد `IP_EVENT_STA_GOT_IP`.
2. **تضمین پاک بودن کامل پیش از اتصال به سوکت (Clean Flash Guarantee):** ممنوعیت مطلق پاک‌سازی فلش در حین باز بودن سوکت HTTP.
3. **غیرفعال‌سازی اریز خودکار فریم‌ورک:** مقداردهی `esp_ota_begin(part, 4096, &h)`.
4. **بهینه‌سازی انتقال:** `WIFI_PS_NONE`، `TCP_NODELAY`، بافر سوکت ۳۲KB و تجمیع داده‌ها در بلوک‌های ۸KB پیش از رایت.
5. **وب‌پورتال تک‌پروازی:** ارسال آنی پاسخ `200 OK`، قفل `s_ota_is_fetching` و هدرهای ضد کش.

📖 **مستند تفصیلی و کدهای آماده:** به سند جامع [HAFFLER_HIGH_SPEED_OTA_ENGINE_BLUEPRINT.md](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HAFFLER_HIGH_SPEED_OTA_ENGINE_BLUEPRINT.md) مراجعه فرمایید.
