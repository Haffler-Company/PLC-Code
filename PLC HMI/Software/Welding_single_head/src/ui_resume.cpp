#include "ui_resume.h"
#include <esp_system.h>
#include <rom/ets_sys.h>

// --- متغیرهای ذخیره‌سازی در حافظه فوق‌سریع RTC Slow SRAM ---
// این حافظه در زمان خاموش/روشن‌شدن ریست می‌شود، اما در تمامی انواع ریست ناشی از
// کرش، واچ‌داگ و ری‌استارت‌های نرم‌افزاری کاملاً پایدار می‌ماند.
// صفر استهلاک فلش، صفر کلاک تأخیر، بدون اشغال حافظه دینامیک Heap.
RTC_NOINIT_ATTR static uint32_t ui_resume_scr_magic;
RTC_NOINIT_ATTR static uint32_t ui_resume_scr_magic_inv;
RTC_NOINIT_ATTR static int16_t  ui_resume_last_screen_id;
RTC_NOINIT_ATTR static int16_t  ui_resume_exit_screen_id;

static bool    s_resume_boot_latched = false;
static bool    s_resume_boot_valid   = false;
static int16_t s_resume_boot_scr     = -1;
static bool    s_resume_apply_pending = false;

extern bool g_is_crash_recovery_boot;
extern uint32_t g_ota_boot_request;
extern uint32_t g_self_update_boot_request;
extern volatile int g_device_locked;

#define OTA_BOOT_MAGIC 0xB007C0DEu
#define SELF_UPDATE_BOOT_MAGIC 0x5E1FCEC0u

// تفکیک دقیق علت ریست: صرفاً علت‌های واقعی هنگ/واچ‌داگ/پنیک
bool ui_resume_reset_is_hang(esp_reset_reason_t rr) {
    switch (rr) {
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
#ifdef ESP_RST_CPU_LOCKUP
        case ESP_RST_CPU_LOCKUP:
#endif
            return true;
        default:
            return false;
    }
}

// لچ اولیه در اولین خط تابع setup()
void ui_resume_boot_latch_early(void) {
    if (s_resume_boot_latched) return;
    s_resume_boot_latched = true;

    const esp_reset_reason_t rr = esp_reset_reason();
    const bool is_hang = ui_resume_reset_is_hang(rr);

    // ریست‌های تعمدی ورود به مد OTA/بروزرسانی نباید وارد ریکاوری صفحه شوند
    if (g_ota_boot_request == OTA_BOOT_MAGIC || g_self_update_boot_request == SELF_UPDATE_BOOT_MAGIC) {
        s_resume_boot_valid = false;
        s_resume_boot_scr = -1;
        s_resume_apply_pending = false;
        g_is_crash_recovery_boot = false;
        return;
    }

    // استارت سرد یا ریست سخت‌افزاری با دکمه EN برد: نمایش لوگو و اسپلش عادی
    if (!is_hang) {
        ui_resume_clear();
        s_resume_boot_valid = false;
        s_resume_boot_scr = -1;
        s_resume_apply_pending = false;
        g_is_crash_recovery_boot = false;
        ets_printf("[UIRESUME] Normal boot (rr=%d), regular startup\n", (int)rr);
        return;
    }

    // صحت‌سنجی دوگانه و ضد نویز حافظه RTC
    bool magic_ok = (ui_resume_scr_magic == UI_RESUME_SCR_MAGIC &&
                     ui_resume_scr_magic_inv == ~UI_RESUME_SCR_MAGIC);

    if (!magic_ok) {
        ui_resume_clear();
        s_resume_boot_valid = false;
        s_resume_boot_scr = -1;
        s_resume_apply_pending = false;
        g_is_crash_recovery_boot = false;
        ets_printf("[UIRESUME] Hang detected (rr=%d) but RTC magic invalid\n", (int)rr);
        return;
    }

    int16_t scr = ui_resume_last_screen_id;
    // دامنه مجاز صفحات کاربری (صفحات اسپلش، لودینگ، رمز عبور، بروزرسانی و وای‌فای نباید پس از کرش مجدداً باز شوند تا لوپ ریست رخ ندهد)
    if (scr < SCREEN_ID_SCREEN_MENU || scr > _SCREEN_ID_LAST ||
        scr == SCREEN_ID_SCREEN_LOADING ||
        scr == SCREEN_ID_SCREEN_ADMIN_PASSWORD ||
        scr == SCREEN_ID_SCREEN_UPDATE ||
        scr == SCREEN_ID_SCREEN_FRAMWARE ||
        scr == SCREEN_ID_SCREEN_PASSWORD ||
        scr == SCREEN_ID_SCREEN_WIFI) {
        scr = SCREEN_ID_SCREEN_MENU;
    }

    // اگر دستگاه به خاطر روزشمار قفل شده، فقط منوی اصلی مجاز است تا پنجره قفل ظاهر شود
    if (g_device_locked) {
        scr = SCREEN_ID_SCREEN_MENU;
    }

    s_resume_boot_valid = true;
    s_resume_boot_scr = scr;
    s_resume_apply_pending = true;
    g_is_crash_recovery_boot = true;
    ets_printf("[UIRESUME] Hang recovery latched! Direct to Screen=%d (rr=%d)\n", (int)scr, (int)rr);
}

