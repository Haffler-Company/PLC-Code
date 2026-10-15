# راهنمای استقرار سرور ابری و انتشار OTA دستگاه بستنی‌ساز صنعتی (Ice Cream)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `ice_cream`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\ICE CREEAM`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین دستگاه بستنی‌ساز صنعتی از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `other/ice-cream/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/other/ice-cream/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/other/ice-cream/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |
| **نوع محتوا (Content-Type)** | `application/octet-stream` |

---

## ۲. اقلام الزامی بسته انتشار

هر بار که کدهای پروژه تکمیل و با سیستم ESP-IDF بیلد گرفته می‌شود، دقیقاً ۲ فایل زیر باید تولید و در سرور مستقر شوند:
1. **`firmware.bin`**: فایل باینری کامپایل‌شده اپلیکیشن (از مسیر `build/ice_cream_hmi.bin` یا `firmware.bin`).
2. **`lastfile`**: فایل توصیف‌گر ۸ بخشی استاندارد هافلر شامل نسخه، هش SHA-256، سایز بایت، نقشه پایه‌ها و چنج‌لاگ سه‌زبانه.

---

## ۳. دستورات خودکار انتشار و اعتبارسنجی

### انتشار خودکار با ابزار متمرکز هافلر:
```bash
python tools/haffler_ota_publisher.py --publish ice_cream
```

### تست و بررسی سلامت لینک فعلی سرور:
```bash
python tools/haffler_ota_publisher.py --verify ice_cream
```

---

## ۴. چک‌لیست اعتبارسنجی دستی برای توسعه‌دهنده و ایجنت
1. آدرس `https://ota.haffler-infi.ir/other/ice-cream/lastfile` کد وضعیت `200` بازگرداند.
2. آدرس `https://ota.haffler-infi.ir/other/ice-cream/firmware.bin` کد وضعیت `200` بازگرداند.
3. مقدار `Content-Length` در پاسخ هدر HTTP دقیقاً برابر با مقدار `size_bytes` در `lastfile` باشد.
