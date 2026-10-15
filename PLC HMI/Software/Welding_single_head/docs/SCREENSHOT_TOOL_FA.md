# ابزار اسکرین‌شات صفحه HMI (از راه سریال)

این ابزار از پروژه‌ی `Cutter_single_head(Digital_metr)` به این پروژه منتقل شده است.

## اجزا

| فایل | نقش |
|---|---|
| `tools/hmi_screenshot.py` | ابزار سمت PC — از پورت سریال عکس PNG واقعی از فریم‌بافر می‌گیرد |
| `include/agent_hmi.h` | پیاده‌سازی بومی و سریع اسنپ‌شات LVGL 9 (`lv_snapshot_take`) روی ESP32-S3 با ورکر تسک Core 0 |

## پیش‌نیازها (سمت PC)

```powershell
pip install pyserial pillow
```

## استفاده

```powershell
python tools/hmi_screenshot.py --port COM17 --baud 115200 --out shot.png
```

- `--port`: پورت سریال چیپ CP210x دستگاه (در این پروژه معمولاً COM17، COM8 یا COM4). **هشدار:** هرگز از COM3 استفاده نکنید (تله Intel AMT سیستم).
- خروجی پیش‌فرض در `simulator/shots/` ذخیره می‌شود؛ با `--out` مسیر دلخواه بدهید.

## نحوه‌ی کار

1. ابزار دستور `SHOT\n` را روی پورت سریال می‌فرستد.
2. فریم‌ور با استفاده از `lv_snapshot_take` در `include/agent_hmi.h` یک اسنپ‌شات با رنگ RGB565 از صفحه فعال می‌گیرد و داده‌ها را در بافر PSRAM ذخیره می‌کند.
3. تسک `ShotWorker` روی Core 0 داده‌ها را به‌صورت خط‌به‌خط Base64 به سریال ارسال می‌کند (`SHOT_BEGIN 480 320 RGB565LE` ... `SHOT_END`) بدون اینکه رندر Core 1 متوقف یا دچار لگ شود.
4. اسکریپت پایتون داده‌ها را به تصویر PNG تبدیل می‌کند.


## مالکیت

منبع اصلی: `PLC HMI/Software/Cutter_single_head(Digital_metr)/tools/hmi_screenshot.py` — اگر باگ یا بهبودی افزودید، آن را به پروژه‌ی مبدأ هم برگردانید تا ابزارها هم‌نسخه بمانند.
