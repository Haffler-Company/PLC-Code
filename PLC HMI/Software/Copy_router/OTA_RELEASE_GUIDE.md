# راهنمای استقرار سرور ابری و انتشار OTA دستگاه کپی روتور (Copy Router)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `copy_router`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Copy_router`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین دستگاه کپی روتور UPVC از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `upvc/copy-router/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/upvc/copy-router/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/upvc/copy-router/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |
| **نوع محتوا (Content-Type)** | `application/octet-stream` |

---

## ۲. دستورات خودکار انتشار و اعتبارسنجی

### انتشار خودکار با ابزار متمرکز هافلر:
```bash
python tools/haffler_ota_publisher.py --publish copy_router
```

### تست و بررسی سلامت لینک سرور:
```bash
python tools/haffler_ota_publisher.py --verify copy_router
```
