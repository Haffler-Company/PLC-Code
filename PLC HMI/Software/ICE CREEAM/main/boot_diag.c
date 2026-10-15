#include "boot_diag.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "driver/temperature_sensor.h"
#include "ds3231.h"
#include "gt911.h"
#include "max6675.h"
#include "fault_log.h"
#include "esp_ota_ops.h"

static const char *TAG = "BOOT_DIAG";

static boot_diag_result_t s_diag = {0};
static bool s_dismissed = false;
static temperature_sensor_handle_t s_temp_sensor = NULL;

static void init_cpu_temp_sensor(void)
{
    if (s_temp_sensor) return;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
    if (temperature_sensor_install(&temp_sensor_config, &s_temp_sensor) == ESP_OK) {
        temperature_sensor_enable(s_temp_sensor);
    }
}

void boot_diag_run_self_test(void)
{
    memset(&s_diag, 0, sizeof(s_diag));
    s_diag.user_resolvable = true; // پیش‌فرض؛ در صورت بروز خطای برد، false می‌شود

    ESP_LOGI(TAG, "=== Starting Hardware Power-On Self-Test (POST) ===");

    /* ۱. آزمون ساعت سخت‌افزاری DS3231 روی I2C */
    ds3231_time_t rtc_t;
    esp_err_t rtc_err = ds3231_get_time(&rtc_t);
    if (rtc_err != ESP_OK) {
        s_diag.has_error = true;
        s_diag.rtc_comms_fail = true;
        s_diag.user_resolvable = false;
        fault_log_record(FAULT_RTC_ERROR, (int16_t)rtc_err);
        ESP_LOGE(TAG, "[POST FAIL] RTC DS3231 I2C comms failed: %s", esp_err_to_name(rtc_err));
    } else {
        if (rtc_t.year < 24) {
            s_diag.rtc_battery_lost = true;
            fault_log_record(FAULT_RTC_ERROR, (int16_t)rtc_t.year);
            ESP_LOGW(TAG, "[POST WARN] RTC DS3231 battery lost / unconfigured year: 20%02d", rtc_t.year);
        } else {
            ESP_LOGI(TAG, "[POST PASS] RTC DS3231 OK: 20%02d/%02d/%02d %02d:%02d:%02d",
                     rtc_t.year, rtc_t.month, rtc_t.date, rtc_t.hour, rtc_t.minute, rtc_t.second);
        }
    }

    /* ۲. آزمون کنترلر تاچ خازنی GT911 روی I2C */
    uint16_t tx = 0, ty = 0;
    bool touch_active = gt911_get_touch(&tx, &ty);
    (void)touch_active;
    // بررسی زنده بودن باس تاچ از طریق درایور
    if (!i2c_get_bus()) {
        s_diag.has_error = true;
        s_diag.touch_comms_fail = true;
        s_diag.user_resolvable = false;
        fault_log_record(FAULT_TOUCH_ERROR, 0);
        ESP_LOGE(TAG, "[POST FAIL] Touch controller GT911 I2C bus offline");
    } else {
        ESP_LOGI(TAG, "[POST PASS] Touch controller GT911 bus OK");
    }

    /* ۳. آزمون سنسورهای دمای صنعتی (تشخیص خودکار MAX6675 یا DS18B20 روی GPIO 21 و GPIO 37) */
    temp_sensor_mode_t smode = max6675_get_sensor_mode();
    bool top_ok = max6675_is_top_valid();
    bool cyl_ok = max6675_is_cylinder_valid();

    if (smode == TEMP_SENSOR_MODE_MAX6675) {
        uint16_t raw_t1 = max6675_get_raw_top();
        uint16_t raw_t2 = max6675_get_raw_cylinder();

        if (max6675_is_top_enabled()) {
            if (!top_ok) {
                s_diag.tc1_probe_disconnected = true;
                fault_log_record(FAULT_THERMO_TOP_DISCONNECT, (int16_t)raw_t1);
                ESP_LOGW(TAG, "[POST WARN] MAX6675 T1 thermocouple probe disconnected (raw=0x%04X)", raw_t1);
            } else {
                s_diag.tc1_temp = max6675_get_temp_top();
                ESP_LOGI(TAG, "[POST PASS] MAX6675 T1 (Hopper) OK: %.1f C", s_diag.tc1_temp);
            }
        }

        if (!cyl_ok) {
            s_diag.tc2_probe_disconnected = true;
            fault_log_record(FAULT_THERMO_CYL_DISCONNECT, (int16_t)raw_t2);
            ESP_LOGW(TAG, "[POST WARN] MAX6675 T2 thermocouple probe disconnected (raw=0x%04X)", raw_t2);
        } else {
            s_diag.tc2_temp = max6675_get_temp_cylinder();
            ESP_LOGI(TAG, "[POST PASS] MAX6675 T2 (Cylinder) OK: %.1f C", s_diag.tc2_temp);
        }

        /* عدم اتصال سنسورها وضعیت عملیاتی اختیاری است و نباید خطای سخت‌افزاری ایجاد کند */
    } else {
        /* مد DS18B20: بررسی حضور سنسورها روی GPIO 21 (T1) و GPIO 37 (T2) */
        if (top_ok) {
            s_diag.tc1_temp = max6675_get_temp_top();
            ESP_LOGI(TAG, "[POST PASS] DS18B20 T1 (Hopper, GPIO %d) OK: %.2f C", (int)DS18B20_TOP_GPIO, s_diag.tc1_temp);
        } else if (max6675_is_top_enabled()) {
            s_diag.tc1_probe_disconnected = true;
            fault_log_record(FAULT_MAX6675_TOP_MISSING, (int16_t)DS18B20_TOP_GPIO);
            ESP_LOGW(TAG, "[POST WARN] DS18B20 T1 (Hopper, GPIO %d) not detected", (int)DS18B20_TOP_GPIO);
        }

        if (cyl_ok) {
            s_diag.tc2_temp = max6675_get_temp_cylinder();
            ESP_LOGI(TAG, "[POST PASS] DS18B20 T2 (Cylinder, GPIO %d) OK: %.2f C", (int)DS18B20_CYL_GPIO, s_diag.tc2_temp);
        } else {
            s_diag.tc2_probe_disconnected = true;
            fault_log_record(FAULT_MAX6675_CYL_MISSING, (int16_t)DS18B20_CYL_GPIO);
            ESP_LOGW(TAG, "[POST WARN] DS18B20 T2 (Cylinder, GPIO %d) not detected", (int)DS18B20_CYL_GPIO);
        }
    }

    /* ۴. آزمون حافظه جانبی PSRAM و هیپ داخلی SRAM */
    size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t free_sram = esp_get_free_heap_size();
    if (free_spiram < 100000) {
        s_diag.has_error = true;
        s_diag.psram_fail = true;
        s_diag.user_resolvable = false;
        fault_log_record(FAULT_MEMORY_ERROR, (int16_t)(free_spiram / 1024));
        ESP_LOGE(TAG, "[POST FAIL] PSRAM heap critically low or absent: %u bytes", (unsigned)free_spiram);
    } else {
        ESP_LOGI(TAG, "[POST PASS] PSRAM memory healthy: %u KB free", (unsigned)(free_spiram / 1024));
    }

    if (free_sram < 35000) {
        s_diag.has_error = true;
        s_diag.heap_low = true;
        s_diag.user_resolvable = false;
        fault_log_record(FAULT_MEMORY_ERROR, (int16_t)(free_sram / 1024));
        ESP_LOGE(TAG, "[POST FAIL] Internal SRAM critically low: %u bytes", (unsigned)free_sram);
    } else {
        ESP_LOGI(TAG, "[POST PASS] Internal SRAM OK: %u KB free", (unsigned)(free_sram / 1024));
    }

    /* ۵. آزمون دمای داخلی پردازنده ESP32-S3 */
    init_cpu_temp_sensor();
    float cpu_celsius = 0.0f;
    if (s_temp_sensor && temperature_sensor_get_celsius(s_temp_sensor, &cpu_celsius) == ESP_OK) {
        s_diag.cpu_temp = cpu_celsius;
        if (cpu_celsius > 75.0f) {
            s_diag.has_error = true;
            s_diag.cpu_overheat = true;
            s_diag.user_resolvable = false;
            fault_log_record(FAULT_CPU_OVERHEAT, (int16_t)cpu_celsius);
            ESP_LOGE(TAG, "[POST FAIL] CPU temperature critical: %.1f C", cpu_celsius);
        } else {
            ESP_LOGI(TAG, "[POST PASS] CPU silicon temperature normal: %.1f C", cpu_celsius);
        }
        temperature_sensor_disable(s_temp_sensor);
        temperature_sensor_uninstall(s_temp_sensor);
        s_temp_sensor = NULL;
    }

    /* ۶. تجمیع و ساخت متن خطاها و راهکارها به زبان‌های سه‌گانه */
    if (s_diag.has_error) {
        s_diag.summary_fa[0] = '\0';
        s_diag.cause_fa[0] = '\0';
        s_diag.solution_fa[0] = '\0';

        s_diag.summary_en[0] = '\0';
        s_diag.cause_en[0] = '\0';
        s_diag.solution_en[0] = '\0';

        s_diag.summary_tr[0] = '\0';
        s_diag.cause_tr[0] = '\0';
        s_diag.solution_tr[0] = '\0';


        if (s_diag.rtc_battery_lost) {
            strlcat(s_diag.summary_fa, "- باتری ساعت خالی است یا تاریخ تنظیم نیست\n", sizeof(s_diag.summary_fa));
            strlcat(s_diag.cause_fa, "خالی شدن باتری سکه‌ای ساعت یا ریست تاریخ.\n", sizeof(s_diag.cause_fa));
            strlcat(s_diag.solution_fa, "باتری سکه‌ای CR2032 روی برد را بررسی یا تعویض کرده و ساعت را در منو تنظیم کنید.\n", sizeof(s_diag.solution_fa));

            strlcat(s_diag.summary_en, "* RTC battery low or time not set\n", sizeof(s_diag.summary_en));
            strlcat(s_diag.cause_en, "CR2032 coin battery is depleted or RTC lost power.\n", sizeof(s_diag.cause_en));
            strlcat(s_diag.solution_en, "Replace the CR2032 coin cell on the board and configure clock.\n", sizeof(s_diag.solution_en));

            strlcat(s_diag.summary_tr, "* RTC pili bitmis veya saat ayarsiz\n", sizeof(s_diag.summary_tr));
            strlcat(s_diag.cause_tr, "CR2032 saat pili bitmis veya sifirlanmis.\n", sizeof(s_diag.cause_tr));
            strlcat(s_diag.solution_tr, "CR2032 pilini degistirin ve menuden saati ayarlayin.\n", sizeof(s_diag.solution_tr));
        }
        if (s_diag.rtc_comms_fail) {
            strlcat(s_diag.summary_fa, "- عدم ارتباط با آی‌سی ساعت RTC DS3231\n", sizeof(s_diag.summary_fa));
            strlcat(s_diag.summary_en, "* DS3231 RTC communication failure\n", sizeof(s_diag.summary_en));
            strlcat(s_diag.summary_tr, "* DS3231 RTC saat iletisim hatasi\n", sizeof(s_diag.summary_tr));
        }
        if (s_diag.touch_comms_fail) {
            strlcat(s_diag.summary_fa, "- کنترلر تاچ خازنی شناسایی نشد\n", sizeof(s_diag.summary_fa));
            strlcat(s_diag.summary_en, "* Capacitive touch controller not found\n", sizeof(s_diag.summary_en));
            strlcat(s_diag.summary_tr, "* Kapasitif dokunmatik algilanamadi\n", sizeof(s_diag.summary_tr));
        }
        if (s_diag.psram_fail || s_diag.heap_low) {
            strlcat(s_diag.summary_fa, "- خطای فضای حافظه رم سیستم (RAM/PSRAM)\n", sizeof(s_diag.summary_fa));
            strlcat(s_diag.summary_en, "* System RAM/PSRAM memory low\n", sizeof(s_diag.summary_en));
            strlcat(s_diag.summary_tr, "* Sistem RAM/PSRAM bellek yetersiz\n", sizeof(s_diag.summary_tr));
        }
        if (s_diag.cpu_overheat) {
            char tbuf[64];
            snprintf(tbuf, sizeof(tbuf), "- دمای پردازنده بحرانی است (%d C)\n", (int)s_diag.cpu_temp);
            strlcat(s_diag.summary_fa, tbuf, sizeof(s_diag.summary_fa));
            snprintf(tbuf, sizeof(tbuf), "* Critical CPU temperature (%d C)\n", (int)s_diag.cpu_temp);
            strlcat(s_diag.summary_en, tbuf, sizeof(s_diag.summary_en));
            snprintf(tbuf, sizeof(tbuf), "* Kritik islemci sicakligi (%d C)\n", (int)s_diag.cpu_temp);
            strlcat(s_diag.summary_tr, tbuf, sizeof(s_diag.summary_tr));
        }

        // در صورتی که خطایی فراتر از توان کاربر باشد، راهکار پشتیبانی کارخانه اضافه می‌شود
        if (!s_diag.user_resolvable) {
            strlcat(s_diag.cause_fa, "ایراد سخت‌افزاری در مدارات داخلی یا آی‌سی‌های برد الکترونیک.\n", sizeof(s_diag.cause_fa));
            strlcat(s_diag.solution_fa, "این ایراد نیاز به بررسی تخصصی الکترونیک دارد.\nلطفاً با پشتیبانی کارخانه آستیاج تماس بگیرید:\nتلفن: 09102367062 | وب‌سایت: ASTIAJ.ir\n", sizeof(s_diag.solution_fa));

            strlcat(s_diag.cause_en, "Internal hardware fault in controller board circuitry.\n", sizeof(s_diag.cause_en));
            strlcat(s_diag.solution_en, "Requires technical service. Please contact ASTIAJ Support:\nPhone: 09102367062 | ASTIAJ.ir\n", sizeof(s_diag.solution_en));

            strlcat(s_diag.cause_tr, "Elektronik kart devrelerinde donanim arizasi.\n", sizeof(s_diag.cause_tr));
            strlcat(s_diag.solution_tr, "Teknik servis gereklidir. Lutfen ASTIAJ Fabrika Destegi arayin:\nTel: 09102367062 | ASTIAJ.ir\n", sizeof(s_diag.solution_tr));
        }

        ESP_LOGW(TAG, "=== POST Completed with ISSUES ===\n%s", s_diag.summary_fa);
    } else {
        ESP_LOGI(TAG, "=== Hardware POST Completed: ALL PASS CLEANLY ===");
    }

    /* بررسی وضعیت پارتیشن آزمایشی OTA و تایید دائم فریمور در صورت سلامت برد (Anti-Brick Rollback) */
    const esp_partition_t *running_part = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (running_part && esp_ota_get_state_partition(running_part, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "OTA partition is in PENDING_VERIFY -> Firmware booted cleanly, marking app VALID & canceling rollback.");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}

const boot_diag_result_t* boot_diag_get_result(void)
{
    return &s_diag;
}

bool boot_diag_has_error(void)
{
    return s_diag.has_error && (s_diag.summary_fa[0] != '\0');
}

bool boot_diag_is_dismissed(void)
{
    return s_dismissed;
}

void boot_diag_dismiss(void)
{
    s_dismissed = true;
}

void boot_diag_retry(void)
{
    s_dismissed = false;
    max6675_reprobe();
    boot_diag_run_self_test();
}
