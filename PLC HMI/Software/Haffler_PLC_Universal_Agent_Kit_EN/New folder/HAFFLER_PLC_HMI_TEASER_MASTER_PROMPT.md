# مستند جامع فنی و پرامپت تیزر معرفی کنترلر صنعتی هافلر (Haffler Industrial PLC HMI)
## شناسنامه سخت‌افزاری، مشخصات قطعی الکترونیک و پرامپت‌های استودیویی تولید تیزر تبلیغاتی (3D / AI Video)

> **مخاطب سند:** طراحان موشن‌گرافیک ۳D، سازندگان ویدیو با هوش مصنوعی (Runway, Sora, Kling, Midjourney)، مدیران بازاریابی، گویندگان تیزر و مهندسان اتوماسیون.  
> **دستگاه هدف:** کنترلر همه‌منظوره صنعتی هافلر با صفحه لمسی یکپارچه (**Haffler Industrial PLC HMI Controller**).  
> **شعار تجاری محصول:** *قدرت و صلابت صنعتی زیمنس در ظرافت و زیبایی اپل (Siemens Rigor meets Apple Design)*.

---

# بخش اول: شناسنامه و کاتالوگ فنی سخت‌افزار (Definitive Technical Specifications)

این کنترلر یک سامانه یکپارچه **PLC + HMI** صنعتی است که بدون نیاز به هیچ PLC جانبی، کنترل ماشین‌آلات خط تولید و واسط کاربری لمسی را به طور مستقل انجام می‌دهد:

### ۱. پردازشگر مرکزی و حافظه (MCU & Architecture)
* **پردازنده مرکزی:** تراشه دوهسته‌ای **ESP32-S3-WROOM-1 (N16R8)** با معماری ۳۲ بیتی Xtensa® LX7 تا فرکانس **۲۴۰ مگاهرتز**.
* **موتور شتاب‌دهنده برداری (Vector Extensions):** مجهز به دستورالعمل‌های پردازش سیگنال (DSP) و هوش مصنوعی لبه (Edge AI).
* **حافظه فلش (ROM):** **۱۶ مگابایت** پرسرعت Octal SPI Flash با پشتیبانی از دو پارتیشن A/B جهت آپدیت ابری ایمن (Anti-Brick Dual Boot).
* **حافظه رم جانبی (PSRAM):** **۸ مگابایت** پرسرعت Octal SPI PSRAM اختصاصی جهت میزبانی بافرهای سنگین گرافیک و وب‌سرور.
* **حافظه رم داخلی (SRAM):** **۵۱۲ کیلوبایت** SRAM پرسرعت با قابلیت دسترسی مستقیم به حافظه (DMA).
* **معماری تفکیک دو هسته‌ای (Dual-Core Determinism):**
  - **هسته ۰ (Core 0 - Control & Connectivity):** اسکن چرخه‌ای فرآیند صنعتی (پریود قطعی ۵۰ تا ۷۰ میلی‌ثانیه)، پروتکل Modbus، وب‌پورتال و پایش ایمنی.
  - **هسته ۱ (Core 1 - Visual Engine):** رندرینگ ۶۰ فریم گرافیک و اسکن پنل لمسی خازنی.

---

### ۲. نمایشگر و پنل لمسی (Display & Industrial Touch)
* **سایز و فناوری پنل:** صفحه نمایش رنگی **۳.۵ اینچ** (یا ۴.۰ اینچ سفارشی) با فناوری **IPS TFT LCD**.
* **رزولوشن تصویر:** **۴۸۰ × ۳۲۰ پیکسل** افقی (Landscape).
* **زاویه دید:** زاویه دید فوق‌عریض **۱۷۸ درجه** (دید کامل از زوایای تند در سالن کارگاهی بدون افت رنگ و کنتراست).
* **درایور گرافیکی:** تراشه‌های صنعتی **ILI9488** یا **ST7796S**.
* **گذرگاه ارتباطی تصویر:** Hardware SPI با کلاک کاری تا **۸۰ مگاهرتز** و کانال انتقال داده DMA دو بافره (بدون پرش، لرزش یا خط‌افتادگی تصویر).
* **موتور گرافیکی:** کتابخانه پیشرفته **LVGL (Light and Versatile Graphics Library)** با تم اختصاصی Dark Slate اپل و انیمیشن‌های نرم ۶۰FPS.
* **پنل لمسی خازنی (Capacitive Touch):** تراشه **Goodix GT911** بر بستر گذرگاه صنعتی I2C (فرکانس ۴۰۰ کیلوهرتز).
* **محافظ شیشه‌ای:** شیشه حرارت‌دیده صنعتی مقاوم در برابر خط‌وخش، نفوذ روغن‌های صنعتی، براده و پاشش قطرات آب.
* **الگوریتم نرم‌افزاری ضد نویز:** مجهز به فیلتر حذف نویز الکترومغناطیسی (EMI Anti-Ghosting) جهت ممانعت از کلیک‌های کاذب در مجاورت اینورترها و جرقه‌ها.

