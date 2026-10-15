# کتابچه ممیزی، عیب‌یابی و صنعتی‌سازی کد (Code Audit & Hardening Playbook)
## راهنمای گام‌به‌گام ایجنت‌های هوش مصنوعی برای کشف عیوب کدهای موجود و تبدیل آن‌ها به سیستم‌های فوق‌العاده مقاوم صنعتی

> **هدف این سند:** وقتی یک ایجنت هوش مصنوعی با قطعه کدی در پروژه‌های هافلر روبرو می‌شود، بتواند با یک نگاه عیوب پنهان، خطرات ایمنی، ناپایداری‌های زمان‌بندی و آسیب‌پذیری‌های آن را کشف کند و بلافاصله نسخه صنعتی، امن و پایدار آن را طبق استانداردهای زیمنس پیاده‌سازی نماید.

---

## ۱. چرخه ۷ مرحله‌ای ممیزی کد صنعتی (The 7-Step Audit Framework)

هنگام بررسی هر فایل سورس (`.c`, `.cpp`, `.ino`, `.h`)، ایجنت باید کدهای مربوطه را از فیلتر ۷ گیت ارزیابی زیر عبور دهد:

```
[گیت ۱: ایمنی سخت‌افزار و پین‌ها] ---> آیا قطع برق یا ریست باعث حرکت ناخواسته جک‌ها می‌شود؟
       |
[گیت ۲: تعینی بودن زمان (Timing)]  ---> آیا تابعی مانع اجرای سیکل بعدی (delay بلاک‌کننده) است؟
       |
[گیت ۳: سلامت داده بین دو هسته]    ---> آیا مسابقه داده (Race Condition) بین هسته ۰ و ۱ وجود دارد؟
       |
[گیت ۴: پایداری ماشین وضعیت (FSM)] ---> آیا هر استیت دارای تایم‌اوت و مسیر خروج شفاف است؟
       |
[گیت ۵: اعتبارسنجی حسگرها]        ---> اگر سیم سنسور قطع یا نویزی شود، سیستم چه می‌کند؟
       |
[گیت ۶: مدیریت حافظه و بافرها]    ---> آیا متغیر پویا (String/malloc) در چرخه بازتکرار وجود دارد؟
       |
[گیت ۷: عیب‌یابی و ثبت خطا]        ---> در صورت توقف، علت ریشه‌ای ثبت و به اپراتور گزارش می‌شود؟
```

---

## ۲. جدول خطاهای رایج آماتور در برابر راهکار صنعتی زیمنس

