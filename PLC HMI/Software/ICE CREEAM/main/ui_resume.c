#include "ui_resume.h"
#include "storage.h"
#include "project_config.h"
#include <string.h>
#include "esp_attr.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "UI_RESUME";

#define UI_RESUME_MAGIC         0x5A3C7E14UL
#define UI_RESUME_CODE_PREFIX   0xA7000000UL

/* Variables placed in RTC Slow Memory: preserved across software resets, panic, and WDT */
RTC_NOINIT_ATTR static uint32_t s_resume_magic;
RTC_NOINIT_ATTR static uint32_t s_resume_magic_inv;
RTC_NOINIT_ATTR static uint32_t s_resume_view_code;
RTC_NOINIT_ATTR static uint32_t s_resume_consecutive_hangs;

static bool s_is_recovered_boot = false;
static bool s_is_post_ota = false;
static bool s_ui_resume_ready = false;
static int s_pending_resume_tab = -1;

bool ui_resume_is_post_ota_boot(void)
{
    return s_is_post_ota;
}

void ui_resume_mark_post_ota(const char *prev_ver, const char *new_ver)
{
    storage_set_u32("ota_upd_ok", 1);
    if (prev_ver && prev_ver[0]) {
        storage_set_str("ota_prev_ver", prev_ver);
    }
    if (new_ver && new_ver[0]) {
        storage_set_str("ota_new_ver", new_ver);
    }
    s_resume_magic = 0;
    s_resume_magic_inv = 0;
    s_resume_view_code = 0;
    s_resume_consecutive_hangs = 0;
    storage_set_i32("last_scr", (int32_t)SCREEN_ID_MAIN_PG);
    storage_set_i32("last_tab", 0);
}

int ui_resume_get_pending_tab(void)
{
    return s_pending_resume_tab;
}

void ui_resume_clear_pending_tab(void)
{
    s_pending_resume_tab = -1;
}

void ui_resume_set_pending_tab(int tab)
{
    s_pending_resume_tab = tab;
}

void ui_resume_set_ready(bool ready)
{
    s_ui_resume_ready = ready;
    ESP_LOGI(TAG, "UI resume engine %s", ready ? "ENABLED" : "DISABLED");
}

bool ui_resume_is_ready(void)
{
    return s_ui_resume_ready;
}

static bool ui_resume_reset_is_hang(esp_reset_reason_t rr)
{
    switch (rr) {
        case ESP_RST_PANIC:       /* Software crash / exception */
        case ESP_RST_INT_WDT:     /* Interrupt watchdog timeout */
        case ESP_RST_TASK_WDT:    /* Task watchdog timeout (hung task) */
        case ESP_RST_WDT:         /* Other watchdog resets */
        case ESP_RST_SW:          /* Software restart (esp_restart) */
#ifdef ESP_RST_CPU_LOCKUP
        case ESP_RST_CPU_LOCKUP:  /* CPU lockup reset */
#endif
        case ESP_RST_EXT:         /* Hardware reset pin / button */
        case ESP_RST_BROWNOUT:    /* Power cut / brownout voltage dip */
            return true;
        case ESP_RST_POWERON:     /* Cold power switch on */
        default:
            return false;
    }
}

void ui_resume_clear(void)
{
    s_resume_magic = 0;
    s_resume_magic_inv = 0;
    s_resume_view_code = 0;
    storage_set_i32("last_scr", (int32_t)SCREEN_ID_MAIN_PG);
    storage_set_i32("last_tab", 0);
}

void ui_resume_checkpoint(enum ScreensEnum screenId, int tab_idx)
{
    /* هنگام بوت و ساخت اولیه ویجت‌ها، هیچ بازنویسی در NVS یا RTC نباید رخ دهد */
    if (!s_ui_resume_ready) {
        return;
    }
    if (screenId <= SCREEN_ID_SPLASH_SCREEN || screenId >= SCREEN_ID_SCREEN_FRAMWARE) {
        return;
    }
    if (tab_idx < 0) tab_idx = 0;
    if (tab_idx > 7) tab_idx = 7;

    uint32_t code = UI_RESUME_CODE_PREFIX |
                    ((uint32_t)screenId & 0x0Fu) |
                    (((uint32_t)tab_idx & 0x0Fu) << 4);

    s_resume_view_code = code;
    s_resume_magic = UI_RESUME_MAGIC;
    s_resume_magic_inv = ~UI_RESUME_MAGIC;

    /* ذخیره پایدار در Flash NVS برای مقاومت ۱۰۰٪ در برابر هرگونه افت برق یا ریست */
    storage_set_i32("last_scr", (int32_t)screenId);
    storage_set_i32("last_tab", (int32_t)tab_idx);
}

