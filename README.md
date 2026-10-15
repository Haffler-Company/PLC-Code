<div align="center">

<img src="Docs/images/haffler_brand_banner.png" alt="Haffler Logo" width="380">

# اکوسیستم کنترلرهای صنعتی و پلتفرم‌های PLC / HMI هافلر
### Haffler Industrial Automation, Embedded PLC & Touchscreen HMI Systems

[![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20ESP32--S3-00979D?style=for-the-badge&logo=espressif&logoColor=white)](#)
[![UI Engine](https://img.shields.io/badge/UI%20Engine-LVGL%20v8%20%2F%20v9-E53935?style=for-the-badge&logo=cplusplus&logoColor=white)](#)
[![Protocol](https://img.shields.io/badge/Industrial-Modbus%20RTU%20%7C%20RS485-FF6F00?style=for-the-badge)](#)
[![Hardware](https://img.shields.io/badge/EDA-Altium%20Designer-008080?style=for-the-badge&logo=altiumdesigner&logoColor=white)](#)
[![Standard](https://img.shields.io/badge/Compliance-MISRA%20C%20%7C%20IEC--61131-4CAF50?style=for-the-badge)](#)

</div>

---

## 📌 معرفی اجمالی مخزن (Overview)

این مخزن مرکز جامع نگهداری کدها، فیرم‌ورها، نقشه‌های شماتیک و برد مدار چاپی (Altium Designer)، مستندات فنی و راهنمای ساخت خانواده کامل کنترلرهای صنعتی و HMI‌های هوشمند شرکت **هافلر** است.

معماری پروژه‌ها مبتنی بر تفکیک دقیق هسته‌های پردازشی (Core Isolation)، عملکرد بدون تاخیر (Zero-Lag)، تایمرها و اینترلاک‌های سخت‌افزاری بلادرنگ، و استانداردهای صنعتی بین‌المللی است.

---

## 🏭 سبد محصولات هافلر (Product Family & Hardware Gallery)

| ردیف | محصول | نمای سخت‌افزاری و پنل کاربری | معماری پردازنده و رابط | کاربرد صنعتی و مستندات |
| :---: | :---: | :---: | :---: | :---: |
| **۱** | **PLC HMI**<br>*(پرچمدار)* | <img src="PLC%20HMI/Docs/PLC%20HMI.png" width="300" alt="PLC HMI"> | **ESP32-S3 Dual-Core**<br>• ال‌سی‌دی رنگی 4 اینچ TFT لمسی<br>• موتور گرافیکی LVGL شصت فریم<br>• پورت ایزوله RS-485 Modbus RTU<br>• وب‌پورتال لوکال و اسکادای ابری | دستگاه‌های صنعتی چندمحوره، اکسترودر، کاترهای اتوماتیک، بستنی‌ساز هوشمند و خطوط تولید<br>📖 [مستندات کامل](PLC%20HMI/Docs/README-PLC-HMI.md) |
| **۲** | **PLC 10K**<br>*(ده کلید)* | <img src="PLC%2010K/Docs/10K_1.png" width="280" alt="PLC 10K"> | **ESP32 SoC**<br>• نمایشگر سون‌سگمنت مولتی‌پارامتر دوقلو<br>• ۱۰ کلید فرمان غشایی صنعتی<br>• خروجی‌های رله اکتیو-LOW و SSR | دستگاه‌های جوش پروفیل، پرس و قالب‌گیری صنعتی با دقت میلی‌ثانیه‌ای<br>📖 [مستندات کامل](PLC%2010K/README-PLC-10K.md) |
| **۳** | **PLC 8K**<br>*(هشت کلید)* | <img src="PLC%208K/Docs/8K_1.png" width="280" alt="PLC 8K"> | **ESP32 SoC**<br>• نمایشگر کاراکتری LCD صنعتی<br>• ۸ کلید دسترسی سریع پارامترها<br>• ورودی‌های ایزوله آنالوگ و دیجیتال | اتوماسیون فرایندی با ورودی/خروجی‌های گسترده و مانیتورینگ متنی<br>📖 [مستندات کامل](PLC%208K/README-PLC-8K.md) |
| **۴** | **PLC 5K**<br>*(پنج کلید)* | <img src="PLC%205K/Docs/5K_1.png" width="280" alt="PLC 5K"> | **ESP32 SoC**<br>• سون‌سگمنت ۳ پارامتری دما و زمان<br>• ۵ کلید کاربری ارگونومیک صنعتی<br>• کنترل حلقه بسته حرارت SSR | کنترل حرارت، زمان‌بندی جوش و سیستم‌های فشرده اتوماسیون کارگاهی<br>📖 [مستندات کامل](PLC%205K/README-PLC-5K.md) |

---

## 🏗️ ساختار درختی مخزن (Repository Layout)

```text
Haffler-Company/PLC/
├── README.md                      # شناسنامه و معرفی رسمی مخزن
├── Docs/                          # مستندات کلان، کاتالوگ و تصاویر
│   ├── images/                    # تصاویر و لوگوهای رسمی هافلر
│   ├── Haffler Catalog.pdf        # کاتالوگ مشخصات محصولات هافلر
│   ├── FIXES.md                   # لاگ بهینه‌سازی‌ها و اصلاحات
│   └── GENERAL_RULES.md           # قوانین مهندسی و خطوط قرمز توسعه
│
├── PLC HMI/                       # پلتفرم HMI لمسی گرافیکی (ESP32-S3)
│   ├── Docs/                      # مستندات تخصصی سخت‌افزار، پین‌اوت و ابعاد
│   ├── Hardware/                  # فایل‌های کامل طراحی Altium Designer (Main & LCD)
│   └── Software/                  # فیرم‌ورهای ماشین‌آلات (کاتر، جوش، بستنی و...)
│
├── PLC 10K/                       # کنترلر صنعتی ۱۰ کلید با سون‌سگمنت
│   ├── Docs/                      # تصاویر پنل و دفترچه راهنما
│   ├── Hardware/                  # شماتیک و PCB بردهای Main و Display
│   └── Software/                  # کدهای نسخه استاندارد و سفارشی TuranTec
│
├── PLC 8K/                        # کنترلر صنعتی ۸ کلید با LCD کاراکتری
│   ├── Docs/                      # مستندات فنی و نقشه‌های کاربری
│   ├── Hardware/                  # طراحی الکترونیک و خروجی‌های ساخت
│   └── Software/                  # فیرم‌ور پایدار صنعتی
│
└── PLC 5K/                        # کنترلر صنعتی ۵ کلید اقتصادی
    ├── Docs/                      # راهنمای پنل و پارامترها
    ├── Hardware/                  # فایل‌های ساخت برد مدار چاپی
    └── Software/                  # نرم‌افزار زمان‌بندی و تنظیم دما
```

---

## ⚡ استانداردهای کلیدی مهندسی هافلر (Engineering Standards)

1. **تفکیک قطعی دو هسته پردازنده (Core Separation):**
   - **هسته ۰ (Core 0):** وظیفه مدیریت شبکه، وای‌فای، سوکت‌های وب‌پورتال، اسکادای ابری و خواندن/نوشتن حافظه فلش (NVS/LittleFS).
   - **هسته ۱ (Core 1):** وظیفه اختصاصی رندرینگ گرافیکی ۶۰ فریم بر ثانیه LVGL و کنترل بدون تاخیر لمسی خازنی GT911.
2. **ایمنی سخت‌افزار و رله‌ها (Active-LOW Relays):**
   - تمامی خروجی‌های رله و ترانزیستوری به صورت Active-LOW عمل می‌کنند (`0=روشن`, `1=خاموش`). در آغاز بوت میکروکنترلر، ابتدا پایه‌ها HIGH شده و سپس به عنوان خروجی پیکربندی می‌شوند تا از هرگونه تریگر ناخواسته جلوگیری شود.
3. **ارتباطات صنعتی بدون مسدودی (Non-Blocking Fieldbus):**
   - بسته‌های داده RS-485 Modbus RTU به صورت آسنکرون و بدون توقف چرخه‌های کنترل حرکتی پردازش می‌شوند.
4. **سیستم امنیت و گارانتی دیجیتال (Hardware Day-Lock):**
   - مجهز به شمارنده امن زمان کارکرد، تقویم جلالی داخلی و الگوریتم‌های چالش-پاسخ ضد دستکاری.

---

<div align="center">
<sub>حقوق معنوی و طراحی‌های مهندسی این مخزن متعلق به <b>شرکت فنی مهندسی هافلر (Haffler)</b> است.</sub>
</div>
