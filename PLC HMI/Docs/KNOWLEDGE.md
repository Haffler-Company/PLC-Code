# دانش فنی PLC HMI

این فایل مرجع فنی فعلی خانواده `PLC HMI` در ساختار همین مخزن است. مسیرهای قدیمی و یادداشت‌های پراکنده قبلی به ساختار فعلی `PLC HMI/Software/...` تبدیل شده‌اند.

## هدف محصول

PLC HMI پلتفرم HMI/PLC لمسی هافلر برای کاربردهای صنعتی مختلف است. پروژه‌ها معمولاً شامل رابط کاربری گرافیکی، کنترل خروجی‌ها، خواندن ورودی‌ها، ذخیره تنظیمات، سنسورها، قفل زمانی و فایل release برای پروگرام هستند.

## پلتفرم مشترک

- میکروکنترلر هدف: خانواده ESP32-S3.
- رابط کاربری: TFT لمسی با LVGL.
- کتابخانه‌های پرتکرار: `LVGL`, `TFT_eSPI`, `bb_captouch`, `RTClib`, `MAX6675`, `ArduinoJson`, `WebSockets`.
- ابزار build بسته به پروژه: Arduino CLI، Arduino IDE یا PlatformIO.
- برای پروژه‌های ESP32-S3 با 16MB Flash و PSRAM، تنظیم اشتباه PSRAM/Flash می‌تواند باعث bootloop شود.

## سخت‌افزار

- برد اصلی: `PLC HMI/Hardware/Main-V4.0.2`
- برد LCD/TFT: `PLC HMI/Hardware/LCD-V4.1.1`
- مرجع سخت‌افزاری مشترک: `PLC HMI/Docs/PLC_HMI_MASTER_REFERENCE.md`
- Touch baseline فعلی:
  - `GPIO8` -> SDA
  - `GPIO9` -> SCL
- برای هر پروژه جدید، pin map همان پروژه باید با master reference مقایسه و اختلاف‌ها در README پروژه ثبت شود.

## پروژه‌های نرم‌افزاری فعلی

| مسیر | وضعیت فعلی | توضیح |
|---|---|---|
| `Software/Config_Plus_Test/V.1.0.0` | دارای سورس و مستندات فنی | پروژه تست/کانفیگ، شامل فایل‌های دانش فنی و TODO |
| `Software/Gas-filling/DemoJamshidi_V0` | دارای سورس و باینری release/build | پروژه پرکن گاز |
| `Software/Bakery/V.1.1.0/ChoneProject` | دارای سورس، docs و release | پروژه Bakery/Chone |
| `Software/Welder_single_head/V.1.5.0` | دارای سورس، release، pin map و دانش فنی | پروژه جوش تک‌سر |
| `Software/libraries` | دارای کتابخانه‌های مشترک | شامل LVGL، TFT_eSPI، RTClib و موارد مشابه |
| پوشه‌های `Aluminum-punching`, `Copy_router`, `Corner_cleaning`, `Digital_cutter_*`, `ICE CREEAM`, `Welder_double_head` | فعلاً خالی/رزرو | باید تکمیل یا حذف شوند |

## قواعد build و release

- سورس پروژه، README، دانش فنی و باینری release باید نگهداری شوند.
- پوشه‌های build کامل مثل `.pio`, `build`, خروجی‌های map/elf و cacheها نباید مبنای release باشند.
- برای release پیشنهادی، از ساختار زیر استفاده شود:

```text
release/
├── bin/
│   ├── firmware.bin
│   ├── bootloader.bin
│   └── partitions.bin
├── FLASH_ESPTOOL.txt
├── pinmap.txt
└── README.txt
```

## نکات LVGL

- پروژه‌های موجود ممکن است LVGL 8 یا LVGL 9 داشته باشند؛ نسخه دقیق باید در README همان پروژه نوشته شود.
- محل `lv_conf.h` باید روشن باشد. اگر داخل sketch است، روش معرفی آن به build tool باید مستند شود.
- مسیرهای دارای فاصله در Windows ممکن است برای بعضی build flagها مشکل ایجاد کنند؛ مسیر build موقت بدون فاصله راه‌حل عملی است.
- در صفحات پرتکرار، متن‌های چندزبانه یا طولانی نباید در هر tick بازنویسی شوند؛ فقط هنگام تغییر زبان/وضعیت به‌روزرسانی شوند.

## قواعد ایمنی و کنترل

- Emergency/Stop باید سخت‌افزارمحور فرض شود و فقط به UI وابسته نباشد.
- خروجی‌های موتور، enable، پالس، جهت، کاتر، SSR و سنسورهای دما باید قبل از تغییر نرم‌افزار با سخت‌افزار واقعی تطبیق داده شوند.
- قبل از flash، نوع برد، Flash size، PSRAM، پورت پروگرام و partition scheme باید قطعی باشد.
- اگر قفل زمانی فعال است، برگشت ساعت RTC یا نامعتبر بودن RTC باید fail-safe و محافظه‌کارانه مدیریت شود.

## قفل زمانی و تنظیمات

الگوی استفاده‌شده در پروژه‌های قبلی:

- قفل زمانی روزمحور باشد، نه وابسته به ساعت/دقیقه.
- وضعیت قفل، تاریخ شروع، تاریخ انقضا، آخرین روز مشاهده‌شده و استفاده از کد اضطراری باید در حافظه غیر فرار ذخیره شود.
- اگر زمان RTC عقب برود، قفل باید فعال شود.
- اگر RTC نامعتبر است و قفل قبلاً فعال بوده، سیستم باید تا تایید ادمین قفل بماند.
- مقادیر ذخیره‌شده مثل دما، روز قفل و تنظیمات حساس باید قبل از اعمال clamp شوند.

## موارد نیازمند تکمیل

- README مستقل برای `Gas-filling/DemoJamshidi_V0`.
- README/دانش فنی تمیز برای `Bakery/V.1.1.0/ChoneProject`.
- تصمیم درباره نگهداری یا حذف build outputها در `Gas-filling/build` و `.pio` پروژه Welder.
- تعیین تکلیف `.git` داخلی در `Bakery/V.1.1.0/ChoneProject` و `Welder_single_head/V.1.5.0`.
- اصلاح نام پوشه `ICE CREEAM` به `Ice_cream` در صورت تایید مالک پروژه.
- حذف یا تکمیل پوشه‌های رزرو شده خالی.
