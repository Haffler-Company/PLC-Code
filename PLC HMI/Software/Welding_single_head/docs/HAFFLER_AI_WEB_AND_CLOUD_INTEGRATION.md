# 🍦 راهنمای جامع اتصال و یکپارچه‌سازی بستنی‌ساز صنعتی با پورتال وب Haffler AI & SCADA
## Haffler Industrial Ice Cream Machine (ASTIAJ CM-700) Cloud & Web SCADA Integration Manual

> **مخاطب:** مهندسین اتوماسیون، توسعه‌دهندگان فریمور و مدیران سیستم‌های اسکادای هافلر.
> **سخت‌افزار هدف:** برد صنعتی Haffler بر پایه ESP32-S3 (N16R8) با ال‌سی‌دی موازی ILI9488، تاچ خازنی GT911، سنسورهای دمای MAX6675 و ساعت RTC DS3231.
> **پورتال وب متناظر:** `C:\Users\Haffler\Documents\General\ai_agent_web` مستقر روی سرور ابری یا محیط محلی.

---

## ۱. معماری کلی ارتباطات دوگانه (Dual-Channel Architecture)

دستگاه بستنی‌ساز هافلر مجهز به دو کانال مستقل و هم‌افزای مخابراتی جهت اتصال به پورتال وب است:

```
                  ┌──────────────────────────────────────────────────────────┐
                  │          Haffler AI & SCADA Web Portal (FastAPI)         │
                  │   (ادیتور کد، چت هوش مصنوعی، تله‌متری زنده، انتشار OTA)     │
                  └────────────┬─────────────────────────────┬───────────────┘
                               │                             │
                     [کانال ۱: سریال فیزیکی]          [کانال ۲: شبکه ابری اینترنت]
                     UART0 / USB-Serial-JTAG                MQTT over TLS/TCP
                     بادریت: 921,600 یا 115,200           پورت 1883 / 8883
                               │                             │
                               ▼                             ▼
                  ┌──────────────────────────────────────────────────────────┐
                  │                 ESP32-S3 Industrial Controller           │
                  │           (serial_hmi.c)                (cloud_client.c) │
                  │                 │                              │         │
                  │                 └──────────────┬───────────────┘         │
                  │                                ▼                         │
                  │                      هسته کنترل FreeRTOS                 │
                  │              (actions_task: پریود ۷۰ میلی‌ثانیه)         │
                  └──────────────────────────────────────────────────────────┘
```

---

## ۲. کانال ۱: پروتکل ارتباطی سریال فیزیکی ($ICE Secure Frame)

### ساختار فریم امنیتی
ارتباط سریال محلی از طریق پورت UART0 (پین‌های پیش‌فرض لاگ و پروگرام) با مکانیزم اعتبارسنجی HMAC-SHA256 محافظت می‌شود:

$$\text{Frame} = \text{"\$ICE,"} + \text{body} + \text{"*"} + \text{HMAC\_SHA256(body, KEY)} + \text{"\textbackslash r\textbackslash n"}$$

- **کلید امنیتی اشتراکی:** `ICE_CREAM_PLC_SECURE_KEY_2026`
- **اجزای بدنه (Body):** `sequence_id,nonce,command,payload`

### فرامین اصلی پشتیبانی‌شده