---

### ۳. ورودی‌ها و حسگرهای صنعتی (Inputs & Sensors)
* **ورودی‌های دیجیتال (Digital Inputs):**
  - **۸ کانال ورودی ایزوله صنعتی (I0 تا I7)** با اپتوکوپلرهای نوری سرعت‌بالا (PC817 / TLP521).
  - پشتیبانی از ولتاژ ۲۴ ولت و ۵ ولت صنعتی (Active-LOW با مقاومت‌های پول‌آپ و پول‌دان حفاظتی).
  - پشتیبانی از سنسورهای القایی، لیمیت‌سوئیچ‌ها، پدال پایی، کلید قارچی توقف اضطراری (E-Stop) و سنسورهای سرعت پولی / پالس انکودر.
  - پشتیبانی از کدگذاری دو سنسور روی یک سیم بر پایه باندهای ولتاژی آنالوگ.
* **کانال‌های ورودی کلیدهای فیزیکی پنل:**
  - **۲ کانال آنالوگ نردبانی مجزا (KEY_F روی GPIO 1 و KEY_M روی GPIO 2)** مجهز به نمونه‌برداری ۱۲ بیتی ADC با فیلتر میانه ۵ نمونه‌ای جهت راه‌اندازی تا **۱۰ کلید لمسی/مکانیکی صنعتی روی قاب پنل** (F1..F5 و کلیدهای ناوبری MODE/START/STOP بدون نیاز به لمس صفحه).
* **کانال‌های اندازه‌گیری دما (Dual Thermocouples):**
  - **۲ کانال مجزای سخت‌افزاری MAX6675** بر بستر گذرگاه SPI اختصاصی (SO=GPIO 37, SCK=GPIO 47).
  - پشتیبانی مستقیم از ترموکوپل‌های صنعتی تیپ K با غلاف فلزی.
  - محدوده دمایی: **۰ تا ۱۰۲۴ درجه سانتی‌گراد** با دقت و تفکیک‌پذیری **۰.۲۵ درجه**.
  - مجهز به سیستم سخت‌افزاری و نرم‌افزاری تشخیص قطعی پراب (Open Thermocouple Detection) و جبران‌ساز پیوند سرد.
* **درگاه توسعه ورودی/خروجی (I/O Expander):**
  - پشتیبانی درایور داخلی از چیپ افزاینده **MCP23017** از طریق درگاه I2C جهت افزایش پایه‌های ورودی و خروجی کمکی (M0..M7).

---

### ۴. خروجی‌های فرمان و قدرت (Digital Outputs & Relays)
* **تعداد کل خروجی‌ها:** **۱۰ خروجی دیجیتال صنعتی مستقل**.
* **خروجی‌های رله الکترومکانیکی:**
  - **۹ کانال رله صنعتی با کنتاکت ۱۰ آمپر (۲۵۰VAC / ۳۰VDC)** با ایزولاسیون کامل اپتوکوپلری برای راه‌اندازی شیرهای برقی پنوماتیک، کنتاکتورهای موتور، المنت‌ها و جک‌های هیدرولیک.
  - منطق تحریک منفی صنعتی (**Active-LOW**).
  - دیودهای هرزگرد موازی (1N4007 Flyback Diodes) و مدارهای اسنابر RC جهت مهار تخلیه سلفی بوبین‌ها.
* **خروجی رله حالت جامد سرعت‌بالا (SSR Output):**
  - **۱ کانال خروجی SSR پرسرعت (Q5 روی GPIO 15)** با پشتیبانی از مدولاسیون پهنای پالس (Software/Hardware PWM) جهت کنترل دمای فوق‌دقیق PID المنت‌های حرارتی با عمر بی‌نهایت و صفر فرسایش مکانیکی.
