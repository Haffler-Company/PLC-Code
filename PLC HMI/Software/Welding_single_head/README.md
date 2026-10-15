# Welding HMI / PLC — Haffler Company

پروژهٔ آمادهٔ ارسال/تولید دستگاه جوش تک‌سر UPVC (Welding Single Head).

## ساختار نسخه (Unified Versioning)

- نسخهٔ فعال فریم‌ور در `include/build_time.h` به صورت خودکار در هر بیلد با `tools/gen_build_time.py` و `version.json` تولید می‌شود (ماکروی `VERSION`).
- ماکروی fallback در `include/defines.h` قرار دارد.

## بیلد (خروجی نهایی)

```powershell
# بیلد استاندارد فریم‌ور در مسیر پروژه:
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

بعد از هر بیلد موفق، اسکریپت `tools/copy_release_artifacts.py` به صورت خودکار پوشهٔ **`release/`** را با فایل‌های نهایی آماده می‌کند (~۵.۴ MB):

| فایل | کاربرد |
|------|--------|
| `firmware.bin` | سرور OTA + esptool (آدرس `0x10000`) |
| `bootloader.bin` | فلش کارخانه (آدرس `0x0`) |
| `partitions.bin` | جدول پارتیشن‌ها (آدرس `0x8000`) |
| `lastfile` | مانیفست نسخه و چنج‌لاگ تجمعی برای سرور OTA |
| `pinmap.txt` | راهنمای پایه‌ها کنار firmware روی سرور OTA |
| `FLASH_ESPTOOL.txt`, `README.txt` | دستور فلش + شناسه نسخه و تاریخ |

`firmware.elf` عمداً داخل release قرار نمی‌گیرد (تنها در `.pio/build/` برای دیباگ نگهداری می‌شود).

جزئیات کامل: [`docs/WELDING_HMI_KNOWLEDGE_FA.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/WELDING_HMI_KNOWLEDGE_FA.md) (بخش «پوشه release»).

## نقشه ساختار دایرکتوری‌های پروژه (Structured Project Layout)

پروژه بر اساس بالاترین استانداردهای معماری زیمنس و ظرافت اپل سازماندهی و پالایش شده است:

```
Welding_single_head/
├── src/            # سورس‌کدهای اصلی C/C++ فریم‌ور و صفحات رابط کاربری LVGL
├── include/        # هدرهای سیستمی، تعاریف پین‌ها، ماشین‌های حالت و تسک‌های FreeRTOS
├── docs/           # مستندات جامع فنی و مهندسی (ایندکس کامل در docs/README.md)
├── tools/          # ابزارهای رسمی بیلد، تست، مانیتورینگ سریال و مدیریت بکاپ
├── data/           # فایل‌های فرانت‌اند وب‌اپلیکیشن کارگاهی (SPIFFS Filesystem)
├── assets/         # دارایی‌های گرافیکی و موکاپ‌های طراحی (assets/design_mockups/)
├── tests/          # تست‌های خودکار E2E، مرورگر و اعتبارسنجی
├── release/        # پکیج نهایی آماده فلش و انتشار سرور OTA
├── components/     # ماژول‌های مستقل HAL و لایه‌های سخت‌افزاری ESP-IDF
└── lib/            # کتابخانه‌های درایور نمایشگر، تاچ، سنسورها و LVGL 9.4
```

- **راهنمای سریع توسعه و استارت:** [`docs/START.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/START.md)
- **ایندکس جامع کلیه مستندات فنی:** [`docs/README.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/README.md)
- **مشخصات جامع پایه‌ها و سیم‌بندی:** [`docs/HARDWARE_IO_PINOUT.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/Welding_single_head/docs/HARDWARE_IO_PINOUT.md)

## آپلود روی برد (⚠️ فقط با دستور صریح کارفرما)

```powershell
# پورت پیش‌فرض دستگاه COM8 یا COM17 یا COM4 (هرگز COM3 استفاده نشود):
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM8
```
