# HAFFLER Config Launcher — ESP32-S3

فریمور «کانفیگ» دستگاه‌های HAFFLER: صفحه‌ی راه‌انداز برای ورود ادمین، اتصال Wi-Fi، و نصب/به‌روزرسانی OTA فریمور سایر ماشین‌ها (جوش تک‌سر/دوسر، برش تک‌سر/دوسر، گاز، بستنی، چانه‌گیر و...).

- سخت‌افزار: برد HT-V3A — ESP32-S3، نمایش 480×320 (TFT_eSPI)، تاچ bb_captouch، فلاش 16MB
- فریم‌ور: Arduino + LVGL 9 + EEZ Flow + FreeRTOS
- پورت آپلود: **COM4 یا COM8** (CP210x — شماره پورت می‌تواند عوض شود)؛ **COM3 دستگاه نیست** (پورت Intel AMT مادربرد است — دام رایج)
- ورود: رمز `123` = منوی به‌روزرسانی، رمز `0000` = محیط تست سخت‌افزار

## ساختار پروژه

```
config-test/
├── firmware/
│   ├── platformio.ini          تنظیمات PlatformIO (env: ht_v3a)
│   ├── config/                 lv_conf.h + جدول پارتیشن‌ها
│   ├── include/
│   │   ├── app/                منطق برنامه: actions, vars, screens, storage, lang, hwtest
│   │   ├── ui/                 موتور EEZ Flow (eez-flow + lz4)
│   │   └── drivers/            درایورهای جانبی (اسکرین‌شات و...)
│   └── src/                    فونت‌ها، تصاویر LVGL و hwtest.cpp (نقطه‌ی ورود config_hwtest.ino در ریشه‌ی firmware/ است)
├── docs/                       مستندات فنی و دانش پروژه
└── tools/                      اسکریپت‌های PC (تبدیل تصویر، پرچم‌ها، اسکرین‌شات)
```

## ساخت (Build)

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "<مسیر این پوشه>/firmware"
# آپلود — دستگاه روی COM4 یا COM8 است (CP210x؛ با GetPortNames() چک کن). هرگز روی COM3 آپلود نشود (Intel AMT مادربرد).
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM4 --project-dir "<مسیر این پوشه>/firmware"
```

مهم: بدون درخواست صریح مالک، build/upload انجام نشود.

## مستندات

| فایل | محتوا |
|---|---|
| `docs/START.md` | صفحه‌ی مسیریابی مستندات |
| `docs/REFERENCE.md` | نقشه‌ی سخت‌افزار/فریم‌ور و قوانین فنی |
| `docs/WORKLOG.md` | کارهای جاری و وضعیت نسخه‌ها |
| `docs/CONFIG_TECHNICAL_KNOWLEDGE_FA.md` | دانش فنی پروژه (فارسی) |
| `docs/CONFIG_PERFORMANCE_KNOWLEDGE_FA.md` | دانش عملکرد/بهینه‌سازی |
| `docs/TODO_HANDOFF_FA.md` | کارهای باقی‌مانده و تحویل |
| `docs/SCREENSHOT_TOOL_FA.md` | راهنمای ابزار عکس گرفتن از صفحه |

## ابزارهای کمکی

- `tools/hmi_screenshot.py` — عکس PNG واقعی از فریم‌بافر روی سریال (راهنما: `docs/SCREENSHOT_TOOL_FA.md`)
- `tools/convert_images_v9.py` / `tools/png_to_lvgl.py` — تبدیل PNG به آرایه‌ی C برای LVGL
- `tools/gen_flags.py` — تولید فایل‌های تصویر پرچم‌های زبان