* **حفاظت قطعی لحظه صفر بوت (Boot Zero-Glitch Clamping):**
  - تمامی خروجی‌ها در نخستین نانوثانیه‌های اتصال برق، پیش از بالا آمدن سیستم‌عامل به صورت سخت‌افزاری روی حالت قطع قفل می‌شوند تا از لرزش یا روشن شدن ناخواسته ادوات در لحظه استارت (Relay Chatter) جلوگیری شود.

---

### ۵. پورت‌های ارتباطی و شبکه (Communication & Ports)
* **پورت سریال صنعتی RS485:**
  - فرستنده/گیرنده ایزوله با پین کنترل جهت بلادرنگ سخت‌افزاری (DE/RE روی GPIO 46).
  - پورت سریال سخت‌افزاری UART2 با باودریت قابل تنظیم از ۹۶۰۰ تا ۱۱۵۲۰۰ بیت بر ثانیه.
  - پشتیبانی کامل از پروتکل استاندارد **Modbus RTU (حالت Master و Slave)** برای شبکه شدن با اینورترها (VFD)، درایورهای سروو و سیستم‌های بالادست SCADA تا فاصله ۱۲۰۰ متری.
  - پشتیبانی داخلی از پرینترهای حرارتی صنعتی RS485 جهت چاپ بارکد، رسید تولید، تمبر کنترل کیفیت (QC Stamp) و آمار شیفت کاری.
* **پورت برنامه‌ریزی، عیب‌یابی و مانیتورینگ پرسرعت (USB-to-UART):**
  - مجهز به مبدل داخلی **Silicon Labs CP210x** متصل به درگاه سخت‌افزاری UART0.
  - سرعت تبادل داده فوق‌سریع **۹۲۱,۶۰۰ باود** جهت پروگرم در چند ثانیه، ارسال زنده‌ی تلمتری، اسکرین‌شات لحظه‌ای با پروتکل امن اختصاصی HSSRP.
* **گذرگاه باس I2C صنعتی:**
  - باس پرسرعت ۴۰۰ کیلوهرتز (SDA=GPIO 8, SCL=GPIO 9) با مقاومت‌های پول‌آپ خارجی و تایم‌اوت‌های ضد فریز برای تاچ، ساعت و افزاینده‌ها.
* **گذرگاه باس SPI صنعتی:**
  - فرکانس تا ۸۰ مگاهرتز اختصاصی نمایشگر و ترموکوپل‌ها با پین‌های Chip-Select مجزا.

---

### ۶. ارتباطات بی‌سیم و اینترنت اشیاء صنعتی (Wireless & IIoT)
* **وای‌فای داخلی (Wi-Fi 2.4 GHz):**
  - استاندارد 802.11 b/g/n با تقویت‌کننده خروجی توان بالا (**+19.5 dBm**) جهت نفوذ سیگنال از میان تابلوبرق‌های ضخیم فلزی.
  - کارکرد دوگانه به صورت نقطه دسترسی محلی (**Access Point / Captive Portal**) و اتصال به شبکه کارخانه (**Station Mode**).
* **وب‌سرور داخلی (Embedded WebPortal):**
  - ارائه وب‌اپلیکیشن ریسپانسیو فوق‌سبک (فشرده‌سازی GZIP زیر ۱۶ کیلوبایت) برای کنترل و پایش دستگاه با گوشی موبایل، تبلت و کامپیوتر بدون نیاز به نصب نرم‌افزار.
  - کانال تلمتری بلادرنگ **WebSocket** با نرخ بروزرسانی ۲۵۰ میلی‌ثانیه.
* **به‌روزرسانی ابری خودکار (Cloud OTA):**
  - اتصال مستقیم به سرور بروزرسانی پایدار (`https://ota.haffler-infi.ir`) جهت آپدیت آنلاین فریمور در چند ثانیه با مقایسه نسخه، دانلود تک‌فایل باینری و لاگ تغییرات تجمعی (Rollup Changelog).
* **بلوتوث کم‌مصرف (BLE 5.0):**
  - راه‌اندازی سریع اولیه، کانفیگ رمزها، کالیبراسیون و قرائت خطاهای سیستم از طریق اپلیکیشن موبایل.

---

