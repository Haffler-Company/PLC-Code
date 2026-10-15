# Welding HMI — V1.5.0

پروژهٔ آمادهٔ ارسال/تولید.

## نسخه

**`include/defines.h`** → `#define VERSION "1.5.0"` — قبل از انتشار حتماً به‌روز کن.

## بیلد (خروجی نهایی)

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "D:\V.1.5.0"
```

بعد از هر بیلد موفق، پوشهٔ **`release/`** فقط این‌ها را دارد (~۵.۳ MB):

| فایل | کاربرد |
|------|--------|
| `firmware.bin` | سرور OTA + esptool |
| `bootloader.bin`, `partitions.bin` | فلش کارخانه |
| `pinmap.txt` | راهنمای پایه کنار firmware روی سرور |
| `FLASH_ESPTOOL.txt`, `README.txt` | دستور فلش + نسخه |

`firmware.elf` عمداً نیست — فقط در `.pio/build/` برای دیباگ.

جزئیات کامل: `WELDING_HMI_KNOWLEDGE_V1.5.0_FA.md` (بخش «پوشه release»).

## آپلود روی برد (اختیاری)

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM17 --project-dir "D:\V.1.5.0"
```

## مسیرها

| مسیر | توضیح |
|------|--------|
| `D:\V.1.5.0` | **این پروژه** — تحویل/ارسال |
| `D:\V.1.4.8_fixpack_2026-06-23` | توسعه |
| `D:\V.1.4.8_fixpack_2026-06-23_archived` | بکاپ‌ها و مستندات قدیمی |
