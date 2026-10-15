# راهنمای استانداردهای ایمنی کدنویسی MISRA C در کنترلرهای صنعتی
## قوانین طلایی مهندسی نرم‌افزار تعبیه‌شده جهت تحقق «احتمال صفر درصد خطا» در فریمورهای C/C++ و FreeRTOS

> **هدف:** اجرای سخت‌گیرانه‌ترین استانداردهای بین‌المللی ایمنی نرم‌افزار (برگرفته از MISRA C:2012 و IEC 61508 SIL-3) روی پردازنده‌های صنعتی ESP32-S3، تا کدهای نوشته شده توسط ایجنت‌ها در برابر سرریز بافر، مسابقه داده‌ها، اشاره‌گرهای معلق، سرریز محاسباتی و قفل شدن حلقه‌ها ۱۰۰٪ مصون باشند.

---

## ۱. جدول انواع داده‌های استاندارد (Explicit Data Types)

استفاده از کلمات کلیدی استاندارد اما مبهم C نظیر `int`, `short`, `long`, `char` یا `unsigned` به تنهایی در تمام پروژه‌ها **مطلقا ممنوع** است، زیرا اندازه بیتی آن‌ها بین کامپایلرهای مختلف (مثلاً Xtensa GCC در برابر x86 Host Sim) متغیر است.

```c
// ❌ تعاریف ممنوعه:
int counter;
unsigned long timestamp;
char buffer[64];

// ✅ تعاریف استاندارد MISRA C (<stdint.h> و <stdbool.h>):
uint8_t   u8_sensor_mask;      // دقیقاً ۸ بیت بدون علامت (0 تا 255)
int16_t   i16_temperature_raw; // دقیقاً ۱۶ بیت علامت‌دار (-32768 تا +32767)
uint32_t  u32_pulse_count;     // دقیقاً ۳۲ بیت بدون علامت (0 تا 4294967295)
float32_t f32_target_position; // اعشاری استاندارد ۳۲ بیتی
bool      b_is_clamped;        // متغیر بولی صریح (true / false)
```

---

## ۲. ایمنی اشاره‌گرها و جلوگیری از دسترسی نامعتبر (Pointer Safety & Lifecycles)

اشاره‌گرهای مخدوش و دسترسی به حافظه آزاد شده عامل ۹۰٪ کرش‌های صنعتی هستند.

1. **اعتبارسنجی اجباری مقادیر ورودی (NULL Guarding):**  
   هیچ تابعی نباید بدون اعتبارسنجی اشاره‌گرهای دریافتی، آن‌ها را دریفنس (Dereference) کند:
   ```c
   // الگوی ایمن MISRA C:
   esp_err_t hal_sensor_read_status(const sensor_dev_t* dev, bool* out_state) 
   {
       // ۱. گیت ایمنی اعتبارسنجی
       if (dev == NULL || out_state == NULL) {
           return ESP_ERR_INVALID_ARG;
       }
       
       // ۲. عملیات ایمن
       *out_state = gpio_get_level(dev->gpio_pin) == 0;
       return ESP_OK;
   }
   ```
2. **پوچ‌سازی فوری اشاره‌گرها پس از آزادسازی (Dangling Pointer Immunity):**  
   اگر اشاره‌گری در طول راه‌اندازی آزاد شد، بلافاصله باید مقدار `NULL` بگیرد:
   ```c
   free(ptr);
   ptr = NULL;
   ```
3. **استفاده اجباری از کلمه کلیدی `const`:**  
   هر متغیری که نباید توسط یک تابع دستکاری شود، باید صریحاً با `const` محافظت گردد تا کامپایلر مانع تغییر ناخواسته آن شود.

---

## ۳. حلقه‌های کران‌دار و ایمنی از قفل شدن (Bounded Loops & Lockup Prevention)

در استانداردهای صنعتی هوایی و خودرویی، هیچ حلقه‌ای نباید پتانسیل اجرای نامحدود داشته باشد.

