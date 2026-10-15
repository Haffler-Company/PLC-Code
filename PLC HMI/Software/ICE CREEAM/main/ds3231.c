#include "project_config.h"
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "gt911.h"        // برای i2c_master_init() و i2c_get_bus()
#include "ds3231.h"
#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_timer.h"
#include <time.h>
#include <sys/time.h>


static const char *TAG = "DS3231";

#define DS3231_ADDR      0x68

// رجیسترهای اصلی
#define REG_SECONDS      0x00
#define REG_MINUTES      0x01
#define REG_HOURS        0x02
#define REG_DAY          0x03
#define REG_DATE         0x04
#define REG_MONTH        0x05
#define REG_YEAR         0x06

// BCD helpers
static inline uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v & 0x0F) + ((v >> 4) * 10)); }
static inline uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static i2c_master_dev_handle_t s_ds_dev = NULL;

static esp_err_t ds3231_write(uint8_t reg, const uint8_t *data, size_t len)
{
    if (!s_ds_dev) return ESP_ERR_INVALID_STATE;
    if (len > 0 && !data) return ESP_ERR_INVALID_ARG;
    uint8_t tmp[1 + 8];
    if (len > sizeof(tmp) - 1) return ESP_ERR_INVALID_SIZE;
    tmp[0] = reg;
    if (len && data) memcpy(&tmp[1], data, len);
    return i2c_master_transmit(s_ds_dev, tmp, 1 + len, 10);
}

static esp_err_t ds3231_read(uint8_t reg, uint8_t *data, size_t len)
{
    if (!s_ds_dev) return ESP_ERR_INVALID_STATE;
    if (len > 0 && !data) return ESP_ERR_INVALID_ARG;
    return i2c_master_transmit_receive(s_ds_dev, &reg, 1, data, len, 10);
}

static bool s_ds_online = false;
static int64_t s_last_hw_read_us = 0;

esp_err_t ds3231_init(void)
{
    ESP_RETURN_ON_ERROR(i2c_master_init(), TAG, "i2c bus init failed");

    i2c_master_bus_handle_t bus = i2c_get_bus();
    if (!bus) {
        ESP_LOGE(TAG, "i2c_get_bus() returned NULL");
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_ds_dev) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_7,
            .device_address  = DS3231_ADDR,
            .scl_speed_hz    = I2C_FREQ_HZ,
        };

        esp_err_t add_err = i2c_master_bus_add_device(bus, &dev_cfg, &s_ds_dev);
        if (add_err != ESP_OK) {
            ESP_LOGE(TAG, "add_device failed: %s", esp_err_to_name(add_err));
            return add_err;
        }
    }

    // خواندن یک بایت تستی برای اطمینان (ثانیه)
    uint8_t sec = 0;
    esp_err_t err = ds3231_read(REG_SECONDS, &sec, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "DS3231 probe read failed (%s) — detaching device to keep Touch I2C bus clean", esp_err_to_name(err));
        i2c_master_bus_rm_device(s_ds_dev);
        s_ds_dev = NULL;
        s_ds_online = false;
        return err;
    }

    s_ds_online = true;
    s_last_hw_read_us = 0;
    ESP_LOGI(TAG, "DS3231 ready @0x%02X", DS3231_ADDR);
    ds3231_sync_system_time();
    return ESP_OK;
}

static esp_err_t ds3231_read_hw_time(ds3231_time_t *t)
{
    if (!t || !s_ds_dev || !s_ds_online) return ESP_ERR_INVALID_STATE;

    uint8_t buf[7];
    esp_err_t err = ds3231_read(REG_SECONDS, buf, sizeof(buf));
    if (err != ESP_OK) return err;

    uint8_t sec   = buf[0] & 0x7F;
    uint8_t min   = buf[1] & 0x7F;
    uint8_t hour  = buf[2];
    uint8_t day   = buf[3] & 0x07;
    uint8_t date  = buf[4] & 0x3F;
    uint8_t month = buf[5] & 0x1F;
    uint8_t year  = buf[6];

    if (hour & 0x40) {
        uint8_t ampm = (hour & 0x20) ? 1 : 0;
        hour = bcd2bin(hour & 0x1F);
        if (hour == 12) hour = 0;
        if (ampm) hour += 12;
    } else {
        hour = bcd2bin(hour & 0x3F);
    }

    t->second = bcd2bin(sec);
    t->minute = bcd2bin(min);
    t->hour   = hour;
    t->day    = day;
    t->date   = bcd2bin(date);
    t->month  = bcd2bin(month);
    t->year   = bcd2bin(year);
    return ESP_OK;
}

