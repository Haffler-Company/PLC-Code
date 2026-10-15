# Haffler Master Cross-Project Pattern Library
## کتابخانه الگوهای طلایی و اثبات‌شده ۱۱ ماشین هافلر (Canonical Code Patterns)

> **AGENT MANDATE:** Do NOT reinvent the wheel. Every pattern in this library has undergone hundreds of workshop hours, hardware vibration testing, and electrical noise validation.
> When implementing or porting features across Haffler machines, use these EXACT canonical implementations.

---

## ۱. الگوی درایور و بازیابی ضد انجماد تاچ خازنی GT911 (GT911 Anti-Freeze & Recovery)
* **پروژه‌های مرجع:** `ICE CREEAM/main/` و `bakery-mixer/main/`
* **مسئله:** نویز سوئیچینگ بارهای سلفی پرقدرت (کمپرسور، کنتاکتور، موتور) وضعیت I2C را در تاچ GT911 معلق کرده و چیپ تاچ فریز می‌شود.
* **الگوی استاندارد خواندن و ریست پرچم (هر سیکل خواندن):**

```c
// در هر سیکل نظارت لمسی، حتماً رجیستر وضعیت 0x814E را صفر کنید
static esp_err_t gt911_clear_status_reg(i2c_port_t port) {
    uint8_t clear_cmd[3] = {0x81, 0x4E, 0x00};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, clear_cmd, sizeof(clear_cmd), true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    return ret;
}
```

