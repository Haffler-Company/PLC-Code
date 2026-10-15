# نظام‌نامه و مستندات فنی موتور به‌روزرسانی آنلاین فوق‌سریع هافلر (Haffler High-Speed Zero-Lag OTA Engine)
## تحلیل ریشه‌ای، اسرار مهندسی و راهنمای پیاده‌سازی روی تمام ۱۱ ماشین صنعتی

> **سند مرجع مهندسی (Architectural Blueprint)**  
> **مخاطب:** کلیه مهندسان سیستم، توسعه‌دهندگان فرمور و ایجنت‌های هوش مصنوعی فعال در پروژه‌های هافلر  
> **هدف:** انتقال ۱۰۰٪ دانش فنی، الگوهای بدون باگ، و معماری استریم با حداکثر پهنای باند و بدون سکته

---

## ۱. کالبدشکافی ریشه‌ای سه چالش مرگبار در OTA امبدد (Root-Cause Autopsy)

در فرآیند توسعه و تیونینگ OTA روی کنترلر صنعتی ESP32-S3، سه چالش زنجیره‌ای و بسیار بحرانی شناسایی و ریشه‌یابی شدند:

### چالش اول: هنگی و تاخیر ۴ ثانیه‌ای اولیه وب‌پورتال (The 4-Second UI Freeze)
* **علت فنی:** در پیاده‌سازی‌های سنتی، به محض فشردن کلید «شروع به‌روزرسانی»، تابع پاک‌سازی کل پارتیشن فلش (`esp_partition_erase_range` به حجم ۳.۵ مگابایت) به صورت همگام در هندلر درخواست وب (`httpd`) روی همان هسته (Core 0) اجرا می‌شد. پاک‌سازی ۳.۵ مگابایت فلش حدود ۳ تا ۴ ثانیه زمان می‌برد. در این مدت:
  1. ترد وب‌سرور مسدود می‌شد و مرورگر هیچ پاسخی دریافت نمی‌کرد (لگ هنگی صفحه).
  2. مرورگر کاربر درخواست را Timeout در نظر گرفته و به صورت خودکار درخواست را تکرار می‌کرد.
* **علت دوم:** کش شدن کدهای HTML/JS وب‌پورتال در مرورگر به علت هدرهای غیراستاندارد ETag/Cache-Control که باعث اجرای نسخه‌های قدیمی جاوااسکریپت در مرورگر می‌شد.

### چالش دوم: دانلود دوگانه فریمور (The Double Download Cycle)
* **علت فنی:** کلاینت جاوااسکریپت از دو چرخه پولینگ متداخل `setInterval` استفاده می‌کرد (یکی در `poll()` کلی و دیگری در مودال OTA). هنگامی که درخواست به علت تاخیر سوکت بیش از ۱ ثانیه طول می‌کشید، درخواست بعدی قبل از پایان درخواست قبلی ارسال می‌شد (`Overlapping Fetch Requests`). وب‌سرور تحت فشار قرار گرفته و در برخی شرایط، دستور آغاز دانلود مجدداً به سیستم ارسال می‌شد یا فرآیند دو بار تکرار می‌گردید.

### چالش سوم: سکته‌های متوالی و ایستادن دانلود (The JIT Flash-Erase Stutter)
* **علت فنی (راز کلیدی و طلایی سیستم):**
  هنگامی که دانلود با حذف اریز اولیه ۴ ثانیه‌ای آغاز شد، اریز به صورت «در حین پرواز» (Just-In-Time) به ازای هر ۶۴KB اعمال شد.
  اما در سخت‌افزار ESP32:
  1. کنترلر فلش SPI در حین اجرای پاک‌سازی یک بلوک ۶۴KB، به مدت **۵۰ تا ۷۰ میلی‌ثانیه کل گذرگاه سخت‌افزاری SPI را قفل می‌کند** و کش پردازنده متوقف می‌شود.
  2. در همین لحظه، داده‌های شبکه وای‌فای از طریق امواج رادیویی به بافر کارت شبکه می‌رسند.
  3. به دلیل قفل بودن پردازنده و وقفه در پردازش سوکت lwIP، بافرهای TCP کارت شبکه پر شده و بسته‌ها دور ریخته می‌شوند (`Packet Drop`).
  4. سرور متوجه عدم دریافت ACK در زمان مناسب شده و الگوریتم کنترل ازدحام (`TCP Congestion Control`) اندازه پنجره (`cwnd`) را به ۱ پکت کاهش می‌دهد و وارد فاز تاخیر نمایی (`Exponential Backoff`) می‌شود.
  5. نتیجه در ظاهر این بود: **۶۴ کیلوبایت با سرعت دانلود می‌شد، سپس سیستم ۳۰ ثانیه یا چند ثانیه سکته می‌کرد، دوباره کمی جلو می‌رفت، دوباره سکته می‌کرد!**