esp_err_t ds3231_get_time(ds3231_time_t *t)
{
    if (!t) return ESP_ERR_INVALID_ARG;

    /* استفاده از کلاک داخلی همگام‌شده بین فواصل ۶۰ ثانیه‌ای جهت حذف کامل ترافیک اضافه روی باس I2C تاچ */
    int64_t now_us = esp_timer_get_time();
    if (s_ds_online && (s_last_hw_read_us == 0 || (now_us - s_last_hw_read_us) >= 60000000LL)) {
        if (ds3231_read_hw_time(t) == ESP_OK) {
            s_last_hw_read_us = now_us;
            return ESP_OK;
        }
    }

    time_t now = time(NULL);
    struct tm ti = {0};
    localtime_r(&now, &ti);
    if (ti.tm_year >= (2024 - 1900)) {
        t->second = (uint8_t)ti.tm_sec;
        t->minute = (uint8_t)ti.tm_min;
        t->hour   = (uint8_t)ti.tm_hour;
        t->day    = (uint8_t)(ti.tm_wday + 1);
        t->date   = (uint8_t)ti.tm_mday;
        t->month  = (uint8_t)(ti.tm_mon + 1);
        t->year   = (uint8_t)((ti.tm_year + 1900) % 100);
        return ESP_OK;
    }

    return ds3231_read_hw_time(t);
}

esp_err_t ds3231_sync_system_time(void)
{
    ds3231_time_t t;
    esp_err_t err = ds3231_get_time(&t);
    if (err != ESP_OK) return err;

    struct tm tm_info = {
        .tm_sec = t.second,
        .tm_min = t.minute,
        .tm_hour = t.hour,
        .tm_mday = t.date,
        .tm_mon = (t.month > 0) ? (t.month - 1) : 0,
        .tm_year = (2000 + t.year) - 1900,
        .tm_isdst = -1
    };
    time_t epoch = mktime(&tm_info);
    if (epoch > 0) {
        struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        ESP_LOGI(TAG, "POSIX system time synced with DS3231: epoch=%ld", (long)epoch);
    }
    return ESP_OK;
}

time_t ds3231_get_epoch(void)
{
    ds3231_time_t t;
    if (ds3231_get_time(&t) != ESP_OK) return 0;

    struct tm tm_info = {
        .tm_sec = t.second,
        .tm_min = t.minute,
        .tm_hour = t.hour,
        .tm_mday = t.date,
        .tm_mon = (t.month > 0) ? (t.month - 1) : 0,
        .tm_year = (2000 + t.year) - 1900,
        .tm_isdst = -1
    };
    return mktime(&tm_info);
}


esp_err_t ds3231_set_time(const ds3231_time_t *t)
{
    if (!t) return ESP_ERR_INVALID_ARG;

    // 24h می‌نویسیم
    uint8_t buf[7];
    buf[0] = bin2bcd(t->second);
    buf[1] = bin2bcd(t->minute);
    buf[2] = bin2bcd(t->hour) & 0x3F;     // 24h -> بیت 6=0
    buf[3] = bin2bcd(t->day   & 0x07);    // 1..7
    buf[4] = bin2bcd(t->date  & 0x3F);
    buf[5] = bin2bcd(t->month & 0x1F);    // century=0
    buf[6] = bin2bcd(t->year);

    esp_err_t err = ds3231_write(REG_SECONDS, buf, sizeof(buf));
    if (err == ESP_OK) {
        s_last_hw_read_us = 0;
        ds3231_sync_system_time();
    }
    return err;
}

esp_err_t rtc_set(uint16_t yyyy, uint8_t mm, uint8_t dd,
                  uint8_t hh, uint8_t mi, uint8_t ss,
                  uint8_t wday /* 1..7 */)
{
    ds3231_time_t t = {
        .second = ss,
        .minute = mi,
        .hour   = hh,               // 0..23
        .day    = wday,             // 1..7
        .date   = dd,               // 1..31
        .month  = mm,               // 1..12
        .year   = (uint8_t)(yyyy % 100) // سال دو رقمی
    };
    return ds3231_set_time(&t);
}

