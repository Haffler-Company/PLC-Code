#include "build_time.h"
#ifndef VERSION
#define VERSION "2.0.0"
#endif
#define OTA_BASE_URL "https://ota.haffler-infi.ir"
#define OTA_INFO_URL "ota.haffler-infi.ir"
#define OTA_FALLBACK_BASE_URL "https://ota.haffler-infi.ir"

//? ============= SCREEN =============

#define TFT_CS 13
#define TFT_DC 14
#define TFT_RST -1
#define TFT_BL -1

#define TFT_MISO 11
#define TFT_MOSI 10
#define TFT_SCLK 12

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

#define SCREEN_ROTATION 3

//? ============= TOUCH =============

#define TOUCH_ORIENTATION 270

#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TOUCH_RST -1
#define TOUCH_INT -1


//? ============= THERMOCOBLE =============

#define THERMO_MISO 37
#define THERMO_SCK 47
#define THERMO_1_CS 21
#define THERMO_2_CS 0

//? ============= OUTPUT =============

#define ON LOW
#define OFF HIGH

#define BUZZER 45

// آیتم: جابجاییِ پایه از منوی اطلاعات — این ۱۰ پایه دیگر #define ثابت نیستند، متغیرهای قابل‌تغییرِ
// زمانِ اجرا هستند (تعریفِ واقعی با مقدارِ پیش‌فرض در main.cpp؛ storage.h مقدارِ ذخیره‌شده را
// موقعِ بوت — پیش از setupPin() — رویشان می‌نویسد). PIN_NC یعنی «به‌جایی وصل نیست».
#define PIN_NC (-1)
#define DEFAULT_SSR_1 15
#define DEFAULT_TABLE 16       //safhe Q0
#define DEFAULT_PRESS_LEFT 42  // press1 Q1
#define DEFAULT_PRESS_RIGHT 39 //press2 Q2
#define DEFAULT_ELEMENT 48     // res hiter Q3
#define DEFAULT_SHABLON 40     // shablon Q4
#define DEFAULT_ZARBEH1 35     // shablon Q6
#define DEFAULT_ZARBEH2 36     // shablon Q7
#define DEFAULT_EMERGENCY 3    // emergency //I6
#define DEFAULT_PEDAL 7        // pedal I7
// آیتم: ۲ ترمینالِ خروجیِ اضافیِ روی همین برد که در این برنامه به هیچ نامی وصل نیستند (در برنامه‌های
// دیگرِ همین پلتفرم استفاده می‌شوند — طبقِ تاییدِ کاربر). فقط به‌عنوانِ گزینهٔ *آزادِ* اضافی در دیالوگِ
// جابجاییِ پایه («Q8»/«Q9») نشان داده می‌شوند؛ هیچ نامی به‌طورِ پیش‌فرض صاحبِ این دو نیست.
#define DEFAULT_SPARE_Q8 38
#define DEFAULT_SPARE_Q9 41
#ifdef __cplusplus
extern "C" {
#endif
extern int SSR_1, TABLE, PRESS_LEFT, PRESS_RIGHT, ELEMENT, SHABLON, ZARBEH1, ZARBEH2;
extern int EMERGENCY, PEDAL;
#ifdef __cplusplus
}
#endif
#define BACK SENSOR_TABLE // برگشت = I5 روی پایه‌ی میز، باندِ بالا — قبلاً اشتباهاً روی GPIO5 (شابلون) بود

// سنسورهای جک — هر پایه‌ی آنالوگ دو سنسور را با باندِ ولتاژ کد می‌کند (طبق تستِ واقعیِ اپراتور):
//   ~2000 (1800-2200) = هیچ‌کدوم / در حالِ حرکت  (باندِ مرده → جلوگیری از خطای گذرای خازن)
//   ~1550 (1300-1800) = سنسورِ «باز» فعال
//   >3500 (~4095)     = سنسورِ «بسته» فعال
//   ~2500 (2300-3100) = هر دو فعال (فقط برای میز+برگشت ممکن است)
#define DEFAULT_SENSOR_ELEMENT 6   // GPIO6: I0=المنت باز(پایین) , I1=المنت بسته(بالا)
#define DEFAULT_SENSOR_SHABLON 5   // GPIO5: I2=شابلون باز(پایین) , I3=شابلون بسته(بالا)
#define DEFAULT_SENSOR_TABLE   4   // GPIO4: I4=میز باز(پایین) , I5=برگشت(بالا)

