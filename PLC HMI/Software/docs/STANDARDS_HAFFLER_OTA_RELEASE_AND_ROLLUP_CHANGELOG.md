# نظام‌نامه جامع تولید بسته انتشار OTA، فراداده `lastfile`، نقشه اتصالات صنعتی و لاگ تجمعی (Rollup Changelog)
## استاندارد مرجع الزام‌آور مهندسی برای کلیه ماشین‌آلات و پروژه‌های هافلر (Master Standard)

> **دامنه شمول:** کلیه پروژه‌ها، فرمورها، وب‌اپلیکیشن‌ها و ایجنت‌های توسعه‌دهنده در مسیر `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software` بدون هیچ‌گونه استثنا.

---

## ۱. اهداف و فلسفه طراحی (Core Engineering Philosophy)

به‌روزرسانی نرم‌افزار ماشین‌آلات صنعتی هافلر باید از همان استانداردها، ظرافت و تجربه کاربری حاکم بر سیستم‌عامل‌های مدرن تلفن‌های هوشمند (Apple iOS و Google Android OTA) پیروی کند:
1. **شفافیت حداکثری و زنجیره ارتقای تجمعی (Rollup Changelog):** کاربر باید در یک نگاه بداند دقیقاً از نسخه‌ای که روی دستگاهش نصب است تا نسخه جدید ارائه‌شده روی سرور، چه قابلیت‌ها، بهینه‌سازی‌ها و اصلاحاتی اضافه شده است (نه صرفاً خلاصه آخرین پچ).
2. **پیشگیری قطعی و ۱۰۰٪ از کاراکترهای مربعی (Zero Tofu Guarantee):** تحت هیچ شرایطی نباید کاراکترهای ناشناخته، گلیف‌های خاص یا کاراکترهای زبان ترکی به صورت مربع (`[]`) روی نمایشگر صنعتی نقش ببندند.
3. **تولید خودکار و یکپارچه بسته ریلیز (Zero Manual Step):** هر بیلد موفق کامپایلر باید به صورت خودکار کلیه فایل‌های موردنیاز سرور و فلش کارخانه‌ای را تولید و اعتبارسنجی کند.
4. **انتزاع تمیز سخت‌افزاری:** حذف قطعی بازر (`BZ`) از نقشه اتصالات اپراتوری و استانداردسازی پایه‌های دوطرفه کمکی `M0..M7`.

---

## ۲. اصل ممنوعیت مطلق دسکتاپ و استقرار مستقیم ابری (Zero-Desktop & Direct Cloud Deployment Mandate)

> 🚨 **قانون الزام‌آور و خط قرمز قطعی:** قرار دادن هرگونه فایل، سورس‌کد، باینری یا اطلاعات روی دسکتاپ (`C:\Users\Haffler\Desktop`) اکیداً و مطلقاً ممنوع است. دسکتاپ کارفرما باید همواره ۱۰۰٪ پاکیزه بماند.

هنگامی که کارفرما درخواست «فایل برای سرور» دارد («فایل برای سرور بده»، «پکیج سرور بده»):
تنها و فقط ۲ فایل درون پوشه داخلی `release/` هر پروژه تولید و نگهداری شده و مستقیماً با ابزار `tools/haffler_ota_publisher.py` روی سرور ابری آروان‌کلاد (باکت `ota-plc`) مستقر می‌شوند:
1. `release/lastfile`: تک‌منبع رسمی متادیتای انتشار، نقشه پایه‌ها، اطلاعات شرکت و چنج‌لاگ سه‌زبانه.
2. `release/firmware.bin`: فایل اصلی باینری فریمور کامپایل‌شده (app0).

درون مخزن در پوشه `release/` هر پروژه این فایل‌ها به انضمام فایل‌های ریکاوری کارخانه‌ای نگهداری می‌شوند:
- `firmware.bin` (app0)
- `lastfile` (فراداده سرور)
- `spiffs.bin` (فایل‌سیستم وب در صورت وجود)
- `bootloader.bin` و `partitions.bin` (فلش آفلاین)

---

## ۳. ساختار الزامی و استاندارد فایل فراداده (`lastfile`)

فایل `lastfile` تنها منبع حقیقت (Single Source of Truth) برای سرور بروزرسانی و کلاینت HMI دستگاه است. این فایل منحصراً شامل ۸ بخش زیر است (همین‌ها و نه هیچ فایل یا بخش دیگر):