| فرمان | آرگومان | شرح عملکرد فریمور | نمونه پاسخ کنترلر |
|:---|:---|:---|:---|
| `METRICS` | _بدون آرگومان_ | گزارش بلادرنگ وضعیت تمام سنسورها، رله‌ها، دماها، قیف‌ها و روزشمار | `OK METRICS {"fps":142,...,"temp_top":4.2,"temp_cyl":-7.6,...}` |
| `STATUS` | _بدون آرگومان_ | گزارش سریع رم آزاد و شناسه صفحه فعال | `OK STATUS heap=182400 psram=7200000 screen=2` |
| `EVENT` | `KEY:START` | آغاز چرخه تولید بستنی | `OK EVENT KEY:START` |
| `EVENT` | `KEY:STOP` | قطع کامل تمامی خروجی‌ها (استپ اضطراری) | `OK EVENT KEY:STOP` |
| `EVENT` | `KEY:F4` / `PADDLE:TOGGLE` | تغییر وضعیت کارکرد همزن / پارو | `OK EVENT KEY:F4` |
| `EVENT` | `KEY:F2` / `WASH:TOGGLE` | فعال/غیرفعال‌سازی شستشوی CIP | `OK EVENT KEY:F2` |
| `EVENT` | `KEY:F3` / `HOLD:TOGGLE` | فعال/غیرفعال‌سازی نگهداری شبانه | `OK EVENT KEY:F3` |
| `EVENT` | `RAINBOW:TOGGLE` | تاگل پمپ ژله رنگی | `OK EVENT RAINBOW:TOGGLE` |
| `EVENT` | `EDGE:TOGGLE` | تاگل پمپ کرم لبه رنگی | `OK EVENT EDGE:TOGGLE` |
| `EVENT` | `COUNTER:RESET` | صفر کردن شمارنده روزانه قیف | `OK EVENT COUNTER:RESET` |
| `EVENT` | `LEASE:UNLOCK:<pin>` | تلاش جهت بازگشایی قفل اقساط با رمز | `OK EVENT LEASE:UNLOCK:...` |
| `NAV` | `SCREEN:<id>` / `TAB:<idx>` | ناوبری میان صفحات و تب‌های HMI | `OK NAV SCREEN:2` |
| `LANG` | `FA` / `EN` / `TR` | تغییر آنی زبان رابط کاربری | `OK LANG FA` |
| `SHOT` | _بدون آرگومان_ | ارسال اسکرین‌شات زنده نمایشگر به صورت Base64 یا Raw | `SHOT_START ... SHOT_END` |

---

## ۳. کانال ۲: اینترنت اشیاء ابری (Cloud MQTT Broker)

### تاپیک‌های رسمی MQTT

```
haffler/ice_cream/{dev_id}/online     --> پیام حضور و سلامت برخط دستگاه (LWT)
haffler/ice_cream/{dev_id}/telemetry  --> ارسال بسته کامل تله‌متری هر ۱۰ ثانیه
haffler/ice_cream/{dev_id}/cmd        --> دریافت فرامین کنترلی ارسالی از پورتال وب
haffler/ice_cream/{dev_id}/cmd_resp   --> تاییدیه اجرای فرامین کنترلی
```

### نمونه بسته تله‌متری دریافتی پورتال وب (`telemetry`)
```json
{
  "dev_id": "HAFFLER-ICE-3A4B5C",
  "model": "ICE CREAM CM-700",
  "online": true,
  "cust": "بستنی‌فروشی شاد",
  "city": "اصفهان",
  "sn": "HAFFLER-ICE-3A4B5C",
  "t_top": 4.1,
  "t_cyl": -7.6,
  "t_top_ok": 1,
  "t_cyl_ok": 1,
  "make": 1,
  "hold": 0,
  "paddle": 1,
  "wash": 0,
  "edge": 0,
  "rainbow": 0,
  "ready": 0,
  "dispense": 0,
  "fault": 0,
  "out_comp": 1,
  "out_make": 1,
  "out_pdl": 1,
  "scoops": 184,
  "scoops_day": 45,
  "cone_price": 50000,
  "lease_st": 0,
  "lease_days": 118,
  "lease_locked": 0,
  "rssi": -58,
  "free_heap": 184520,
  "uptime": 14200
}
```

---

## ۴. چرخه به‌روزرسانی ابری OTA و استقرار دو فایله دسکتاپ

1. **بیلد پروژه در پورتال وب:**
   مهندس با استفاده از ویرایشگر وب کدهای سورس (`main/` یا `ui/`) را اصلاح نموده و با هوش مصنوعی ممیزی می‌کند.
2. **انتشار فریمور روی سرور آروان‌کلاد:**
   پورتال وب فایل `build/ili9488_lvgl_btn.bin` را به همراه متادیتای ۸ بخشی `lastfile` روی باکت `ota-plc` در مسیر `other/ice-cream/` آپلود و سلامت لینک‌های CDN را اعتبارسنجی می‌کند.
3. **دریافت آپدیت توسط دستگاه:**
   کلاینت `ota_update.c` در دستگاه با مقایسه نسخه فعلی و سرور، چنج‌لاگ تجمعی را در زبان جاری اپراتور نمایش داده و پس از تأیید، به‌روزرسانی دوال پارتیشن را بدون خطر بریک شدن اعمال می‌نماید.

---

## ۵. کانال ۳: کاوش و شناسایی خودکار در شبکه محلی (Local Network Auto-Discovery)

پورتال وب هافلر علاوه بر اسکن مادباس پورت ۵۰۲، از طریق تابع `scan_network_esp32()` به طور موازی تمام آی‌پی‌های ساب‌نت (۱ الی ۲۵۴) را روی پورت ۸۰ و ۸۰۸۰ بررسی می‌کند.