---

## ۲. ارکان پنج‌گانه معماری طلایی هافلر (The 5 Golden Pillars)

برای دستیابی به سرعت دانلود ۲ مگابایت بر ثانیه (تکمیل ۳.۵ مگابایت فریمور در کمتر از ۲.۵ ثانیه) بدون حتی ۱ میلی‌ثانیه لگ یا سکته، معماری زیر پیاده‌سازی و تثبیت شد:

```
+-----------------------------------------------------------------------------------+
|                           Haffler High-Speed OTA Engine                           |
+-----------------------------------------------------------------------------------+
|  1. Early Background Pre-Erase (Boot 5s / WiFi Got-IP)                           |
|     --> 3.5MB passive partition is 100% erased before user clicks anything        |
+-----------------------------------------------------------------------------------+
|  2. Strict Zero-Erase During Active Socket (Clean Flash Guarantee)                |
|     --> Ensure remaining blocks erased BEFORE opening HTTP client                 |
+-----------------------------------------------------------------------------------+
|  3. ESP-IDF need_erase Bypass                                                     |
|     --> esp_ota_begin(part, 4096, &h) sets need_erase=false                       |
+-----------------------------------------------------------------------------------+
|  4. Hardware Transport & Socket Buffering                                         |
|     --> WIFI_PS_NONE (20dBm) + TCP_NODELAY + 32KB SO_RCVBUF + 8KB chunk write     |
+-----------------------------------------------------------------------------------+
|  5. Non-Blocking Single-In-Flight Web Architecture                               |
|     --> Instant 200 OK + lock guard + dynamic no-cache ETag                       |
+-----------------------------------------------------------------------------------+
```

### ۱. پاک‌سازی پیش‌گسترانه پس‌زمینه (Autonomous Background Pre-Erase)
* پاک‌سازی پارتیشن غیرفعال (`ota_0` یا `ota_1`) هیچ وابستگی به اتصال وای‌فای یا شبکه ندارد زیرا یک عملیات داخلی فلش است.
* بنابراین در **ثانیه ۵ پس از بوت** (زمانی که رندر اولیه LVGL و خواندن NVS کامل شده) و همچنین در لحظه اتصال به مودم (`IP_EVENT_STA_GOT_IP`)، تسک `ota_pre_erase_worker` روی هسته ۰ با اولویت پایین (Priority 1) فعال می‌شود.
* این تسک به ازای هر بلوک ۶۴KB، به میزان ۱۵ میلی‌ثانیه پردازنده را آزاد می‌کند (`vTaskDelay(15)`). هسته ۱ با فریم‌ریت پایدار ۶۰FPS به رندرینگ UI ادامه می‌دهد.
* کل پارتیشن ۳.۵ مگابایتی ظرف حدود ۳.۳ ثانیه در پس‌زمینه پاک شده و فلگ `s_pre_erase_done = true` تنظیم می‌شود.

### ۲. قانون مطلق: عدم اجرای اریز در حین باز بودن سوکت (Never Erase With Open Socket)
* اگر به هر دلیلی کاربر بلافاصله پس از بوت دکمه به‌روزرسانی را زد و هنوز بخشی از پارتیشن پاک نشده بود:
  **هرگز نباید سوکت HTTP باز شود تا زمانی که اریز ۱۰۰٪ کامل شود.**