| شماره | خطای کد آماتور / غیرصنعتی | خطر فیزیکی در کارگاه | راهکار صنعتی زیمنس / هافلر |
|:---:|:---|:---|:---|
| **۱** | استفاده از `delay(1000)` برای ایجاد مکث بین دو جک | قفل شدن کل سیستم، از دست رفتن پدال استاپ و عدم پاسخ HMI | استفاده از تایمرهای شمارشی مبتنی بر `millis()` یا ساختار `TON` |
| **۲** | راه‌اندازی رله Active-LOW بعد از `pinMode(pin, OUTPUT)` | بوبین شیر به مدت ۲۰ تا ۱۰۰ میلی‌ثانیه وصل شده و جک شوک می‌زند | مقداردهی پین به `HIGH` پیش از تبدیل به `OUTPUT` |
| **۳** | عدم وجود تایم‌اوت در انتظار برای رسیدن میکروسوئیچ | گیر کردن نامحدود دستگاه اگر سیم میکروسوئیچ پاره شود | تعریف تایم‌اوت سخت‌گیرانه با ترایگر آلارم `TIMEOUT_ALARM` |
| **۴** | خواندن مستقیم مقدار آنالوگ یا پین دیجیتال بدون فیلتر | نویز موتور یا پمپ باعث تریپ بی‌مورد یا تکرار سیکل می‌شود | فیلتر دیبانس چندچرخه‌ای و میانگین متحرک وزنی (EMA) |
| **۵** | تغییر متغیر مشترک بین هسته‌ها بدون محافظت | خطای خواندن متغیر ناقص (Torn Read) و پرش به استیت غلط | استفاده از `portMUX_TYPE`، متغیرهای `atomic` یا صف پیام |
| **۶** | الحاق رشته‌ها با `String s = "Temp: " + t` در لوپ | نشت حافظه (Memory Leak) و کرش پردازنده پس از چند ساعت | استفاده از بافرهای ایستا `char buf[32]` و `snprintf` |
| **۷** | بررسی نکردن بازه سنسور دما قبل از فرمان به المنت | در صورت قطع سنسور، المنت مداوم روشن مانده و باعث آتش‌سوزی می‌شود | تست محدوده مجاز (`0 < temp < 300`) و قطع برق المنت در صورت خطا |
| **۸** | استفاده از فرمت `%02d:%02d` در برچسب‌های فارسی | کاراکتر دو نقطه خنثی بوده و جای دقیقه و ثانیه وارونه می‌شود | استفاده از عبارات صریح فارسی («X دقیقه و Y ثانیه») یا قفل یونیکد LRO |
| **۹** | کادر دیالوگ خطا با ارتفاع ناکافی و اسکرول زودهنگام | خط آخر روی خط کرسی کات شده و حروف فارسی شکسته دیده می‌شوند | ایجاد بافر ارتفاع +۲۰px، هدر ۳۶px و پدینگ سطری ۴px |
| **۱۰** | فراخوانی مودال‌های UI از هسته ۰ بدون قفل | تداخل همروندی با تسک رندرینگ هسته ۱ و فریز احتمالی | الزام به اخذ قفل بازگشتی `if (!lvgl_lock(100)) return;` |
| **۱۱** | کلمپ استپرهای با مقادیر پیش‌فرض بزرگ به ۹۹۹ | مخدوش شدن مقادیر کارخانه (مانند ۷۲۰۰s) با اولین لمس کلید | ارتقای کران بالای استپر متناسب با بیشینه مهندسی (۸۶۴۰۰s) |
| **۱۲** | ناهمخوانی واحدها بین وب‌پورتال و نمایشگر فیزیکی | سردرگمی اپراتور کارگاهی و خطای تنظیمات فرآیند | منبع واحد تعاریف و اعتبارسنجی استاتیک تطابق واحدها |

---

## ۳. دستورالعمل‌های عملیاتی تبدیل کدهای خام به کدهای صنعتی (Refactoring Recipes)

### ۳.۱. اصلاح تاخیر بلاک‌کننده (`delay`) به تایمر غیربلاک‌کننده
```c
// ❌ نسخه غلط و خطرناک:
void step_clamp_down() {
    digitalWrite(PIN_CLAMP, LOW); // فعال شدن جک نگهدارنده
    delay(2000);                  // قفل کامل سیستم به مدت ۲ ثانیه! E-Stop خوانده نمی‌شود!
    digitalWrite(PIN_CUTTER, LOW);// خروج تیغه اره
}

// ✅ نسخه صنعتی زیمنس (FSM با تایمر غیربلاک‌کننده):
typedef enum {
    STATE_IDLE,
    STATE_CLAMPING,
    STATE_CUTTING,
    STATE_FAULT
} cut_state_t;

static cut_state_t s_state = STATE_IDLE;
static uint32_t s_state_start_ms = 0;

void run_cycle_step() {
    uint32_t now = millis();
    
    switch(s_state) {
        case STATE_IDLE:
            if (cmd_start_pressed && safety_interlocks_ok()) {
                hal_relay_set(PIN_CLAMP, true);
                s_state_start_ms = now;
                s_state = STATE_CLAMPING;
            }
            break;

        case STATE_CLAMPING:
            // بررسی میکروسوئیچ اتمام کورس جک کلمپ
            if (hal_sensor_read(PIN_CLAMP_DOWN_LIMIT)) {
                hal_relay_set(PIN_CUTTER, true);
                s_state_start_ms = now;
                s_state = STATE_CUTTING;
            } 
            // تایم‌اوت محافظتی: اگر جک در ۲ ثانیه نرسید، توقف و آلارم
            else if (now - s_state_start_ms >= 2000) {
                hal_relay_set(PIN_CLAMP, false);
                trigger_alarm(ALARM_CLAMP_TIMEOUT);
                s_state = STATE_FAULT;
            }
            break;

        case STATE_CUTTING:
            // ادامه منطق برش ...
            break;

        case STATE_FAULT:
            // قطع تمام رله‌ها در حالت خطا
            hal_emergency_shutdown_all();
            break;
    }
}
```