// ثبت ورود کاربر به صفحه (Screen Entry)
void ui_resume_record_screen_entry(int16_t screen_id) {
    if (s_resume_apply_pending) return; // تا زمان اتمام بازیابی اولیه، مقدار بازنویسی نشود
    if (screen_id < SCREEN_ID_SCREEN_MENU || screen_id > _SCREEN_ID_LAST || screen_id == SCREEN_ID_SCREEN_LOADING) {
        return;
    }

    // گارد Write-on-Change در حافظه RTC Slow
    if (ui_resume_last_screen_id == screen_id &&
        ui_resume_scr_magic == UI_RESUME_SCR_MAGIC &&
        ui_resume_scr_magic_inv == ~UI_RESUME_SCR_MAGIC) {
        return;
    }

    ui_resume_last_screen_id = screen_id;
    ui_resume_scr_magic = UI_RESUME_SCR_MAGIC;
    ui_resume_scr_magic_inv = ~UI_RESUME_SCR_MAGIC;
}

// ثبت خروج کاربر از صفحه (Screen Exit)
void ui_resume_record_screen_exit(int16_t from_screen_id, int16_t to_screen_id) {
    if (s_resume_apply_pending) return;
    if (from_screen_id >= SCREEN_ID_SCREEN_MENU && from_screen_id <= _SCREEN_ID_LAST && from_screen_id != SCREEN_ID_SCREEN_LOADING) {
        ui_resume_exit_screen_id = from_screen_id;
    }

    // اگر مقصد حرکت کاربر معتبر و مشخص است، پیش از ترنزیشن ثبت کن
    if (to_screen_id >= SCREEN_ID_SCREEN_MENU && to_screen_id <= _SCREEN_ID_LAST && to_screen_id != SCREEN_ID_SCREEN_LOADING) {
        ui_resume_last_screen_id = to_screen_id;
        ui_resume_scr_magic = UI_RESUME_SCR_MAGIC;
        ui_resume_scr_magic_inv = ~UI_RESUME_SCR_MAGIC;
    }
}

// بررسی نیاز به لود مستقیم صفحه بدون نمایش اسپلش
bool ui_resume_boot_should_restore(int16_t *out_screen) {
    if (!s_resume_boot_latched) {
        ui_resume_boot_latch_early();
    }
    if (out_screen) {
        *out_screen = s_resume_boot_scr;
    }
    return s_resume_boot_valid;
}

int16_t ui_resume_boot_get_screen(void) {
    if (!s_resume_boot_latched) {
        ui_resume_boot_latch_early();
    }
    return s_resume_boot_valid ? s_resume_boot_scr : (int16_t)SCREEN_ID_SCREEN_MENU;
}

void ui_resume_notify_screen_applied(void) {
    s_resume_apply_pending = false;
}

void ui_resume_clear(void) {
    ui_resume_scr_magic = 0;
    ui_resume_scr_magic_inv = 0;
    ui_resume_last_screen_id = -1;
    ui_resume_exit_screen_id = -1;
}

bool ui_resume_is_hang_recovery(void) {
    return s_resume_boot_valid;
}

// توابع سازگار با کدهای قدیمی
extern "C" void rtc_save_active_screen(int16_t screen_id) {
    ui_resume_record_screen_entry(screen_id);
}

extern "C" int16_t get_saved_active_screen(void) {
    if (s_resume_boot_valid && s_resume_boot_scr >= SCREEN_ID_SCREEN_MENU && s_resume_boot_scr <= _SCREEN_ID_LAST) {
        return s_resume_boot_scr;
    }
    if (ui_resume_scr_magic == UI_RESUME_SCR_MAGIC &&
        ui_resume_scr_magic_inv == ~UI_RESUME_SCR_MAGIC &&
        ui_resume_last_screen_id >= SCREEN_ID_SCREEN_MENU &&
        ui_resume_last_screen_id <= _SCREEN_ID_LAST &&
        ui_resume_last_screen_id != SCREEN_ID_SCREEN_LOADING) {
        return ui_resume_last_screen_id;
    }
    return (int16_t)SCREEN_ID_SCREEN_MENU;
}
