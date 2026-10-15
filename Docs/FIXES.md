# لیست اصلاحات مخزن PLC

این فایل مرجع پیگیری اصلاحات مخزن است. هر تغییری که برای استانداردسازی انجام می‌شود در بخش «انجام‌شده» و هر مورد باقی‌مانده در بخش «در انتظار» ثبت شود.

## ✅ اصلاحات انجام‌شده (2026-07-12)

1. **README اصلی مخزن** بازنویسی شد و چهار خانواده محصول، ساختار استاندارد و ریسک‌های GitHub را توضیح می‌دهد.
2. **README هر چهار پروژه اصلاح شد** — متن‌های تکراری و اشتباه که همه را «PLC 5K» معرفی می‌کردند حذف شد و هر پروژه مطابق فایل‌های واقعی خودش توضیح داده شد.
3. **فایل دانش فنی (`Docs/KNOWLEDGE.md`) برای PLC 5K، PLC 8K و PLC 10K ایجاد شد** — قبلاً فقط PLC HMI دانش فنی داشت.
4. **دانش فنی PLC HMI بازنویسی شد** — مسیرهای قدیمی `Soft Ware` و یادداشت‌های نامنطبق با ساختار فعلی به ساختار جدید `Software/...` تبدیل شد.
5. **قوانین عمومی مخزن (`GENERAL_RULES.md`) در ریشه ایجاد شد.**
6. **`.gitignore` ریشه ایجاد شد** — برای جلوگیری از اضافه شدن خروجی‌های build، cache، فایل‌های موقت Altium و فایل‌های سیستم‌عامل.
7. **پوشه‌های خالی مستند شدند** — گیت پوشه خالی را آپلود نمی‌کند؛ در هر پوشه خالی یک `README.md` توضیحی قرار گرفت.

## ⚠️ موارد نیازمند اصلاح / تصمیم (هنوز انجام نشده)

### بحرانی — قبل از push به GitHub باید تعیین تکلیف شود
1. **۷ ریپوی گیت تودرتو (nested `.git`)** — این پوشه‌ها روی GitHub به‌صورت لینک شکسته (submodule بدون آدرس) آپلود می‌شوند و محتوایشان دیده نمی‌شود:
   - `PLC 10K/Software/V.4.4.0/welding_7segment/.git`
   - `PLC 10K/Software/V.4.4.0 TuranTec/welding_7segment/.git`
   - `PLC 5K/Software/V4.6.0/welding_7segment/.git`
   - `PLC 8K/Software/V.2.7.0/V.2.7.0/welding_cleaning/.git`
   - `PLC 8K/Software/V.2.7.0/V.2.7.0 OLD/welding_cleaning/.git`
   - `PLC HMI/Software/Bakery/V.1.1.0/ChoneProject/.git`
   - `PLC HMI/Software/Welder_single_head/V.1.5.0/.git`

   راه‌حل: حذف پوشه `.git` داخلی (تاریخچه داخلی از بین می‌رود) یا تبدیل به submodule واقعی. **حذف نشده چون تصمیم مالک لازم است.**
2. **حجم مخزن (~۱ گیگابایت)** — عمده حجم از کتابخانه‌های vendored مثل `lvgl` و `TFT_eSPI` در `Welder_single_head/V.1.5.0/lib` و عکس‌های کاتالوگ داخل پوشه‌های نرم‌افزار است. پیشنهاد: عکس‌های غیرفنی (`Image/عکس های کاتالوگ...`) به پوشه Docs یا خارج از مخزن منتقل شوند.

### پوشه‌های خالی (فاقد محتوا — یا محتوا اضافه شود یا حذف شوند)
- این پوشه‌ها اکنون README توضیحی دارند، اما هنوز محتوای عملیاتی ندارند و باید تکمیل یا حذف شوند:
  `PLC 5K/Software/libraries`,
  `PLC 8K/Software/libraries`,
  `PLC 10K/Software/libraries`,
  `PLC HMI/Software/Aluminum-punching`,
  `PLC HMI/Software/Copy_router`,
  `PLC HMI/Software/Corner_cleaning`,
  `PLC HMI/Software/Digital_cutter_double_head`,
  `PLC HMI/Software/Digital_cutter_single_head`,
  `PLC HMI/Software/ICE CREEAM`,
  `PLC HMI/Software/Welder_double_head`,
  `PLC HMI/Software/Config_Plus_Test/V.1.0.0/ui`,
  `PLC 10K/Hardware/DIS-V.3.6.4/History`,
  `PLC 5K/Hardware/DIS-V3.8.4/__Previews`.

### ناسازگاری نام‌گذاری
- الگوی نسخه یکدست نیست: `V.4.4.0` (با نقطه بعد از V) در مقابل `V4.6.0` (بدون نقطه). استاندارد پیشنهادی: `V.x.y.z`.
- `PLC 8K/Software/V.2.7.0/V.2.7.0/` — تودرتویی تکراری نسخه؛ یک سطح اضافه است. فایل‌های `V.2.7.0.rar` و `V.2.7.0 OLD.rar` کنار نسخه استخراج‌شده تکراری‌اند (پیشنهاد: حذف rar ها).
- غلط املایی در نام پوشه: `ICE CREEAM` → `Ice_cream`؛ `Program Addres.txt` → `Program Address.txt`؛ `CHANGLOG.md` (داخل کتابخانه DS3231 — متعلق به upstream، دست نزنید).
- نام پوشه‌ها هم فارسی هم انگلیسی است (`عکس های کاتالوگ وب جمشیدی`، `مونتاژ.xlsx`). برای GitHub مشکلی ایجاد نمی‌کند ولی برای یکدستی پیشنهاد می‌شود نام پوشه‌های کد لاتین باشد.
- فاصله در نام پوشه‌ها (`PLC 10K`, `V.2.7.0 OLD`) — کار می‌کند ولی در اسکریپت‌ها و مسیر بیلد دردسرساز است (به‌ویژه `LV_CONF_PATH` — طبق KNOWLEDGE.md خود پروژه HMI).

### نقص مستندات
- برای پروژه‌های نرم‌افزاری داخلی HMI هنوز README مستقل کامل لازم است؛ مخصوصاً `Gas-filling/DemoJamshidi_V0`، `Bakery/V.1.1.0/ChoneProject` و `Welder_single_head/V.1.5.0`.
- پروژه‌های HMI که کد دارند ولی مستند ندارند: `Gas-filling/DemoJamshidi_V0` (فقط `language.txt`)، `Digital_cutter_*` (خالی).
- برای فایل‌های سخت‌افزار (PcbDoc/SchDoc) خروجی PDF شماتیک در مخزن نیست؛ پیشنهاد: خروجی PDF در `Project Outputs` قرار گیرد تا بدون آلتیوم قابل مشاهده باشد.

### موارد امنیتی
- رمزهای پیش‌فرض در مستندات و کد آمده است (WiFi AP: `ESP32-UPLOAD/12345678`، کد اضطراری `112233` در KNOWLEDGE.md). اگر مخزن قرار است **public** شود این موارد باید حذف/تغییر کنند. برای مخزن private مشکلی نیست.

### وضعیت گیت
- فایل‌های حذف‌شده `PLC-HMI` و `main.c` هنوز در ایندکس گیت به‌صورت deleted هستند — در کامیت بعدی حذفشان ثبت شود.
