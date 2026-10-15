# راهنمای استقرار سرور ابری و انتشار OTA دستگاه برش دیجیتال تک‌سر (Digital Cutter Single-Head)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `cutter_single_head`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Cutter_single_head(Digital_metr)`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین دستگاه اره برش تک‌سر از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `upvc/digital-cutter_single_head/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/upvc/digital-cutter_single_head/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/upvc/digital-cutter_single_head/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |
| **نوع محتوا (Content-Type)** | `application/octet-stream` |

---

## ۲. اقلام الزامی بسته انتشار

هر بار که کدهای پروژه تکمیل و بیلد گرفته می‌شود، دقیقاً ۲ فایل زیر باید تولید و در سرور مستقر شوند:
1. **`firmware.bin`**: فایل باینری کامپایل‌شده اپلیکیشن.
2. **`lastfile`**: فایل توصیف‌گر ۸ بخشی استاندارد هافلر شامل نسخه، هش SHA-256، سایز بایت، نقشه پایه‌ها (بدون بازر و با پایه‌های M) و چنج‌لاگ سه‌زبانه.

---

## ۳. دستورات خودکار پکیج، انتشار و اعتبارسنجی

### روش اول: انتشار خودکار با ابزار متمرکز هافلر (توصیه‌شده)
پس از اجرای موفق کامپایل، دستور زیر را در ترمینال ریشه مخزن اجرا کنید:
```bash
python tools/haffler_ota_publisher.py --publish cutter_single_head
```
این دستور کارهای زیر را انجام می‌دهد:
* باینری کامپایل‌شده و `lastfile` را از پوشه‌های بیلد یا ریشه پروژه پیدا می‌کند.
* فایل‌ها را به `s3://ota-plc/upvc/digital-cutter_single_head/` با دسترسی `public-read` آپلود می‌کند.
* فوراً لینک عمومی را تست کرده و گزارش تایید سبز را نمایش می‌دهد.

### روش دوم: تست و بررسی سلامت لینک فعلی سرور
```bash
python tools/haffler_ota_publisher.py --verify cutter_single_head
```

---

## ۴. چک‌لیست اعتبارسنجی دستی برای توسعه‌دهنده و ایجنت
1. آدرس `https://ota.haffler-infi.ir/upvc/digital-cutter_single_head/lastfile` کد وضعیت `200` بازگرداند.
2. آدرس `https://ota.haffler-infi.ir/upvc/digital-cutter_single_head/firmware.bin` کد وضعیت `200` بازگرداند.
3. مقدار `Content-Length` در پاسخ هدر HTTP دقیقاً برابر با مقدار `size_bytes` در `lastfile` باشد.