bool ui_resume_check_boot_recovery(enum ScreensEnum *out_screen, int *out_tab)
{
    if (out_screen) *out_screen = SCREEN_ID_SPLASH_SCREEN;
    if (out_tab) *out_tab = 0;

    /* در صورتی که ریست ناشی از اتمام موفقیت‌آمیز بروزرسانی OTA باشد یا نسخه فریمور تغییر کرده باشد،
     * به هیچ عنوان هنگ یا کرش تلقی نشود و پاپ‌آپ خطای سخت‌افزاری مسدود گردد */
    uint32_t ota_ok = storage_get_u32("ota_upd_ok", 0);
    char last_boot_ver[32] = {0};
    storage_get_str("app_boot_ver", "", last_boot_ver, sizeof(last_boot_ver));
    bool is_ver_upgraded = (strcmp(last_boot_ver, FIRMWARE_VERSION_STR) != 0);

    if (ota_ok == 1 || is_ver_upgraded) {
        ESP_LOGI(TAG, "Post-OTA / New Version boot detected (ota_ok=%lu, prev='%s', new='%s'). Bypassing crash/hang recovery.",
                 (unsigned long)ota_ok, last_boot_ver, FIRMWARE_VERSION_STR);
        s_is_post_ota = true;
        s_resume_consecutive_hangs = 0;
        s_is_recovered_boot = false;
        ui_resume_clear();
        storage_set_u32("ota_upd_ok", 0);
        storage_set_str("app_boot_ver", FIRMWARE_VERSION_STR);
        if (last_boot_ver[0] != '\0') {
            storage_set_str("ota_prev_ver", last_boot_ver);
        }
        storage_set_str("ota_new_ver", FIRMWARE_VERSION_STR);
        if (out_screen) *out_screen = SCREEN_ID_SPLASH_SCREEN;
        if (out_tab) *out_tab = 0;
        return false;
    }

    const esp_reset_reason_t rr = esp_reset_reason();
    const bool is_hang = ui_resume_reset_is_hang(rr);

    /* بررسی اعتبار رکورد در حافظه فوق سریع RTC Slow RAM */
    bool rtc_valid = (s_resume_magic == UI_RESUME_MAGIC &&
                      s_resume_magic_inv == ~UI_RESUME_MAGIC &&
                      (s_resume_view_code & 0xFF000000UL) == UI_RESUME_CODE_PREFIX);

    enum ScreensEnum target_scr = SCREEN_ID_MAIN_PG;
    int target_tab = 0;

    if (rtc_valid) {
        target_scr = (enum ScreensEnum)(s_resume_view_code & 0x0Fu);
        target_tab = (int)((s_resume_view_code >> 4) & 0x0Fu);
    } else {
        /* در صورت نامعتبر بودن RTC Slow RAM، خواندن آخرین صفحه/تب از Flash NVS */
        int32_t nvs_scr = storage_get_i32("last_scr", (int32_t)SCREEN_ID_MAIN_PG);
        int32_t nvs_tab = storage_get_i32("last_tab", 0);
        if (nvs_scr >= SCREEN_ID_MAIN_PG && nvs_scr <= SCREEN_ID_SCREEN_FRAMWARE) {
            target_scr = (enum ScreensEnum)nvs_scr;
            target_tab = (int)nvs_tab;
        }
    }

    if (!is_hang) {
        /* راه‌اندازی با کلید برق اصلی / قطع و وصل برق (ریست سخت‌افزاری) */
        ESP_LOGI(TAG, "Hardware power-on / cold boot detected (rr=%d). Running standard boot splash.", (int)rr);
        s_resume_consecutive_hangs = 0;
        s_is_recovered_boot = false;
        ui_resume_clear();
        if (out_screen) *out_screen = SCREEN_ID_SPLASH_SCREEN;
        if (out_tab) *out_tab = 0;
        return false;
    }

    /* در صورت بروز ریست نرم‌افزاری یا سخت‌افزاری، فعال‌سازی حفاظت از لوپ کرش */
    s_resume_consecutive_hangs++;
    if (s_resume_consecutive_hangs > 3) {
        ESP_LOGE(TAG, "Crash loop protection tripped (%lu consecutive crashes)! Dropping to Main Screen.",
                 (unsigned long)s_resume_consecutive_hangs);
        ui_resume_clear();
        s_resume_consecutive_hangs = 0;
        if (out_screen) *out_screen = SCREEN_ID_MAIN_PG;
        if (out_tab) *out_tab = 0;
        s_is_recovered_boot = false;
        return false;
    }

    if (target_scr <= SCREEN_ID_SPLASH_SCREEN || target_scr > SCREEN_ID_SCREEN_FRAMWARE) {
        target_scr = SCREEN_ID_MAIN_PG;
    }

    if (target_scr == SCREEN_ID_SETTINGS_PG) {
        s_pending_resume_tab = target_tab;
    } else {
        s_pending_resume_tab = -1;
    }

    if (out_screen) *out_screen = target_scr;
    if (out_tab) *out_tab = target_tab;

    s_is_recovered_boot = true;
    ESP_LOGW(TAG, "Seamless session recovery: rr=%d -> Instant resume to Screen=%d Tab=%d (operator sees zero interruption)",
             (int)rr, (int)target_scr, target_tab);
    return true;
}

void ui_resume_mark_stable(void)
{
    if (s_resume_consecutive_hangs > 0) {
        ESP_LOGI(TAG, "System running stable (>10s). Resetting crash counter.");
        s_resume_consecutive_hangs = 0;
    }
}

bool ui_resume_is_recovered(void)
{
    return s_is_recovered_boot;
}