```ini
# lastfile -- Haffler firmware release descriptor (LASTFILE STANDARD)
# Single source of truth for Haffler server release metadata.

[company]
name=Haffler Industrial Automation
brand=Haffler
website=www.haffler.ir
support=support@haffler.ir

[version]
code=1.8.58
display=V1.8.58
build_date=2026-09-08
build_time=18:59

[binary]
file=firmware.bin
size_bytes=2054256
sha256=BA30456763CF29A3362970B135AD651A4243E3077B6FE0A30F936D205A4CB41F
signed=no
partition=16MB flash, app0 ota_0 at 0x10000 (7MB), NVS at 0x9000 preserved
flash_compatible=esp32-s3-devkitc-1 (qio/qio_qspi)

[description_fa]
line1=V1.5.729 | 2026-09-08 | رفع انحراف مختصات رمز و ارتقای پایداری رندر
line2=V1.5.728 | 2026-09-08 | یکپارچه‌سازی متون سه زبانه و پشتیبانی پایه‌های M
line3=V1.5.727 | 2026-09-08 | بزرگ‌سازی کارت تایید لینک بروزرسانی
line4=V1.5.720 | 2026-09-07 | بهینه‌سازی رندر و روان‌سازی انیمیشن‌ها

[description_en]
line1=V1.5.729 | 2026-09-08 | Fixed password offset drift and improved render stability
line2=V1.5.728 | 2026-09-08 | Integrated trilingual notes and M-pin support
line3=V1.5.727 | 2026-09-08 | Enlarged link confirmation toast
line4=V1.5.720 | 2026-09-07 | Rendering optimization and smooth animations

[description_tr]
line1=V1.5.729 | 2026-09-08 | Sifre ofset kaymasi giderildi ve render kararliligi artirildi
line2=V1.5.728 | 2026-09-08 | Uc dilli notlar entegre edildi ve M pin destegi
line3=V1.5.727 | 2026-09-08 | Baglanti onay karti genisletildi
line4=V1.5.720 | 2026-09-07 | Render optimizasyonu ve akici animasyonlar

[inputs]
in1=I0 المنت باز
in2=I1 المنت بسته
in3=I2 شابلون باز
in4=I3 شابلون بسته
in5=I4 میز باز
in6=I5 برگشت
in7=I6 اضطراری
in8=I7 پدال
in9=GPIO21 CS1 MAX6675 thermocouple TC1 (SO=37 SCK=47)
in10=M0 ورودی/خروجی کمکی 0
in11=M1 ورودی/خروجی کمکی 1

[outputs]
out1=GPIO16 Q0 relay میز
out2=GPIO42 Q1 relay پرس چپ
out3=GPIO39 Q2 relay پرس راست
out4=GPIO48 Q3 relay المنت
out5=GPIO40 Q4 relay شابلون
out6=GPIO15 Q5 SSR heater المنت (software PWM)
out7=GPIO35 Q6 relay ضربه 1
out8=GPIO36 Q7 relay ضربه 2
out9=M0 خروجی/ورودی کمکی 0
out10=M1 خروجی/ورودی کمکی 1
note=all outputs active LOW; emergency stop immediately isolates actuators

[install]
method=esptool write_flash 0x0 bootloader + 0x8000 partitions + 0x10000 firmware (COM17)
command=esptool.py --chip esp32s3 --baud 921600 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
status=STABLE
```

---

## ۴. مکانیسم رول‌آپ تغییرات تجمعی (Cumulative Rollup Logic)

### ۴.۱. نحوه ثبت تغییرات در سورس‌کد
هر پروژه باید دارای فایل مستند `docs/CHANGELOG.md` و هدر `include/fw_changelog.h` باشد. با اعمال هر تغییر، یک سطر با قالب دقیق زیر ثبت می‌شود:
```text
V<Major>.<Minor>.<Patch> | <YYYY-MM-DD> | [fa] <توضیح فارسی> | [en] <English Note> | [tr] <Turkce Not>
```

### ۴.۲. نحوه استخراج و فیلتر در کلاینت HMI
کلاینت نمایشگر با مقایسه نسخه فعلی دستگاه ($V_{\text{installed}}$) با نسخه خطوط سرور ($V_{\text{line}}$)، فیلتر زیر را اعمال می‌کند:
$$\text{If } V_{\text{line}} > V_{\text{installed}} \implies \text{نمایش به عنوان تغییر جدید}$$
$$\text{If } V_{\text{line}} \le V_{\text{installed}} \implies \text{عدم نمایش یا نمایش در بخش سوابق قبلی}$$