// الگوریتم دقیق جلالی (بر مبنای جلالیِ منصوری/روزبه/هاداک) – نسخه‌ی فشرده
void gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd)
{
    static const int g_month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    static const int j_month_days[12] = {31,31,31,31,31,31,30,30,30,30,30,29};

    int gy2 = gy - 1600;
    int gm2 = gm - 1;
    int gd2 = gd - 1;

    long g_day_no = 365L * gy2 + (gy2 + 3) / 4 - (gy2 + 99) / 100 + (gy2 + 399) / 400;
    for (int i = 0; i < gm2; ++i) g_day_no += g_month_days[i];
    // کبیسه‌ی گرگوری برای بعد از فوریه
    if (gm2 > 1 && ((gy % 4 == 0 && gy % 100 != 0) || (gy % 400 == 0))) g_day_no += 1;
    g_day_no += gd2;

    long j_day_no = g_day_no - 79;

    int j_np = (int)(j_day_no / 12053);        // هر 33 سالِ جلالی = 12053 روز
    j_day_no %= 12053;

    int jy2 = 979 + 33 * j_np + 4 * (int)(j_day_no / 1461);
    j_day_no %= 1461;

    if (j_day_no >= 366) {
        jy2 += (int)((j_day_no - 1) / 365);
        j_day_no = (j_day_no - 1) % 365;
    }

    int jm2 = 0;
    while (jm2 < 11 && j_day_no >= j_month_days[jm2]) {
        j_day_no -= j_month_days[jm2];
        ++jm2;
    }

    *jy = jy2;
    *jm = jm2 + 1;
    *jd = (int)j_day_no + 1;
}

// Jalali -> Gregorian (پایدار)
void jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd)
{
    static const int g_month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    static const int j_month_days[12] = {31,31,31,31,31,31,30,30,30,30,30,29};

    int jy2 = jy - 979;
    int jm2 = jm - 1;
    int jd2 = jd - 1;

    long j_day_no = 365L * jy2 + (jy2 / 33) * 8 + ((jy2 % 33) + 3) / 4;
    for (int i = 0; i < jm2; ++i) j_day_no += j_month_days[i];
    j_day_no += jd2;

    long g_day_no = j_day_no + 79;

    int gy2 = 1600 + (int)(400 * (g_day_no / 146097));
    g_day_no %= 146097;

    int leap = 1;
    if (g_day_no >= 36525) {
        g_day_no--;
        gy2 += (int)(100 * (g_day_no / 36524));
        g_day_no %= 36524;
        if (g_day_no >= 365) g_day_no++;
        else leap = 0;
    }

    gy2 += (int)(4 * (g_day_no / 1461));
    g_day_no %= 1461;

    if (g_day_no >= 366) {
        leap = 0;
        g_day_no--;
        gy2 += (int)(g_day_no / 365);
        g_day_no %= 365;
    }

    int gm2 = 0;
    while (gm2 < 11) {
        int md = g_month_days[gm2];
        if (gm2 == 1 && leap) md++;  // فوریه در سال کبیسه
        if (g_day_no < md) break;
        g_day_no -= md;
        ++gm2;
    }

    *gy = gy2;
    *gm = gm2 + 1;
    *gd = (int)g_day_no + 1;
}

// محاسبه‌ی روز هفته‌ی میلادی (Sakamoto). خروجی: 0=Sunday..6=Saturday
static int weekday_gregorian(int y, int m, int d)
{
    static const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (m < 3) y -= 1;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

// ---------- Public wrappers for Jalali ----------

esp_err_t ds3231_get_time_jalali(jalali_time_t *jt)
{
    if (!jt) return ESP_ERR_INVALID_ARG;

    ds3231_time_t t;
    esp_err_t err = ds3231_get_time(&t);
    if (err != ESP_OK) return err;

    int jy, jm, jd;
    // t.year دو رقمی است؛ فرض: 2000..2099
    int gy = 2000 + (int)t.year;
    int gm = (int)t.month;
    int gd = (int)t.date;

    gregorian_to_jalali(gy, gm, gd, &jy, &jm, &jd);

    jt->jy     = (uint16_t)jy;
    jt->jm     = (uint8_t)jm;
    jt->jd     = (uint8_t)jd;
    jt->hour   = t.hour;
    jt->minute = t.minute;
    jt->second = t.second;

    // wday در DS3231: 1=Sunday..7=Saturday
    int w = weekday_gregorian(gy, gm, gd); // 0..6 with 0=Sunday
    jt->wday = (uint8_t)(w + 1);
    return ESP_OK;
}

esp_err_t rtc_set_jalali(uint16_t jyy, uint8_t jmm, uint8_t jdd,
                         uint8_t hh, uint8_t mi, uint8_t ss)
{
    int gy, gm, gd;
    jalali_to_gregorian((int)jyy, (int)jmm, (int)jdd, &gy, &gm, &gd);

    // روز هفته برای DS3231 (1=Sunday..7=Saturday)
    int w = weekday_gregorian(gy, gm, gd); // 0..6
    uint8_t wday = (uint8_t)(w + 1);

    return rtc_set((uint16_t)gy, (uint8_t)gm, (uint8_t)gd, hh, mi, ss, wday);
}
