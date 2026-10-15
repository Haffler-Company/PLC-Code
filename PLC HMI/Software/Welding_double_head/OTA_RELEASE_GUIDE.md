# راهنمای استقرار سرور ابری و انتشار OTA دستگاه جوش دو‌سر (Welding Double-Head)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `welding_double_head`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Welding_double_head`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین دستگاه جوش دو‌سر UPVC از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `upvc/welding-double-head/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/upvc/welding-double-head/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/upvc/welding-double-head/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |
| **نوع محتوا (Content-Type)** | `application/octet-stream` |

---

## ۲. اقلام الزامی بسته انتشار

هر بار که کدهای پروژه تکمیل و بیلد گرفته می‌شود، دقیقاً ۲ فایل زیر باید تولید و در سرور مستقر شوند:
1. **`firmware.bin`**: فایل باینری کامپایل‌شده اپلیکیشن.
2. **`lastfile`**: فایل توصیف‌گر ۸ بخشی استاندارد هافلر شامل نسخه، هش SHA-256، سایز بایت، نقشه پایه‌ها (بدون بازر و با پایه‌های M) و چنج‌لاگ سه‌زبانه.

---

## ۳. دستورات خودکار انتشار و اعتبارسنجی

### انتشار خودکار با ابزار متمرکز هافلر:
```bash
python tools/haffler_ota_publisher.py --publish welding_double_head
```

### تست و بررسی سلامت لینک سرور:
```bash
python tools/haffler_ota_publisher.py --verify welding_double_head
```