### ۷. سیستم ساعت نجومی و تقویم دوزمانه (High-Precision RTC)
* **تراشه ساعت بلادرنگ:** آی‌سی صنعتی **Dallas / Maxim DS3231**.
* **نوسان‌ساز کریستالی تصحیح‌شده با دما (TCXO):** خطای فوق‌العاده کم (کمتر از ۱ دقیقه در کل سال حتی در نوسانات دمایی منفی ۲۰ تا مثبت ۷۰ درجه سانتی‌گراد).
* **باتری پشتیبان سخت‌افزاری (Backup Coin Battery):** اسلات باتری سکه‌ای (CR1220 / CR2032) با طول عمر ۵ ساله برای نگهداری ساعت و تاریخ هنگام قطعی کامل برق.
* **تقویم دوزمانه میلادی و هجری شمسی:** مبدل سبک داخلی بدون باگ سال‌های کبیسه، مناسب ثبت گزارش‌های روزانه و شیفت کاری.
* **سیستم قفل اقساط ۵ مرحله‌ای (Day-Lock Security):** ساب‌سیستم ضد دستکاری ساعت سخت‌افزاری (Monotonic Anti-Tamper) همراه با چالش-پاسخ امنیتی جهت مدیریت فروش اقساطی دستگاه.

---

### ۸. تغذیه، بازر و تاب‌آوری الکترومغناطیسی (Power & Ruggedness)
* **ولتاژ ورودی نامی:** **۲۴ ولت مستقیم (24V DC)** با بازه کاری عریض (۱۸ الی ۳۶ ولت DC).
* **رگولاتورهای سوئیچینگ صنعتی:** دو طبقه مبدل Buck با راندمان بالا جهت تولید تغذیه ۵ ولت بوبین‌ها و ۳.۳ ولت مدارات منطقی.
* **سیستم هشدار صوتی:** بازر پیزوالکتریک صنعتی (GPIO 45) با مدولاسیون فرکانسی PWM جهت ارائه فیدبک لمس و آلارم‌های استاندارد ISA-18.2.
* **حفاظت براون‌اوت (Brownout Recovery):** پایش افت ولتاژ شبکه برق کارگاه و ذخیره وضعیت جاری در حافظه فوق‌سریع RTC SRAM جهت بازیابی نامحسوس صفحه بعد از وصل مجدد برق (Zero Flash-Wear).
* **واچ‌داگ دوگانه:** پایش همزمان نرم‌افزاری و سخت‌افزاری سلامت کدهای کنترلر جهت ریست ایمن سیستم در کمتر از ۱ ثانیه در صورت بروز نویزهای مهیب صاعقه یا جرقه.

---

# بخش دوم: پرامپت مادر برای ابزارهای تولید ویدیو و تصویر هوش مصنوعی
### (Master Visual Prompt for Runway Gen-3 / Sora / Kling / Midjourney / 3D CGI)

این پرامپت‌ها را می‌توانید مستقیماً در ابزارهای تولید تصویر و ویدیوی هوش مصنوعی قرار دهید:

```text
Cinematic 4K macro commercial shot of the Haffler Industrial PLC HMI Controller, sleek matte-black anodized aluminum chassis with chamfered edges, dark frosted glass front panel. 

The center showcases an ultra-crisp 3.5-inch IPS display glowing with an Apple-style dark mode industrial user interface (deep slate blue #0F172A, neon emerald green indicators, crisp white typography, zero lag, silky smooth 60 FPS gauge animation). 

Camera slowly tracks across the unit:
- Close-up on the emerald green and slate blue heavy-duty screw terminal blocks labeled I0 to I7 (optocoupled 24V inputs) and Q0 to Q9 (10 high-power relay and SSR outputs).
- Macro zoom into the transparent window revealing the ESP32-S3 dual-core processor glowing with subtle cyan data pulses, labeled '240MHz DUAL CORE - 16MB FLASH - 8MB PSRAM'.
- Subtle laser engravings on the metal side: 'RS485 MODBUS', 'WI-FI 2.4G / BLE 5.0', '24V DC INDUSTRIAL', 'DUAL MAX6675 THERMOCOUPLE'.
- Floating holographic telemetry UI overlays showing real-time 60 FPS graphs, Wi-Fi antenna waves, and temperature readouts.
- Dramatic studio rim lighting, subtle industrial smoke atmosphere, dark cinematic background, ultra-photorealistic, Octane Render style, Unreal Engine 5 tech product showcase aesthetic, depth of field, 8k resolution.
```

---

# بخش سوم: فیلم‌نامه و سناریوی ثانیه‌به‌ثانیه تیزر تبلیغاتی (Teaser Storyboard)
### مدت زمان: ۶۰ ثانیه | ضرب‌آهنگ: مدرن، کوبنده، صنعتی و هیجان‌انگیز

