# راهنمای استقرار سرور ابری و انتشار OTA دستگاه تمیزکننده گوشه (Corner Cleaning)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `corner_cleaning`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Corner_cleaning`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین دستگاه تمیزکننده گوشه UPVC از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `upvc/corner-cleaning/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/upvc/corner-cleaning/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/upvc/corner-cleaning/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |
| **نوع محتوا (Content-Type)** | `application/octet-stream` |

---

## ۲. دستورات خودکار انتشار و اعتبارسنجی

### انتشار خودکار با ابزار متمرکز هافلر:
```bash
python tools/haffler_ota_publisher.py --publish corner_cleaning
```

### تست و بررسی سلامت لینک سرور:
```bash
python tools/haffler_ota_publisher.py --verify corner_cleaning
```