---

### ۳.۲. اصلاح راه‌اندازی رله‌های تحریک منفی (Active-LOW Safe Boot)
```c
// ❌ نسخه غلط (ایجاد پرش لحظه‌ای روی شیرها):
void init_relays_wrong() {
    pinMode(RELAY_VALVE_1, OUTPUT); // در این لحظه پین LOW است و شیر باز می‌شود!
    digitalWrite(RELAY_VALVE_1, HIGH); // پس از چند میکروثانیه بسته می‌شود!
}

// ✅ نسخه صنعتی زیمنس (خاموشی تضمین‌شده در زمان بوت):
void init_relays_industrial() {
    // ۱. ابتدا نوشتن مقدار HIGH روی ثبات پین
    gpio_set_level((gpio_num_t)RELAY_VALVE_1, 1);
    
    // ۲. سپس تعیین جهت به عنوان خروجی
    gpio_set_direction((gpio_num_t)RELAY_VALVE_1, GPIO_MODE_OUTPUT);
    
    // ۳. ثبت وضعیت مطمئن در ساختار سیستم
    g_relay_state[RELAY_VALVE_1] = false; // خاموش
}
```

---

### ۳.۳. همگام‌سازی متغیرهای اشتراکی میان Core 0 (HMI) و Core 1 (PLC)
```c
// ❌ نسخه غلط (Race Condition و خواندن داده مخدوش):
float g_target_cut_length = 1250.5; // متغیر بدون محافظت در هر دو هسته تغییر می‌کند

// ✅ نسخه صنعتی زیمنس (با Spinlock یا Mutex):
static portMUX_TYPE s_config_mux = portMUX_INITIALIZER_UNLOCKED;
static float s_target_cut_length = 1250.5;

void plc_set_target_length(float new_len) {
    portENTER_CRITICAL(&s_config_mux);
    s_target_cut_length = new_len;
    portEXIT_CRITICAL(&s_config_mux);
}

float plc_get_target_length() {
    float val;
    portENTER_CRITICAL(&s_config_mux);
    val = s_target_cut_length;
    portEXIT_CRITICAL(&s_config_mux);
    return val;
}
```

---

### ۳.۴. ایمنی لحظه روشن شدن (Boot Failsafe) و حل باگ رله چتر
```c
// ❌ نسخه غلط (پین شناور یا تحریک ناخواسته در بوت):
void outputs_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_1),
        .mode = GPIO_MODE_OUTPUT, // پین خروجی می‌شود در حالی که سطح آن هنوز تعریف نشده!
    };
    gpio_config(&io_conf);
    gpio_set_level(RELAY_1, 1); // رله برای چند میلی‌ثانیه وصل شده و جک شوک می‌زند!
}

// ✅ نسخه صنعتی و مقاوم هافلر:
void outputs_init(void) {
    // ۱. ابتدا پیش‌تنظیم صلب سطح خروجی‌ها پیش از هرگونه پیکربندی
    gpio_set_level(RELAY_1, 1); // Active-LOW: 1 = OFF
    gpio_set_level(BUZZER_PIN, 0); // 0 = Silent

    // ۲. پیکربندی با مود دوطرفه و مقاومت Pull-Up داخلی فعال
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_1),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}
```

---