// هویت فیزیکی هر شمارهٔ I روی برد (مستقل از ردیفِ UI) — برای remap مستقلِ ردیف‌ها
#ifndef GPIO_FOR_INPUT_SILK_FN
#define GPIO_FOR_INPUT_SILK_FN
static inline int gpio_for_input_silk(int iNum) {
    switch (iNum) {
        case 0: case 1: return DEFAULT_SENSOR_ELEMENT;
        case 2: case 3: return DEFAULT_SENSOR_SHABLON;
        case 4: case 5: return DEFAULT_SENSOR_TABLE;
        case 6: return DEFAULT_EMERGENCY;
        case 7: return DEFAULT_PEDAL;
        default: return PIN_NC;
    }
}
// قطبیت باند = همان تعریف سنسور بالا + چاپ I روی برد:
//   زوج (I0/I2/I4 = باز)  → باند۱ (~1550)
//   فرد (I1/I3/I5 = بسته) → باند۰ (~4095)
//   باند۳ = هر دو
static inline bool input_silk_signal_active(int silk, int band) {
    if (silk < 0 || silk > 5) return false;
    if (silk & 1) return (band == 0 || band == 3);
    return (band == 1 || band == 3);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern int SENSOR_ELEMENT, SENSOR_SHABLON, SENSOR_TABLE;
#ifdef __cplusplus
}
#endif
#define SENSOR_BAND_OPEN_LO  1300 // سنسورِ «باز»: 1300..1800
#define SENSOR_BAND_OPEN_HI  1800
#define SENSOR_BAND_NONE_LO  1850
#define SENSOR_BAND_NONE_HI  2240
#define SENSOR_BAND_BOTH_LO  2350
#define SENSOR_BAND_BOTH_HI  2750
#define SENSOR_BAND_CLOSED   3500 // سنسورِ «بسته»: > 3500
#define SENSOR_TIMEOUT_MS    7000 // اگر سنسور تا ۷ ثانیه نرسید → خطا + استوپ



//? ============= DIGITAL INPUT =============


// 5 -- I
// 6 -- I

//? ============= ANALOG INPUT =============

#define KEY_F 1
#define KEY_M 2

#define HEAT_SENSOR 25
// #define LAMA_SENSOR 36

//? ============= EEPROM =============

#define MAGIC_NUMBER 0xBBFF0FF6

//? ============= RTOS =============

#define LVGL_TICK_PERIOD 1

// #define PID 15

//? ============= LOCK / COUNTDOWN BRIDGE (آیتم ۴/۸) =============
// متغیرهای پل بین main.cpp (C++) و screens.c (C). main آن‌ها را به‌روز می‌کند.
#ifdef __cplusplus
extern "C" {
#endif
extern volatile int g_lock_remaining_days; // -1 = تایمر غیرفعال، 0 = قفل، >0 روزِ مانده
extern volatile int g_device_locked;       // 0/1 — آیا دستگاه هم‌اکنون قفل است
// دیالوگ هشدار دما (آیتم ۲): task درخواست می‌دهد، دیالوگ تأیید/انصراف را برمی‌گرداند.
extern volatile int g_temp_warn_request;   // 1 = task می‌خواهد دیالوگ نشان داده شود
extern volatile int g_temp_override_start; // 1 = کاربر «تأیید» زد → سیکل با دمای پایین شروع شود
// پیغام پایان جوش (#۳): task در پایان چرخه درخواست می‌دهد، UI پیغام را می‌سازد.
extern volatile int g_weld_done_request;        // 1 = نمایش پیغام پایان جوش
extern volatile unsigned long g_weld_done_elapsed_ms; // زمان کل از اولین استارت تا پایان
extern volatile int g_weld_done_frames;         // تعداد فریم (قابل‌پاک‌شدن)
// خطای سنسور: اگر جک تا ۷ ثانیه نرسید، task درخواست می‌دهد، UI خطا را نشان می‌دهد و چرخه استوپ می‌شود.
extern volatile int g_sensor_err_request;       // 1 = نمایش خطای سنسور
extern volatile int g_sensor_err_id;            // 1=شابلون بسته 2=شابلون باز 3=میز باز 4=المنت بسته 5=المنت باز
// پُرشدنِ پلکانیِ لامپِ جک با سنسور: مرحله‌ی فعالِ سنسوردار و درصدِ پُرشدن (۰..۱۰۰). step=-1 یعنی هیچ.
extern volatile int g_sensor_lamp_step;
extern volatile int g_sensor_lamp_pct;
// خطای SSR/المنت: المنت با وجود فرمانِ گرم‌شدن، گرم نمی‌شود (SSR قطع شد + هشدار).
extern volatile bool g_ssr_fault;
extern volatile bool g_ssr_warn_dismissed;
// گرمای داخلیِ سخت‌افزار (بیشترینِ دمای CPU/RTC): 0=نرمال، 1=۷۰-۷۹.۹، 2=۸۰-۸۴.۹، 3=۸۵+ (بحرانی).
extern volatile int g_internal_temp_band;
// لامپِ سنسوریِ هر جک (۰..۱۰۰)؛ -1 = حالت زمانی (سنسور خاموش). بدونِ وابستگی به مرحلهٔ گذرا.
extern volatile int g_lamp_pct_shablon;
extern volatile int g_lamp_pct_table;
extern volatile int g_lamp_pct_element;
// روی خطای سنسور/استپ: لامپ‌های سیگنال بدونِ درنظرگرفتنِ سنسورها خاموش (خالی) شوند.
extern volatile bool g_force_lamps_off;
// مقدار خامِ ADC هر سنسور (برای دیباگ/تشخیصِ باندِ «both active»).
extern volatile int g_sens_raw_shablon;
extern volatile int g_sens_raw_element;
extern volatile int g_sens_raw_table;
// خطای سنسور: حالتِ نامعتبر (نه باز نه بسته → احتمالاً هر دو سنسور فعال یا قطع).
extern volatile bool g_sensor_err_both;
// شمارهٔ لنگه در چهارچوبِ جاری (۱..۴) برای پیامِ پایان جوش.
extern volatile int g_weld_done_in_frame;
// هشدار فشردن پدال/استارت خارج از صفحه اصلی (منوی تنظیمات)
extern volatile int g_pedal_outside_menu_warn_request;
extern bool g_splash_variant;
#ifdef __cplusplus
}
#endif



