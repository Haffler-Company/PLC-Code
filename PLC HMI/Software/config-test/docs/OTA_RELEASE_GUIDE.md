# راهنمای استقرار سرور ابری و انتشار OTA پروژه تست کانفیگ (Config Test)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **شناسه پروژه:** `config_test`  
> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\config-test`

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

کلیه به‌روزرسانی‌های آنلاین پروژه تست کانفیگ از مسیر اختصاصی زیر در فضای ابری آروان‌کلاد دریافت می‌شوند:

| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `upvc/config-test/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/upvc/config-test/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/upvc/config-test/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |
| **نوع محتوا (Content-Type)** | `application/octet-stream` |

---

## ۲. دستورات خودکار انتشار و اعتبارسنجی

### انتشار خودکار با ابزار متمرکز هافلر:
```bash
python tools/haffler_ota_publisher.py --publish config_test
```

### تست و بررسی سلامت لینک فعلی سرور:
```bash
python tools/haffler_ota_publisher.py --verify config_test
```
