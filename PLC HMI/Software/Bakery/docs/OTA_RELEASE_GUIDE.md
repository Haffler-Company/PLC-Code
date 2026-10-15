# راهنمای استقرار سرور ابری و انتشار OTA خط ماشین‌آلات نانوایی و قنادی (Bakery)
## نظام‌نامه الزام‌آور استقرار خروجی‌های کامپایل و اعتبارسنجی آنلاین لینک‌ها

> **پوشه سورس:** `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Bakery`  
> **ماشین‌های تحت پوشش:** چانه‌گیر نانوایی (`bakery_changir`) و میکسر قنادی (`bakery_mixer`)

---

## ۱. مشخصات و آدرس‌های سرور ابری (Server Destinations)

به‌روزرسانی‌های آنلاین پروژه نانوایی در دو مسیر مجزا در فضای ابری آروان‌کلاد قرار می‌گیرند:

### ۱.۱. دستگاه چانه‌گیر و گردکن خمیر (`bakery-changir`)
| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `other/bakery-changir/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/other/bakery-changir/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/other/bakery-changir/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |

### ۱.۲. دستگاه میکسر قنادی و نانوایی (`bakery-mixer`)
| عنوان | مقدار / آدرس قطعی |
|:---|:---|
| **صندوقچه ابری (Bucket)** | `ota-plc` (منطقه `ir-thr-at1`) |
| **پوشه اختصاصی روی سرور** | `other/bakery-mixer/` |
| **لینک دانلود مستقیم متادیتا** | `https://ota.haffler-infi.ir/other/bakery-mixer/lastfile` |
| **لینک دانلود مستقیم باینری** | `https://ota.haffler-infi.ir/other/bakery-mixer/firmware.bin` |
| **دسترسی (ACL)** | الزماً `public-read` |

---

## ۲. دستورات خودکار پکیج، انتشار و اعتبارسنجی

### انتشار دستگاه چانه‌گیر:
```bash
python tools/haffler_ota_publisher.py --publish bakery_changir
python tools/haffler_ota_publisher.py --verify bakery_changir
```

### انتشار دستگاه میکسر:
```bash
python tools/haffler_ota_publisher.py --publish bakery_mixer
python tools/haffler_ota_publisher.py --verify bakery_mixer
```