### اندپوینت شناسایی (`GET /api/info`)
وب‌سرور داخلی فریمور (`webportal.c`) پاسخ زیر را بلافاصله با کد ۲۰۰ بازمی‌گرداند:
```json
{
  "status": "ok",
  "id": "HAFFLER-ICE-CM700",
  "device_id": "HAFFLER-ICE-CM700",
  "model": "ICE CREAM CM-700",
  "firmware": "v2.6.4",
  "uptime": 14200,
  "free_heap": 184520,
  "min_free_heap": 142000
}
```
با دریافت این پاسخ، پورتال وب فوراً دستگاه را در لیست تجهیزات آنلاین با دکمه اتصال مستقیم (۱-Click Connect) نمایش می‌دهد.

---

## ۶. پخش زنده نمایشگر و آینه‌سازی HMI (Live Screen Streaming)

### ۱. استریم مستقیم از وب‌سرور داخلی فریمور (`GET /api/screenshot`)
- فریمور از طریق `serial_hmi_capture_sync()` فریم‌بافر ۴۸۰ در ۳۲۰ پیکسل LVGL را قفل کرده و بدون نیاز به انکودر ویدیویی سنگین، هدر استاندارد BMP ۱۶ بیتی RGB565 (سرعت بسیار بالا، حجم ۳۰۷ کیلوبایت) را تولید و با هدر `Content-Type: image/bmp` مستقیماً به کلاینت وب ارسال می‌کند.
- قابلیت Cache-Control برای جلوگیری از کش مرورگر فعال است (`no-cache, no-store, must-revalidate`).

### ۲. ارسال اسکرین‌شات ابری از طریق MQTT (`take_screenshot`)
- هنگام ارسال فرمان `take_screenshot` روی تاپیک `haffler/ice_cream/{dev_id}/cmd`، فریمور تصویر را ضبط کرده و از طریق درخواست HTTP POST به اندپوینت پورتال هافلر (`/api/plc/upload-screenshot`) با هدر امنیتی اختصاصی ارسال می‌کند:
  `X-Device-Key: ICE_CREAM_PLC_SECURE_KEY_2026`

### ۳. مانیتورینگ زنده در رابط وب هافلر
- با زدن دکمه «پخش زنده» (`btnHmiLiveStream`) در پنل اسکادای وب هافلر، حلقه تایمر ۲.۵ ثانیه‌ای فعال شده و با نشانگر سبزرنگ `LIVE` آخرین تصویر نمایشگر را بدون وقفه نمایش می‌دهد.

---

## ۷. کنترل از راه دور و سطوح دسترسی (RBAC: Admin / Technician)

پورتال وب فرامین کنترلی را با امضای نقش کاربر به دستگاه مخابره می‌کند:

| سطح دسترسی | فرامین مجاز | محدودیت‌ها |
|:---|:---|:---|
| **تکنسین (Technician)** | تغییر دمای مخزن (`set_param:hopper_temp`)، تنظیم درصد غلظت (`set_param:viscosity`)، راه‌اندازی شستشو (`start_wash`)، نگهداری شبانه (`start_hold`)، استارت/استپ همزن (`toggle_beater`) | عدم دسترسی به بازگشایی قفل اقساط و تنظیمات کارخانه |
| **مدیر ارشد (Admin)** | دسترسی کامل به کلیه عملگرها (`toggle_compressor`, `toggle_beater`, `toggle_air_pump`)، قیمت قیف، بازگشایی اقساط (`lease_unlock`)، فلش ابری فریمور | دسترسی نامحدود |

### فرامین MQTT اضافه شده در فریمور (`cloud_client.c`)
- `take_screenshot`: فراخوانی کپچر فریم‌بافر و آپلود به پورتال وب.
- `toggle_compressor`: قطع یا وصل رله کمپرسور اصلی.
- `toggle_beater`: قطع یا وصل رله موتور همزن.
- `toggle_air_pump`: قطع یا وصل رله پمپ هوادهی.
- `set_param:hopper_temp:<val>`: تنظیم دمای تارگت مخزن.
- `set_param:viscosity:<val>`: تنظیم نقطه کار ویسکوزیته و سفتی بستنی.

> **نکته نام‌گذاری تاپیک‌ها:** کلاینت ابری فریمور جهت سازگاری ۱۰۰٪ با تمامی نگارش‌های پورتال وب، همزمان در تاپیک‌های `haffler/ice_cream/#` و `haffler/ice_creeam/#` عضویت (Subscribe) دارد.