* **الگوی بازیابی اضطراری باس I2C با ۹ پالس کلاک (I2C Bus Recovery Sequence):**
```c
void haffler_i2c_bus_recover(gpio_num_t scl_pin, gpio_num_t sda_pin, gpio_num_t rst_pin) {
    // 1. تغییر موقت مود پین‌ها به GPIO عمومی با مقاومت بالاکش
    gpio_set_direction(scl_pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(sda_pin, GPIO_MODE_INPUT);
    
    // 2. اعمال ۹ پالس کلاک برای آزادسازی خط SDA توسط اسلیو گیرکرده
    for (int i = 0; i < 9; i++) {
        gpio_set_level(scl_pin, 0);
        esp_rom_delay_us(5);
        gpio_set_level(scl_pin, 1);
        esp_rom_delay_us(5);
    }
    
    // 3. اعمال ریست سخت‌افزاری به چیپ تاچ
    if (rst_pin != GPIO_NUM_NC) {
        gpio_set_direction(rst_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(15));
        gpio_set_level(rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

---

## ۲. الگوی ایمنی راه‌اندازی رله‌ها و پیشگیری از استارت ناخواسته (Relay Boot-Safe Init)
* **پروژه‌های مرجع:** تمام ۱۱ ماشین صنعتی هافلر
* **منطق:** خروجی‌های رله صنعتی هافلر Active-LOW هستند (سطح ۰ = رله روشن، سطح ۱ = رله خاموش).
* **خط قرمز ایمنی:** اگر جهت پین خروجی قبل از سطح منطقی تنظیم شود، پین در حالت شناور یا صفر قرار گرفته و تیغه/موتور/جک یک لحظه ضربه می‌زند.
* **الگوی استاندارد مقداردهی ایمن:**

```c
void outputs_init_safe(void) {
    const gpio_num_t relay_pins[] = {
        PIN_RELAY_Q0, PIN_RELAY_Q1, PIN_RELAY_Q2, 
        PIN_RELAY_Q3, PIN_RELAY_Q4, PIN_RELAY_Q5
    };
    
    for (size_t i = 0; i < sizeof(relay_pins)/sizeof(relay_pins[0]); i++) {
        // ۱. اول سطح ۱ منطقی (خاموش قطعی) اجبار می‌شود
        gpio_set_level(relay_pins[i], 1);
        // ۲. سپس مود خروجی با Pull-up فعال می‌گردد
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << relay_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        // ۳. اطمینان مجدد از خاموش بودن خروجی
        gpio_set_level(relay_pins[i], 1);
    }
}
```

---

## ۳. الگوی رندرینگ پرسرعت مبتنی بر تغییر (LVGL Zero-Lag Dirty-Driven UI)
* **پروژه‌های مرجع:** `bakery-mixer`, `ICE CREEAM`, `Welding_single_head`
* **مسئله:** اجرای بی قید و شرط `lv_label_set_text_fmt` و آپدیت ویجت‌ها در هر فریم، پهنای باند DMA ال‌سی‌دی را پر کرده و فریم‌ریت را به ۱۰ FPS کاهش می‌دهد.
* **الگوی استاندارد Dirty-Driven:**

```c
void tick_screen_production(void) {
    static int16_t s_last_temp = INT16_MIN;
    static uint32_t s_last_counter = UINT32_MAX;
    static uint8_t s_last_state = 0xFF;
    
    // ۱. بررسی دمای سنسور — فقط در صورت تغییر، ویجت بازطراحی می‌شود
    int16_t cur_temp = plc_get_temperature();
    if (cur_temp != s_last_temp) {
        s_last_temp = cur_temp;
        lv_label_set_text_fmt(ui_lbl_temp_val, "%d°C", cur_temp);
    }
    
    // ۲. بررسی شمارنده تولید
    uint32_t cur_counter = plc_get_cycle_count();
    if (cur_counter != s_last_counter) {
        s_last_counter = cur_counter;
        lv_label_set_text_fmt(ui_lbl_counter_val, "%lu", (unsigned long)cur_counter);
    }
    
    // ۳. بررسی تغییر وضعیت ماشین وضعیت
    uint8_t cur_state = plc_fsm_get_state();
    if (cur_state != s_last_state) {
        s_last_state = cur_state;
        update_state_pill_badge(cur_state);
    }
}
```

---

## ۴. الگوی قفل متقابل هسته‌ها در FreeRTOS (FreeRTOS Dual-Core LVGL Isolation)
* **پروژه‌های مرجع:** کلیه ۱۱ ماشین هافلر
* **هسته ۱:** تسک گرافیک و تاچ (`lv_task`)
* **هسته ۰:** تسک منطق تعینی ۵۰ms پی‌ال‌سی (`actions_task`) و وب‌پورتال
* **الگوی فراخوانی امن گرافیک از هسته کنترل (Core 0):**

```c
static SemaphoreHandle_t s_lvgl_mutex = NULL;

bool lvgl_lock(TickType_t timeout_ticks) {
    if (s_lvgl_mutex == NULL) return false;
    return (xSemaphoreTakeRecursive(s_lvgl_mutex, timeout_ticks) == pdTRUE);
}

void lvgl_unlock(void) {
    if (s_lvgl_mutex != NULL) {
        xSemaphoreGiveRecursive(s_lvgl_mutex);
    }
}

// نحوه فراخوانی از هسته ۰ (Core 0):
void plc_notify_alarm_to_ui(uint16_t fault_code) {
    if (lvgl_lock(pdMS_TO_TICKS(50))) {
        ui_show_alarm_modal(fault_code);
        lvgl_unlock();
    } else {
        ESP_LOGW("HAFFLER_CORE", "LVGL lock timeout - GUI frame dropped safely");
    }
}
```

---

## ۵. الگوی سیستم سه‌زبانه پویا با تضمین صفر درصد مربع (Trilingual Zero-Tofu Engine)
* **پروژه‌های مرجع:** `main/ui/ui_lang.c` در کلیه پروژه‌های صنعتی
* **قانون:** هیچ رشته‌ای هاردکد نمی‌شود. متون ترکی با حروف استاندارد صنعتی اسکی سازگار با فونت یکان جایگزین می‌شوند.
* **الگوی جدول دیکشنری:**

```c
typedef enum {
    STR_START,
    STR_STOP,
    STR_TEMPERATURE,
    STR_CYLINDER,
    STR_FAULT_ESTOP,
    STR_TOKEN_COUNT
} lang_token_t;