| زمان | نمای بصری (Visual Shot) | تایتل متنی روی تصویر (Lower Third) | متن نریشن / گویندگی (Voiceover) |
| :---: | :--- | :--- | :--- |
| **00:00 - 00:08** | **شروع در تاریکی:** بارقه‌های نور آبی بر روی لبه‌های تراش‌خورده قاب آلومینیومی می‌تابد. صفحه ۳.۵ اینچی IPS با لوگوی درخشان هافلر در یک چشم برهم زدن روشن می‌شود. | **HAFFLER PLC HMI**<br>*نسل نوین اتوماسیون صنعتی* | «ترکیب قدرت بی‌پایان زیمنس، با اوج ظرافت و زیبایی اپل... کنترلر یکپارچه لمسی هافلر متولد شد.» |
| **00:08 - 00:18** | **نمای انفجاری از مغز پردازنده (Exploded View):** قطعات به نرمی از هم باز می‌شوند. روی تراشه مرکزی زوم می‌شود: دو هسته پردازشی با پالس‌های فیروزه‌ای نورانی. | **مغز متفکر ۲۴۰ مگاهرتز**<br>Dual-Core Xtensa LX7<br>16MB Flash \| 8MB PSRAM | «پردازنده قدرتمند دوهسته‌ای ۲۴۰ مگاهرتز با ۱۶ مگابایت فلش و ۸ مگابایت رم پرسرعت؛ تفکیک کامل هسته کنترل فرآیند از هسته گرافیک برای تضمین ۱۰۰٪ پایداری.» |
| **00:18 - 00:28** | **شات ماکرو از ترمینال‌ها و رله‌ها:** دوربین با زاویه تند روی ترمینال‌های صنعتی سر می‌خورد. رله‌های مشکی با چراغ‌های ال‌ای‌دی سبز نئونی به نوبت تیک می‌زنند. یک چارت حرارتی درخشان در کنار خروجی SSR پدیدار می‌شود. | **۱۰ خروجی پرقدرت**<br>9x Relays (10A) + 1x High-Speed SSR<br>8x Opto Inputs \| Dual MAX6675 | «۱۰ خروجی دیجیتال پرقدرت شامل رله‌های صنعتی ایزوله و خروجی SSR با کنترل حرارتی فوق‌دقیق PID؛ در کنار ورودی‌های اپتوکوپلری مصون از هرگونه نویز محیط کارگاه.» |
| **00:28 - 00:38** | **پورت‌های ارتباطی و آنتن بی‌سیم:** نوری زرین از پورت RS485 جریان می‌یابد. امواج وای‌فای سه‌بعدی به بیرون ساطع می‌شوند. یک تبلت در کنار دستگاه، داشبورد وب را بدون وقفه لود می‌کند. | **ارتباطات نامحدود صنعتی**<br>RS485 Modbus RTU<br>Wi-Fi + BLE 5.0 + Embedded WebServer | «پورت صنعتی RS485 مجهز به پروتکل مالبوس و شبکه پرسرعت وای‌فای با وب‌سرور داخلی؛ خط تولید شما همیشه و از هر جای کارگاه روی تلفن همراه در دسترس است.» |
| **00:38 - 00:48** | **تجربه کاربری و نرم‌افزار روان:** دست اپراتوری با دستکش چرمی صنعتی روی صفحه لمسی خازنی ضدضربه تاچ می‌زند. منوهای سه‌زبانه (فارسی، انگلیسی، ترکی) با نرمی ۶۰ فریم تعویض می‌شوند. | **صفحه لمسی ۶۰ فریم IPS**<br>Anti-Scratch Capacitive Touch<br>ساعت دقیق نجومی DS3231 | «نمایشگر ۳.۵ اینچ IPS ضدخش با تاچ خازنی صنعتی، ساعت فوق‌دقیق تقویم شمسی و آپدیت آنلاین ابری ابدی.» |
| **00:48 - 00:60** | **شات پایانی پرچمدار:** قطعات به هم جفت می‌شوند. دستگاه درون تابلوی برق صنعتی جا می‌گیرد. مهر سبز رنگ رسمی "100% QC PASSED" روی نمایشگر نقش می‌بندد. لوگو و وب‌سایت هافلر ظاهر می‌شود. | **HAFFLER AUTOMATION**<br>Smart Industrial Controllers<br>www.haffler.ir | «هافلر؛ نهایت هوشمندی و اطمینان در قلب ماشین‌آلات صنعتی.» |

