#include "boot_guard.h"
#include "ui_resume.h"
#include <rom/ets_sys.h>

#define BOOT_GUARD_MAGIC      0xB007C0DF
#define BOOT_GUARD_CRASH_MAX  2       // آستانه تشخیص بوت‌لوپ: ۲ کرش زودهنگام پیاپی
#define BOOT_GUARD_STEADY_MS  25000   // زمان مورد نیاز برای اثبات سلامت فریمور (۲۵ ثانیه)

// متغیرهای ذخیره‌شده در حافظه فوق‌سریع RTC Slow SRAM
// پایدار در برابر انواع ریست، پنیک، واچ‌داگ و ری‌استارت نرم‌افزاری
RTC_NOINIT_ATTR static uint32_t s_boot_guard_magic;
RTC_NOINIT_ATTR static uint32_t s_boot_guard_magic_inv;
RTC_NOINIT_ATTR static uint8_t  s_early_crash_count;
RTC_NOINIT_ATTR static uint8_t  s_boot_steady_state; // 0: در حال بوت/نامعتبر، 1: پایدار و تاییدشده

void boot_guard_early_check(void) {
    const esp_reset_reason_t rr = esp_reset_reason();
    const bool is_hang = ui_resume_reset_is_hang(rr);

    // بررسی سلامت حافظه RTC
    bool magic_ok = (s_boot_guard_magic == BOOT_GUARD_MAGIC &&
                     s_boot_guard_magic_inv == ~BOOT_GUARD_MAGIC);

    if (!magic_ok) {
        // استارت سرد یا بار اول پس از قطع برق: مقداردهی اولیه متغیرها
        s_boot_guard_magic = BOOT_GUARD_MAGIC;
        s_boot_guard_magic_inv = ~BOOT_GUARD_MAGIC;
        s_early_crash_count = 0;
        s_boot_steady_state = 0;
        ets_printf("[BOOT-GUARD] Cold boot / fresh RTC initialized (rr=%d)\n", (int)rr);
        return;
    }

    if (is_hang) {
        if (s_boot_steady_state == 0) {
            // کرش در حین بوت یا قبل از رسیدن به وضعیت پایدار رخ داده است!
            s_early_crash_count++;
            ets_printf("\n[BOOT-GUARD] WARNING: Early boot crash detected! Count=%u/%u (rr=%d)\n",
                       (unsigned)s_early_crash_count, (unsigned)BOOT_GUARD_CRASH_MAX, (int)rr);
        } else {
            // کرش پس از سپری شدن زمان پایداری و در جریان کارکرد عادی ماشین رخ داده است
            // طبق دستور اکید کارفرما، کرش‌های حین کار نباید به پروژه کانفیگ منتقل شوند
            s_early_crash_count = 0;
            s_boot_steady_state = 0;
            ets_printf("[BOOT-GUARD] Post-steady state reset detected (rr=%d). Safe recovery allowed.\n", (int)rr);
        }
    } else {
        // ریست نرمال سخت‌افزاری یا برنامه‌ریزی‌شده
        s_early_crash_count = 0;
        s_boot_steady_state = 0;
    }

    // اگر تعداد کرش‌های متوالی زودهنگام به آستانه بحرانی رسید: فریمور معیوب است!
    if (s_early_crash_count >= BOOT_GUARD_CRASH_MAX) {
        ets_printf("\n===================================================\n");
        ets_printf("[BOOT-GUARD] CRITICAL BOOT LOOP DETECTED (%u crashes)!\n", (unsigned)s_early_crash_count);
        ets_printf("[BOOT-GUARD] Rolling back to Config-Test / fallback partition...\n");
        ets_printf("===================================================\n");

        // صفر کردن شمارنده قبل از ریست تا پارتیشن هدف در لوپ نیفتد
        s_early_crash_count = 0;
        s_boot_steady_state = 0;
        s_boot_guard_magic = 0;
        s_boot_guard_magic_inv = 0;
        ui_resume_clear();

        boot_guard_rollback_to_fallback_partition();
    }
}

bool boot_guard_rollback_to_fallback_partition(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *target = NULL;

    if (running) {
        if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
            target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
        } else {
            target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
        }
    }

    if (!target) {
        target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    }

    if (running) {
        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                ets_printf("[BOOT-GUARD] Image is pending verify -> invoking esp_ota_mark_app_invalid_rollback_and_reboot()...\n");
                esp_ota_mark_app_invalid_rollback_and_reboot();
                // اگر تابع بالا برنگردد، سیستم خودش ریست شده است
            }
        }
    }

    if (target) {
        ets_printf("[BOOT-GUARD] Setting boot partition explicitly to %s @ 0x%08x\n",
                   target->label, (unsigned)target->address);
        esp_err_t err = esp_ota_set_boot_partition(target);
        if (err == ESP_OK) {
            ets_printf("[BOOT-GUARD] Boot partition set successfully. Restarting now...\n");
            esp_restart();
            return true;
        } else {
            ets_printf("[BOOT-GUARD] Failed to set boot partition (err=%d)\n", (int)err);
        }
    } else {
        ets_printf("[BOOT-GUARD] ERROR: No valid target fallback partition found!\n");
    }

    // در بدترین حالت ریست ساده انجام شود
    esp_restart();
    return false;
}

void boot_guard_mark_steady_state(void) {
    if (s_boot_steady_state) return;
    s_boot_steady_state = 1;
    s_early_crash_count = 0;

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_valid_cancel_rollback();
            ets_printf("[BOOT-GUARD] Current OTA firmware verified and marked VALID (rollback cancelled).\n");
        }
    }
    ets_printf("[BOOT-GUARD] System reached steady state (uptime=%lu ms). Safety interlocks locked.\n", (unsigned long)millis());
}

void boot_guard_check_steady_state(void) {
    if (!s_boot_steady_state && millis() > BOOT_GUARD_STEADY_MS) {
        boot_guard_mark_steady_state();
    }
}

uint8_t boot_guard_get_early_crash_count(void) {
    return s_early_crash_count;
}

bool boot_guard_is_steady_state(void) {
    return (s_boot_steady_state != 0);
}