static const char* const s_dict[STR_TOKEN_COUNT][3] = {
    [STR_START]        = {"شروع",        "START",       "BASLAT"},
    [STR_STOP]         = {"توقف",        "STOP",        "DURDUR"},
    [STR_TEMPERATURE]  = {"دما",         "TEMP",        "SICAKLIK"},
    [STR_CYLINDER]     = {"سیلندر",      "CYLINDER",    "SILINDIR"},
    [STR_FAULT_ESTOP]  = {"استپ اضطراری", "EMERGENCY",   "ACIL STOP"}
};

const char* ui_lang_get(lang_token_t token) {
    uint8_t current_lang = get_active_language_index(); // 0=FA, 1=EN, 2=TR
    if (token >= STR_TOKEN_COUNT || current_lang > 2) return "???";
    return s_dict[token][current_lang];
}
```

---

## ۶. الگوی اینترلاک لچ‌شده استپ اضطراری (ISO 13850 Safety Latch Pattern)
* **پروژه‌های مرجع:** استاندارد ماشین‌آلات برش و جوش و بستنی‌ساز هافلر
* **استاندارد:** پس از رها شدن فیزیکی شستی استپ اضطراری، دستگاه نباید خودبه‌خود روشن شود. باید منتظر فشردن شستی Reset یا Start توسط اپراتور بماند.
* **الگوی پیاده‌سازی FSM:**

```c
typedef struct {
    bool estop_input_active;
    bool estop_latched;
    bool reset_button_pressed;
} safety_circuit_t;

void safety_monitor_step(safety_circuit_t* sec) {
    sec->estop_input_active = (gpio_get_level(PIN_ESTOP) == 0); // Active-LOW
    
    if (sec->estop_input_active) {
        sec->estop_latched = true;
        plc_outputs_all_off_instant();
        plc_set_state(STATE_FAULT);
        plc_raise_alarm(FAULT_EMERGENCY_STOP);
    }
    
    // خروج از وضعیت خطا تنها در صورتی مجاز است که کلید آزاد شده و کلید ریست زده شود
    if (sec->estop_latched && !sec->estop_input_active) {
        if (sec->reset_button_pressed) {
            sec->estop_latched = false;
            plc_clear_alarm(FAULT_EMERGENCY_STOP);
            plc_set_state(STATE_IDLE);
        }
    }
}
```

---

## ۷. الگوی حفاظت ضدتکرار کمپرسور و موتور سنگین (Anti-Short-Cycle Timer)
* **پروژه‌های مرجع:** `ICE CREEAM/main/actions.c`
* **علت صنعتی:** برابری فشار گاز مبرد در سیکل تبرید حداقل ۱۸۰ ثانیه زمان نیاز دارد. استارت مجدد قبل از این زمان باعث سوختن سیم‌پیچ کمپرسور می‌شود.
* **الگوی پیاده‌سازی:**

```c
#define COMP_MIN_OFF_TIME_MS (180000ULL)
static uint64_t s_comp_last_stop_time_ms = 0;

bool comp_can_start(void) {
    uint64_t now = esp_timer_get_time() / 1000ULL;
    if (s_comp_last_stop_time_ms == 0) return true;
    return ((now - s_comp_last_stop_time_ms) >= COMP_MIN_OFF_TIME_MS);
}

void comp_mark_stopped(void) {
    s_comp_last_stop_time_ms = esp_timer_get_time() / 1000ULL;
}