```c
// ❌ کد خطرناک (اگر سنسور قطع شود، MCU تا ابد اینجا قفل می‌شود):
while (gpio_get_level(PIN_CYLINDER_LIMIT) != 0) {
    // انتظار نامحدود!
}

// ✅ حلقه کران‌دار صنعتی با سقف تکرار سخت‌گیرانه (MISRA Compliant):
#define MAX_LIMIT_POLL_CYCLES 200000UL

uint32_t poll_guard = 0;
bool limit_reached = false;

while (poll_guard < MAX_LIMIT_POLL_CYCLES) {
    if (gpio_get_level(PIN_CYLINDER_LIMIT) == 0) {
        limit_reached = true;
        break;
    }
    poll_guard++;
}

if (!limit_reached) {
    // خطای سخت‌افزاری صادر شود و جک متوقف گردد
    report_critical_fault(FAULT_CYLINDER_SENSOR_TIMEOUT);
}
```

---

## ۴. مدیریت خطای بازگشتی (Mandatory Error Propagation)

هیچ تابعی که با سخت‌افزار، حافظه، شبکه یا ارتباط سریال سروکار دارد، نباید مقدار بازگشتی `void` داشته باشد.

```c
// تمام توابع سیستمی باید کد وضعیت برگردانند:
typedef enum {
    SYS_OK = 0,
    SYS_ERR_INVALID_PARAM,
    SYS_ERR_TIMEOUT,
    SYS_ERR_HARDWARE_FAULT,
    SYS_ERR_BUFFER_OVERFLOW
} sys_status_t;

// صدا زننده ملزم به چک کردن وضعیت است:
sys_status_t status = eeprom_write_calibration(&g_calib_data);
if (status != SYS_OK) {
    alarm_trigger_nvs_failure(status);
    // ورود به مد اضطراری امن
}
```

---

## ۵. سرریز بافر و ایمنی رشته‌ها (Buffer Overflow Immunity)

استفاده از توابع ناامن کتابخانه استاندارد C نظیر `sprintf`, `strcpy`, `strcat`, `gets` در کل مخزن **مطلقا ممنوع و رد صلاحیت‌کننده است**.

```c
// ❌ توابع ممنوعه:
char msg[32];
sprintf(msg, "Current Value: %f and status %s", val, str); // ریسک سرریز شدید!

// ✅ توابع ایمن با تعیین صریح سقف بافر:
char msg[32];
snprintf(msg, sizeof(msg), "Val: %.2f", val); // تضمین عدم خروج از ۳۲ بایت
```

---

## ۶. ثبات‌های متغیر و تداخل وقفه/هسته‌ها (`volatile` and Memory Ordering)

متغیرهایی که در روتین‌های وقفه (ISR) یا تسک‌های هسته متقابل تغییر می‌کنند، باید با `volatile` مشخص شوند تا بهینه‌ساز کامپایلر آن‌ها را در رجیستر کش نکند:

```c
// متغیری که توسط سنسور انکودر در ISR افزایش می‌یابد:
static volatile uint32_t s_u32_encoder_ticks = 0;

// خواندن اتمیک در تسک اصلی:
uint32_t get_encoder_ticks_atomic(void) {
    uint32_t ticks;
    portENTER_CRITICAL(&s_encoder_mux);
    ticks = s_u32_encoder_ticks;
    portEXIT_CRITICAL(&s_encoder_mux);
    return ticks;
}
```

---

## ۷. فلگ‌های کامپایلر برای صفر کردن احتمال خطا (Compiler Warning Enforcements)

در فایل‌های `platformio.ini` و `CMakeLists.txt`، فلگ‌های سخت‌گیرانه زیر فعال می‌شوند تا کامپایلر هیچ هشداری را نادیده نگیرد:

```ini
build_flags = 
    -Wall                   ; فعال‌سازی تمام اخطارهای استاندارد
    -Wextra                 ; فعال‌سازی هشدارهای پیشرفته کیفیت کد
    -Werror=return-type     ; خطای قطعی اگر تابعی بدون مقدار بازگشتی پایان یابد
    -Werror=uninitialized   ; خطای قطعی برای متغیرهای مقداردهی‌نشده
    -Wshadow                ; هشدار اگر نام متغیر محلی با متغیر سراسری هم‌پوشانی داشته باشد
    -Wpointer-arith         ; جلوگیری از محاسبات ریاضی نامعتبر روی اشاره‌گرها
```
