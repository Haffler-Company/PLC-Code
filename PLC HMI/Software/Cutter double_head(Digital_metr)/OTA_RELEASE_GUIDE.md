# راهنمای استقرار سرور ابری و انتشار OTA دستگاه برش دیجیتال دو‌سر (Digital Cutter Double-Head)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `cutter_double_head`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Cutter double_head(Digital_metr)`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین دستگاه اره دو‌سر از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `upvc/digital-cutter double_head/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/upvc/digital-cutter%20double_head/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/upvc/digital-cutter%20double_head/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |
| **نوع محتوا (Content-Type)** | `application/octet-stream` |

---

## ۲. دستورات خودکار انتشار و اعتبارسنجی

### انتشار خودکار با ابزار متمرکز هافلر:
```bash
python tools/haffler_ota_publisher.py --publish cutter_double_head
```

### تست و بررسی سلامت لینک سرور:
```bash
python tools/haffler_ota_publisher.py --verify cutter_double_head
```