### ۳.۵. دیاگ سخت‌افزاری خروجی‌ها با ماسک نرم‌افزاری وضعیت
```c
// ❌ نسخه غلط (خواندن مستقیم پایه خروجی با شکست مواجه می‌شود):
void toggle_relay_bad(gpio_num_t pin) {
    int lvl = gpio_get_level(pin); // در حالت OUTPUT همیشه 0 برمی‌گرداند!
    gpio_set_level(pin, !lvl);     // رله هرگز تاگل نمی‌شود!
}

// ✅ نسخه صنعتی هافلر با ماسک نرم‌افزاری:
static uint64_t s_active_relay_mask = 0;

void toggle_relay_good(gpio_num_t pin) {
    if (machine_is_running()) return; // اینترلاک ایمنی فرآیند جاری

    bool was_on = (s_active_relay_mask & (1ULL << pin)) != 0;
    bool now_on = !was_on;

    if (now_on) {
        s_active_relay_mask |= (1ULL << pin);
        gpio_set_level(pin, 0); // Active-LOW: ON
    } else {
        s_active_relay_mask &= ~(1ULL << pin);
        gpio_set_level(pin, 1); // Active-LOW: OFF
    }
}
```

---

### ۳.۶. محاسبه تقویم شمسی جلالی در اقساط بدون مصرف DRAM
```c
// ✅ الگوریتم محاسباتی سریع میلادی به جلالی هافلر:
void gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd) {
    static const int g_d_m[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int gy2 = (gm > 2) ? (gy + 1) : gy;
    long g_day_no = 365L * (long)gy + (long)((gy2 + 3) / 4) - (long)((gy2 + 99) / 100)
                    + (long)((gy2 + 399) / 400) - 80L + (long)gd + (long)g_d_m[gm - 1];
    long j_np = g_day_no / 12053L;
    g_day_no %= 12053L;
    long jy_calc = 979L + 33L * j_np + 4L * (g_day_no / 1461L);
    g_day_no %= 1461L;
    if (g_day_no >= 366L) {
        jy_calc += (g_day_no - 1L) / 365L;
        g_day_no = (g_day_no - 1L) % 365L;
    }
    int jm_calc = 0, jd_calc = 0;
    for (int i = 0; i < 11 && g_day_no >= (long)((i < 6) ? 31 : 30); ++i) {
        g_day_no -= (long)((i < 6) ? 31 : 30);
        jm_calc++;
    }
    *jy = (int)jy_calc;
    *jm = jm_calc + 1;
    *jd = (int)g_day_no + 1;
}
```

---

## ۴. چک‌لیست پذیرش برای ارتقای کدها (Acceptance Criteria)

پیش از آن‌که اعلام شود اصلاح یک قطعه کد صنعتی به پایان رسیده است، موارد زیر باید محقق شده باشند:

1. [ ] هیچ فراخوانی مستقیمی به `delay()` در مسیرهای بلادرنگ وجود نداشته باشد.
2. [ ] تمام رله‌ها در خط صفر `outputs_init()` با منطق تحریک منفی بدون اسپایک فعال‌سازی شوند.
3. [ ] در دیاگ خروجی‌ها، وضعیت رله‌ها با ماسک نرم‌افزاری کنترل شده و اینترلاک کارکرد ماشین فعال باشد.
4. [ ] در کارت‌های استپر افقی، فاصله تنفس ۱۶px از دکمه `+` و تراز ریاضی کانتینر پدینگ صفر رعایت شود.
5. [ ] هر جک مکانیکی و المنت حرارتی دارای تایم‌اوت سخت‌افزاری/نرم‌افزاری مستقل باشد.
6. [ ] داده‌های ورودی حسگرها حداقل ۳ چرخه متوالی اعتبارسنجی شده باشند.
7. [ ] استاپ اضطراری به عنوان اولویت اول همواره تمام خروجی‌ها را ظرف کمتر از ۲۰ms قطع کند.
8. [ ] رویدادهای بحرانی و خطاهای سیستم در حافظه غیرفرار همراه با ساعت دقیق ثبت شوند.
9. [ ] وب‌سرور HMI و پورت‌های تله‌متری کلود از فیلتر تست نفوذ خودکار Strix AI و گاردریل‌های IEC 62443 عبور کرده باشند (رجوع به [STANDARDS_STRIX_AI_OT_PENTESTING_AND_IEC62443.md](file:///C:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_STRIX_AI_OT_PENTESTING_AND_IEC62443.md)).