* در `ota_download_worker_task`، ابتدا وضعیت UI به `OTA_INST_CONNECTING` تغییر کرده و پیام «در حال اتصال...» به کاربر نمایش داده می‌شود.
* سپس در همان حالت، مابقی بلوک‌ها بدون تاخیر اضافی پاک می‌شوند (حداکثر ۱ الی ۱.۵ ثانیه).
* **سپس و تنها سپس** تابع `esp_http_client_init` و `esp_http_client_open` فراخوانی می‌شود.
* با این روش، هنگامی که جریان TCP برقرار می‌شود، حافظه فلش ۱۰۰٪ سفید و آماده رایت مستقیم است.

### ۳. دور زدن هوشمند موتور اریز داخلی فریم‌ورک ESP-IDF
* به طور پیش‌فرض، تابع `esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &handle)` تمام پارتیشن را به صورت مسدودکننده در حین باز بودن سوکت پاک می‌کند!
* با فراخوانی:
  ```c
  esp_err_t ota_err = esp_ota_begin(update_partition, 4096, &ota_handle);
  ```
  مقدار `image_size = 4096` باعث می‌شود درایور ESP-IDF فلگ داخلی `need_erase` را برابر `false` قرار دهد. بدین ترتیب، توابع بعدی `esp_ota_write` مستقیماً داده‌ها را روی فلش می‌نویسند و هرگز تابع اریز را صدا نمی‌زنند.

### ۴. بهینه‌سازی لایه انتقال، رادیو و رایت (Transport Layer Acceleration)
* **رادیو وای‌فای:** غیرفعال‌سازی حالت خواب رادیو (`esp_wifi_set_ps(WIFI_PS_NONE)`) و تنظیم توان روی ۲۰dBm (`esp_wifi_set_max_tx_power(80)`).
* **تنظیمات سوکت:**
  ```c
  int nodelay = 1;
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
  int rcvbuf = 32768;
  setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
  ```
* **تجمیع بافر به ۸KB:** تجمیع پکت‌های دریافتی در بافر ۱۶KB تا رسیدن به حداقل ۸۱۹۲ بایت، سپس رایت یکباره روی فلش (`esp_ota_write`). این کار تعداد عملیات رایت فلش را به نصف کاهش داده و زمان رایت هر بلاک ۸KB تنها حدود ۳ میلی‌ثانیه است.

### ۵. وب‌پورتال تک‌پروازی و هدرهای ضد کش (WebPortal Single In-Flight Poller)
* در سرور: متد POST شروع دانلود بلافاصله پاسخ `{"ok":true}` برمی‌گرداند تا سوکت وب آزاد شود.
* در جاوااسکریپت: متغیر قفل `s_ota_is_fetching` تضمین می‌کند تا زمانی که پاسخ درخواست قبلی دریافت نشده، هیچ درخواست جدیدی به سرور ارسال نشود.
* هدرهای ضد کش شامل `ETag: "astiaj-vXXX"`, `Cache-Control: no-store, no-cache, must-revalidate, max-age=0` و `Connection: close` برای بستن سریع نشست‌های TCP پس از پاسخ.

---

## ۳. الگوهای آماده کد جهت استفاده در سایر پروژه‌ها (Copy-Paste Ready Implementation)

### الف) الگوی پاک‌سازی پس‌زمینه (`ota_update.c`)
```c
static volatile bool s_pre_erase_done = false;
static volatile bool s_pre_erase_running = false;
static volatile uint32_t s_erased_upto_offset = 0;
static TaskHandle_t s_pre_erase_task_handle = NULL;

static void ota_pre_erase_worker(void *arg)
{
    (void)arg;
    s_pre_erase_running = true;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition) {
        const uint32_t BLOCK = 65536u;
        uint32_t erase_size = 0x380000u; // حداقل ۳.۵ مگابایت
        if (erase_size > update_partition->size) erase_size = update_partition->size;

        for (uint32_t offset = s_erased_upto_offset; offset < erase_size; offset += BLOCK) {
            if (!s_pre_erase_running) break;
            uint32_t len = ((offset + BLOCK) <= erase_size) ? BLOCK : (erase_size - offset);
            esp_err_t er = esp_partition_erase_range(update_partition, offset, len);
            if (er != ESP_OK) break;
            s_erased_upto_offset = offset + len;
            vTaskDelay(pdMS_TO_TICKS(15)); // واگذاری پردازنده برای حفظ ۶۰FPS روی هسته ۱
        }
        if (s_pre_erase_running && s_erased_upto_offset >= erase_size) {
            s_pre_erase_done = true;
        }
    }
    s_pre_erase_running = false;
    s_pre_erase_task_handle = NULL;
    vTaskDelete(NULL);
}

void ota_pre_erase_start(void)
{
    if (s_pre_erase_done || s_pre_erase_running || s_install_task_handle != NULL) return;
    xTaskCreatePinnedToCore(ota_pre_erase_worker, "ota_pre_erase", 4096, NULL, 1, &s_pre_erase_task_handle, 0);
}
```