uint32_t comp_get_remaining_lock_seconds(void) {
    uint64_t now = esp_timer_get_time() / 1000ULL;
    uint64_t elapsed = now - s_comp_last_stop_time_ms;
    if (elapsed >= COMP_MIN_OFF_TIME_MS) return 0;
    return (uint32_t)((COMP_MIN_OFF_TIME_MS - elapsed) / 1000ULL);
}
```

---

## ۸. الگوی ابرداده هشت‌بخشی انتشار سرور و OTA (`lastfile`)
* **استاندارد مرجع:** `docs/STANDARDS_HAFFLER_OTA_RELEASE_AND_ROLLUP_CHANGELOG.md`
* **فرمت قطعی:** هیچ کلیدی حذف یا تغییر نام داده نمی‌شود. بازر (`BZ`) مطلقاً در ورودی/خروجی‌ها درج نمی‌شود.

```ini
[company]
name=Haffler Industrial Automation
brand=HAFFLER
support=+98-912-000-0000

[version]
code=304
display=3.0.4
target_machine=ICE CREEAM

[binary]
filename=firmware.bin
size_bytes=1441792
sha256=a1b2c3d4e5f67890abcdef1234567890abcdef1234567890abcdef1234567890

[description_fa]
- اضافه شدن حفاظت هوشمند جریان موتور پارو
- بهینه‌سازی تاخیر ۱۸۰ ثانیه‌ای استارت کمپرسور
- رفع خطای افتادگی ارقام در صفحه کالیبراسیون

[description_en]
- Added intelligent paddle motor overload protection
- Optimized 180s anti-short-cycle compressor delay
- Fixed numeral baseline drop on calibration screen

[description_tr]
- Palet motoru asiri yuk korumasi eklendi
- 180s kompresor anti-dongu gecikmesi optimize edildi
- Kalibrasyon ekranindaki sayi kayma hatasi duzeltildi

[inputs]
I0=سنسور دور پولی (القایی)
I2=کلاچ عقب (میکروسوئیچ)
I4=اهرم تخلیه جلو
I6=استپ اضطراری قارچی
I7=شستی استارت پنل

[outputs]
Q0=کمپرسور مخزن شیر بالا
Q1=کمپرسور سیلندر انجماد
Q2=موتور پارو و همزن
Q3=پمپ تزریق کرم
Q4=پمپ ژله رنگی رینبو
Q5=شیر برقی هوای فشرده
M0=رله ریست مدار فرمان
```

---

## ۹. الگوی موتور فلش و استریم فوق‌سریع OTA بدون سکته (Haffler High-Speed OTA Engine)
* **پروژه‌های مرجع:** `ICE CREEAM`, `bakery-mixer`
* **مسئله:** لگ ۴ ثانیه‌ای وب‌پورتال در شروع، دانلود دوگانه فریم‌ور، و سکته‌های متوالی ناشی از قفل سخت‌افزاری گذرگاه SPI در حین دریافت بسته‌های شبکه TCP.
* **راز معماری:**
  1. پاک‌سازی پس‌زمینه ۳.۵ مگابایتی پارتیشن غیرفعال در ثانیه ۵ پس از بوت و لحظه اتصال به مودم (`IP_EVENT_STA_GOT_IP`).
  2. تضمین ۱۰۰٪ پاک بودن فلش پیش از اتصال به سوکت شبکه (`Never Erase While HTTP Socket Is Open`).
  3. غیرفعال‌سازی اریز خودکار با `esp_ota_begin(update_partition, 4096, &ota_handle)`.
  4. بافر سوکت ۳۲KB، فعال‌سازی `TCP_NODELAY`، توان رادیو `WIFI_PS_NONE`، و تجمیع داده‌ها در بافر ۸KB پیش از هر رایت فلش.
  5. پولینگ تک‌پروازی وب‌پورتال با قفل `s_ota_is_fetching` و هدرهای ضد کش ETag.
* **سند مرجع تفصیلی:** [HAFFLER_HIGH_SPEED_OTA_ENGINE_BLUEPRINT.md](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/HAFFLER_HIGH_SPEED_OTA_ENGINE_BLUEPRINT.md)