بدین ترتیب اگر کاربری ۳ ماه دستگاه خود را آپدیت نکرده باشد، تمامی نکات، اصلاحات امنیتی و قابلیت‌های جدید اضافه شده در نسخه‌های میانی را به صورت یکپارچه مشاهده می‌کند.

---

## ۵. استاندارد ضد مربعی و پالایش متن (Zero-Tofu Sanitization Engine)

گلیف‌های مربعی (`[]` یا `[?]`) زمانی رخ می‌دهند که فونت رندرینگ از کاراکتر ورودی پشتیبانی نکند. برای حل ریشه‌ای این معضل:

### ۵.۱. قوانین فونت پویا
* **زبان فارسی (`lang == 0`):** منحصراً از فونت فارسی پروژه (`ui_font_kalameh_bold_14` یا `ui_font_kalame_bold_14`) با جهت `LV_BASE_DIR_RTL` استفاده شود.
* **زبان‌های انگلیسی و ترکی (`lang == 1, 2`):** منحصراً از فونت استاندارد لاتین (`&lv_font_montserrat_14`) با جهت `LV_BASE_DIR_LTR` استفاده شود.

### ۵.۲. خط لوله پالایش کاراکترها (`fw_sanitize_server_text`)
قبل از ارسال هر متنی از سرور به ویجت‌های LVGL، متن باید از فیلتر زیر عبور کند:
1. کاراکترهای بولت خاص (`•`, `‣`, `⁃`) به کاراکتر امن ASCII `"- "` تبدیل شوند.
2. خط تیره‌های ام‌دش و ان‌دش (`—`, `–`, `―`) به `" - "` تبدیل شوند.
3. علامت‌های نقل قول هوشمند (`“`, `”`, `«`, `»`) به `"` تبدیل شوند.
4. نیم‌فاصله نشکن (`\u00A0`) به فاصله استاندارد (`0x20`) تبدیل شود.
5. سه‌نقطه پیوسته (`…`) به `"..."` تبدیل شود.
6. در زبان ترکی و انگلیسی، کاراکترهای دارای اکسنت و علائم ترکی (`ş`, `ğ`, `ı`, `İ`, `ç`, `ö`, `ü`) به فرم متناظر استاندارد لاتین (`s`, `g`, `i`, `I`, `c`, `o`, `u`) نگاشت شوند تا تحت هر شرایطی متن روان و بدون مربع خوانده شود.

---

## ۶. خطوط قرمز پایه‌ها و سخت‌افزار (Hardware Pinout Red Lines)

1. **ممنوعیت قطعی بازر (No Buzzer in Pinmap):** پایه بازر (`BZ`, `GPIO45 buzzer`) تحت هیچ عنوانی نباید در نقشه پایه‌های اتصالات دستگاه نمایش داده شود.
2. **الزام درج پایه‌های دوطرفه کمکی M:** پایه‌های کمکی `M0..M7` باید به صورت شفاف با عنوان `M0 = ورودی/خروجی کمکی 0` هم در ورودی‌ها و هم در خروجی‌ها (یا ستون اختصاصی) درج شوند.
3. **پالایش ارجاعات GPIO:** نام‌های مستقیم پایه‌های میکروکنترلر مانند `GPIO16` یا `GPIO42` باید در رابط کاربری فیلتر شده و نام‌های استاندارد صنعتی PLC (`I0..I7`, `Q0..Q7`, `T1..T2`, `M0..M7`) ارائه شوند.

---

## ۷. اسکریپت مرجع پایتون جهت ادغام خودکار (`tools/copy_release_artifacts.py`)

کلیه پروژه‌های دارای `platformio.ini` باید اسکریپت استاندارد زیر را در `extra_scripts` معرفی نمایند:
```ini
extra_scripts = post:tools/copy_release_artifacts.py
```
این اسکریپت بلافاصله پس از کامپایل موفق، کلیه فایل‌های ریلیز را اعتبارسنجی کرده و فایل استاندارد `lastfile` را به صورت خودکار با هش SHA-256 و ساختار سه‌زبانه در پوشه `release/` بازتولید می‌کند.