---

# بخش چهارم: کدهای دستوری و پرامپت‌های تفکیک‌شده عکس‌برداری (Image Prompt Pack)

### ۱. پرامپت رندر روبه‌رو (Front Hero Shot - 3D Render)
```text
Studio product photograph of the Haffler 3.5-inch Industrial PLC HMI panel, front elevation view. Sleek matte graphite-black enclosure, narrow bezel, premium brushed chamfered edges. 3.5" IPS display is on, showing a dark-mode industrial control dashboard with circular gauges, status pills in neon emerald and electric blue, and crisp white typography. Below the screen, discreet physical tactile keys with subtle blue LED backlights. Isolated on a dark reflective pedestal with soft studio light bars, premium tech aesthetic, 8k resolution, photorealistic.
```

### ۲. پرامپت رندر زاویه‌دار و ترمینال‌های پشت (Rear / Angled I/O Terminals)
```text
Isometric angled perspective of the Haffler Industrial PLC controller hardware board. Detailed focus on industrial detachable screw terminal blocks: 10 digital outputs with relays, 8 optocoupled 24V inputs, RS485 Modbus connector, dual K-type thermocouple inputs with ceramic terminal sockets, gold-plated USB-C debug port, onboard coin battery for DS3231 RTC. High-density industrial PCB layout with matte black solder mask, exposed gold vias, clean surface mount electronics, ultra-detailed engineering photography, shallow depth of field.
```

### ۳. پرامپت محیط کارگاهی واقعی (In-Situ Industrial Cabinet)
```text
Cinematic shot inside a modern clean German-style industrial electrical cabinet (Schrack / Rittal). The Haffler 3.5" Touch PLC HMI is mounted neatly on the cabinet door. Cable ducts, DIN rail contactors, Schneider breakers, and neat wiring surrounding it. Ambient cool LED strip lights inside the panel reflect softly off the screen. An engineer's hand wearing a protective antistatic glove touches the screen to adjust a parameter. Photorealistic, 35mm film still, industrial automation atmosphere.
```

---

# بخش پنجم: متن نریشن ضبط استودیویی (Studio Voiceover Scripts)

### نسخه اول: متن نریشن فارسی (حماسی، صنعتی و پرچمدار)
> «وقتی پای دقت میکرومتری، سرعت بی‌وقفه و کارکرد شبانه‌روزی در سخت‌ترین شرایط صنعتی در میان است، به مغزی نیاز دارید که هرگز خسته نشود.  
> **کنترلر یکپارچه لمسی هافلر.**  
> مجهز به پردازنده پرقدرت دوهسته‌ای ۲۴۰ مگاهرتز با ۱۶ مگابایت حافظه ذخیره‌سازی، ۱۰ خروجی پرقدرت رله و SSR با خنک‌کاری مدرن، ۸ ورودی کامپکت ایزوله و دو کانال سنجش دمای ترموکوپل تیپ K.  
> نمایشگر رنگی ۳.۵ اینچی ضدخش با زاویه دید باز و فناوری تاچ خازنی بدون نویز، همراه با پورت استاندارد RS485 مالبوس و شبکه وای‌فای داخلی با قابلیت نظارت ابری و وب‌اپلیکیشن کارگاهی.  
> صلابت و ایمنی سخت‌گیرانه صنعتی، در روکش چشم‌نواز طراحی نسل آینده.  
> **هافلر؛ تعریف مدرن کنترل ماشین‌آلات صنعتی.**»

---

### نسخه دوم: متن انگلیسی بین‌المللی (Global English Voiceover)
> *"In the demanding world of modern manufacturing, precision and reliability define success.  
> Introducing the **Haffler All-in-One Industrial PLC HMI**.  
> Powered by an ultra-fast 240MHz dual-core processor with 16MB Flash and 8MB PSRAM, seamlessly separating deterministic machine control from real-time graphics.  
> Featuring 10 high-power outputs including precision PID solid-state relay control, 8 optocoupled industrial inputs, dual K-type thermocouples, RS485 Modbus, and built-in Wi-Fi telemetry with embedded web server access.  
> Encased in a rugged industrial shell with a brilliant 3.5-inch anti-glare capacitive touch display.  
> Engineered for zero downtime. Designed for absolute control.  
> **Haffler Automation — The Heart of Smart Machinery.**"*