### ب) تضمین پاک بودن فلش پیش از اتصال به سوکت (`ota_download_worker_task`)
```c
    // توقف پیش‌پاک‌سازی اگر در حال اجراست
    if (s_pre_erase_running) {
        s_pre_erase_running = false;
        vTaskDelay(pdMS_TO_TICKS(40));
    }

    // پاک‌سازی باقیمانده حافظه قبل از باز کردن سوکت
    uint32_t req_erase = (((uint32_t)expected_size + 65535u) & ~65535u);
    if (req_erase < 0x380000u) req_erase = 0x380000u;
    if (req_erase > update_partition->size) req_erase = update_partition->size;

    if (s_erased_upto_offset < req_erase) {
        while (s_erased_upto_offset < req_erase) {
            if (s_cancel_requested) break;
            uint32_t len = ((s_erased_upto_offset + 65536u) <= req_erase) ? 65536u : (req_erase - s_erased_upto_offset);
            esp_partition_erase_range(update_partition, s_erased_upto_offset, len);
            s_erased_upto_offset += len;
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
    s_pre_erase_done = true;

    // آغاز OTA با need_erase=false
    esp_ota_handle_t ota_handle = 0;
    ESP_ERROR_CHECK(esp_ota_begin(update_partition, 4096, &ota_handle));

    // اکنون سوکت باز می‌شود...
    esp_http_client_init(&http_config);
    esp_http_client_open(client, 0);
```

### ج) نقاط اتصال ماشه پاک‌سازی (Trigger Points)
1. **در `main.c` (یک هرتز هوک - ثانیه ۵ پس از بوت):**
   ```c
   if (s_stable_sec == 5) {
       extern void ota_pre_erase_start(void);
       ota_pre_erase_start();
   }
   ```
2. **در `wifi.c` (لحظه دریافت IP):**
   ```c
   } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
       extern void ota_pre_erase_start(void);
       ota_pre_erase_start();
   }
   ```
3. **در هندلر اطلاعات نسخه (`/api/ota/info`):**
   ```c
   ota_pre_erase_start();
   ```

---

## ۴. چک‌لیست اعتبارسنجی پیش از تحویل در پروژه‌های بعدی (Verification Checklist)

هنگام پیاده‌سازی این سیستم روی هر ماشین جدید:
- [x] بافر دریافت سوکت (`SO_RCVBUF`) حداقل ۳۲KB باشد.
- [x] فلگ `TCP_NODELAY` روی سوکت فعال باشد.
- [x] پارتیشن هدف حداقل ۳.۵ مگابایت باشد (`partitions.csv` دارای دو پارتیشن ۷ مگابایتی `ota_0` و `ota_1`).
- [x] مقدار `need_erase` با فرستادن `4096` به عنوان سایز ایمیج در `esp_ota_begin` غیرفعال شود.
- [x] هیچ تابع اریزی در داخل حلقه خواندن پکت‌های `esp_http_client_read` وجود نداشته باشد (مگر صرفاً فال‌بک ایمنی که هرگز اجرا نشود).
- [x] بستن سوکت شبکه (`esp_http_client_close` و `cleanup`) **قبل** از تایید هش و اتمام OTA (`esp_ota_end`) انجام پذیرد تا کلیه بافرهای رم شبکه آزاد شوند.
