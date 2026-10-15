# راهنمای استقرار سرور ابری و انتشار OTA دستگاه تزریق گاز اسپیسر شیشه (Gas Filling)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `gas_filling`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Gas-filling`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین دستگاه تزریق گاز از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `upvc/gas-filling/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/upvc/gas-filling/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/upvc/gas-filling/firmware.bin` |
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
python tools/haffler_ota_publisher.py --publish gas_filling
```

### روش دوم: تست و بررسی سلامت لینک فعلی سرور
```bash
python tools/haffler_ota_publisher.py --verify gas_filling
```

---

## ۴. چک‌لیست اعتبارسنجی دستی برای توسعه‌دهنده و ایجنت
1. آدرس `https://ota.haffler-infi.ir/upvc/gas-filling/lastfile` کد وضعیت `200` بازگرداند.
2. آدرس `https://ota.haffler-infi.ir/upvc/gas-filling/firmware.bin` کد وضعیت `200` بازگرداند.
3. مقدار `Content-Length` در پاسخ هدر HTTP دقیقاً برابر با مقدار `size_bytes` در `lastfile` باشد.
