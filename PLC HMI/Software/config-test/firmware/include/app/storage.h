#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include "defines.h"
#include <freertos/semphr.h>  // SemaphoreHandle_t برای محافظت از race condition

#define WIFI_STORE_MAX 16  // حداکثر تعداد شبکه‌های ذخیره‌شده

class Storage
{
private:
    Preferences preferences;
    String version = "1.0.0";

    int firmware_partion_select = 0;

    // ----- WiFi saved credentials (in-RAM cache, persisted to NVS) -----
    int wifi_count = 0;
    String wifi_ssids[WIFI_STORE_MAX];
    String wifi_pwds[WIFI_STORE_MAX];

    mutable SemaphoreHandle_t _mutex = nullptr;  // محافظ دسترسی همزمان (Core 0 ↔ Core 1)
                                                  // mutable: متدهای const نیز می‌توانند mutex بگیرند (B-01)


public:
    void begin()
    {
        _mutex = xSemaphoreCreateMutex();  // یک‌بار در setup() — قبل از هر نخ دیگری
        preferences.begin("chone", false);
        if (preferences.getULong("magic", 0x00) != MAGIC_NUMBER)
        {
            preferences.putULong("magic", MAGIC_NUMBER);
            save();
        }
        else
        {
            load();
        }
        loadWifi();
    }

    int language = 2;  // پیش‌فرض: فارسی (LANG_FA=2)
    int buzzer_volume = 180; // پیش‌فرض ولوم بازر
    bool sound_enabled = true; // فعال یا خاموش بودن عمومی صدا
    String admin_password = "123";
    String hwtest_password = "0000";
    float tc1_offset = 0.0f;
    float tc2_offset = 0.0f;
    String last_qc_report = "";
    uint8_t last_is_hwtest = 0;
    uint8_t last_screen_id = 0;
    uint8_t last_suite = 0;
    int16_t last_scroll_x = 0;

    void load()
    {
        version = preferences.getString("version", "1.0.0");
        firmware_partion_select = preferences.getInt("fw_part_sel", preferences.getInt("firmware_partio", 0));
        language = preferences.getInt("language", 2);
        buzzer_volume = preferences.getInt("buz_vol", 180);
        sound_enabled = preferences.getBool("snd_en", true);
        admin_password = preferences.getString("adm_pass", "123");
        hwtest_password = preferences.getString("hwt_pass", "0000");
        tc1_offset = preferences.getFloat("tc1_off", 0.0f);
        tc2_offset = preferences.getFloat("tc2_off", 0.0f);
        last_qc_report = preferences.getString("qc_rep", "");
        last_is_hwtest = preferences.getUChar("lst_hwt", 0);
        last_screen_id = preferences.getUChar("lst_scr", 0);
        last_suite = preferences.getUChar("lst_ste", 0);
        last_scroll_x = preferences.getShort("scx", 0);
    }

    void save()
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        preferences.putString("version", version);
        preferences.putInt("fw_part_sel", firmware_partion_select);
        preferences.putInt("language", language);
        preferences.putInt("buz_vol", buzzer_volume);
        preferences.putBool("snd_en", sound_enabled);
        preferences.putString("adm_pass", admin_password);
        preferences.putString("hwt_pass", hwtest_password);
        preferences.putFloat("tc1_off", tc1_offset);
        preferences.putFloat("tc2_off", tc2_offset);
        preferences.putString("qc_rep", last_qc_report);
        preferences.putUChar("lst_hwt", last_is_hwtest);
        preferences.putUChar("lst_scr", last_screen_id);
        preferences.putUChar("lst_ste", last_suite);
        preferences.putShort("scx", last_scroll_x);
        if (_mutex) xSemaphoreGive(_mutex);
    }

    void saveLastScreen(uint8_t is_hwtest, uint8_t screen_id, uint8_t suite)
    {
        if (screen_id == 0 || (is_hwtest == 0 && screen_id == 1)) return; // هرگز اسپلش یا نامعتبر ذخیره نشود
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        if (last_is_hwtest != is_hwtest || last_screen_id != screen_id || last_suite != suite)
        {
            last_is_hwtest = is_hwtest;
            last_screen_id = screen_id;
            last_suite = suite;
            preferences.putUChar("lst_hwt", last_is_hwtest);
            preferences.putUChar("lst_scr", last_screen_id);
            preferences.putUChar("lst_ste", last_suite);
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    void saveScrollX(int16_t x)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        if (last_scroll_x != x)
        {
            last_scroll_x = x;
            preferences.putShort("scx", last_scroll_x);
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    int16_t getScrollX()
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        int16_t x = last_scroll_x;
        if (_mutex) xSemaphoreGive(_mutex);
        return x;
    }

    void factoryReset()
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        preferences.clear();
        admin_password = "123";
        hwtest_password = "0000";
        tc1_offset = 0.0f;
        tc2_offset = 0.0f;
        wifi_count = 0;
        for (int i = 0; i < WIFI_STORE_MAX; i++) {
            wifi_ssids[i] = "";
            wifi_pwds[i] = "";
        }
        last_is_hwtest = 0;
        last_screen_id = 0;
        last_suite = 0;
        last_scroll_x = 0;
        sound_enabled = true;
        buzzer_volume = 180;
        last_qc_report = "";
        if (_mutex) xSemaphoreGive(_mutex);
    }

    bool getLastScreen(uint8_t &is_hwtest, uint8_t &screen_id, uint8_t &suite)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        is_hwtest = last_is_hwtest;
        screen_id = last_screen_id;
        suite = last_suite;
        if (_mutex) xSemaphoreGive(_mutex);
        return (screen_id > 1 || is_hwtest != 0);
    }

    bool isSoundEnabled() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        bool en = sound_enabled && (buzzer_volume > 0);
        if (_mutex) xSemaphoreGive(_mutex);
        return en;
    }

    void setSoundEnabled(bool en)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        sound_enabled = en;
        preferences.putBool("snd_en", sound_enabled);
        if (_mutex) xSemaphoreGive(_mutex);
    }

    int consumeSplashAlternate()
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        int cur = preferences.getInt("splash_alt", 0);
        preferences.putInt("splash_alt", (cur == 0) ? 1 : 0);
        if (_mutex) xSemaphoreGive(_mutex);
        return cur;
    }

    void saveBuzzerVolume(int vol)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        buzzer_volume = vol;                        // B-18: داخل mutex (قبلاً خارج بود → race condition)
        preferences.putInt("buz_vol", vol);
        if (_mutex) xSemaphoreGive(_mutex);
    }
    int getBuzzerVolume() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        int val = buzzer_volume;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    void setBuzzerVolume(int v)
    {
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        saveBuzzerVolume(v);
    }

    int getLanguage() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        int val = language;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    void setLanguage(int v)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        language = v;
        preferences.putInt("language", language);
        if (_mutex) xSemaphoreGive(_mutex);
    }

    String getAdminPassword() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        String val = admin_password;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    void setAdminPassword(const String &p)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        if (admin_password != p)
        {
            admin_password = p;
            preferences.putString("adm_pass", admin_password);
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    String getHwTestPassword() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        String val = hwtest_password;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    void setHwTestPassword(const String &p)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        if (hwtest_password != p)
        {
            hwtest_password = p;
            preferences.putString("hwt_pass", hwtest_password);
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    float getTc1Offset() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        float val = tc1_offset;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    void setTc1Offset(float v)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        if (fabs(tc1_offset - v) >= 0.001f)
        {
            tc1_offset = v;
            preferences.putFloat("tc1_off", tc1_offset);
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    float getTc2Offset() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        float val = tc2_offset;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    void setTc2Offset(float v)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        if (fabs(tc2_offset - v) >= 0.001f)
        {
            tc2_offset = v;
            preferences.putFloat("tc2_off", tc2_offset);
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    String getLastQcReport() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        String val = last_qc_report;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    void setLastQcReport(const String &r)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        if (last_qc_report != r)
        {
            last_qc_report = r;
            preferences.putString("qc_rep", last_qc_report);
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    // ===================== WiFi credential store =====================

    void loadWifi()
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        wifi_count = preferences.getInt("wc", 0);
        if (wifi_count < 0) wifi_count = 0;
        if (wifi_count > WIFI_STORE_MAX) wifi_count = WIFI_STORE_MAX;
        for (int i = 0; i < wifi_count; i++)
        {
            wifi_ssids[i] = preferences.getString((String("ws") + i).c_str(), "");
            wifi_pwds[i]  = preferences.getString((String("wp") + i).c_str(), "");
        }
        if (_mutex) xSemaphoreGive(_mutex);
    }

    // B-08: این متد داخلی است؛ فقط از saveWifi صدا زده می‌شود (زیرِ mutex)
    // public نگه‌داشتیم چون inline در همین کلاس صدا زده می‌شود — بیرون از کلاس نباید مستقیم صدا زده شود
    void persistWifi()
    {
        // توجه: فراخوان (saveWifi) قبلاً mutex گرفته — تکرار mutex ممنوع (non-recursive)
        preferences.putInt("wc", wifi_count);
        for (int i = 0; i < wifi_count; i++)
        {
            preferences.putString((String("ws") + i).c_str(), wifi_ssids[i]);
            preferences.putString((String("wp") + i).c_str(), wifi_pwds[i]);
        }
    }

    // آیا برای این SSID رمزی ذخیره شده؟
    bool hasWifi(const String &ssid) const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        bool found = false;
        for (int i = 0; i < wifi_count; i++)
            if (wifi_ssids[i] == ssid) { found = true; break; }
        if (_mutex) xSemaphoreGive(_mutex);
        return found;
    }

    // رمز ذخیره‌شده‌ی این SSID (در صورت نبود، رشته‌ی خالی)
    String getWifiPassword(const String &ssid) const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        String pwd;
        for (int i = 0; i < wifi_count; i++)
            if (wifi_ssids[i] == ssid) { pwd = wifi_pwds[i]; break; }
        if (_mutex) xSemaphoreGive(_mutex);
        return pwd;
    }

    // ذخیره/به‌روزرسانی رمز یک شبکه؛ اگر پر بود، قدیمی‌ترین حذف می‌شود (FIFO)
    void saveWifi(const String &ssid, const String &pwd)
    {
        if (ssid.length() == 0) return;
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        // اگر از قبل هست، فقط رمز را به‌روز کن
        for (int i = 0; i < wifi_count; i++)
        {
            if (wifi_ssids[i] == ssid)
            {
                wifi_pwds[i] = pwd;
                persistWifi();
                if (_mutex) xSemaphoreGive(_mutex);
                return;
            }
        }
        // جدید است
        if (wifi_count < WIFI_STORE_MAX)
        {
            wifi_ssids[wifi_count] = ssid;
            wifi_pwds[wifi_count] = pwd;
            wifi_count++;
        }
        else
        {
            // شیفت به چپ، قدیمی‌ترین حذف، جدید در انتها
            for (int i = 1; i < WIFI_STORE_MAX; i++)
            {
                wifi_ssids[i - 1] = wifi_ssids[i];
                wifi_pwds[i - 1] = wifi_pwds[i];
            }
            wifi_ssids[WIFI_STORE_MAX - 1] = ssid;
            wifi_pwds[WIFI_STORE_MAX - 1] = pwd;
        }
        persistWifi();
        if (_mutex) xSemaphoreGive(_mutex);
    }

    // ----- Getters -----
    String getVersion() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        String val = version;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }
    int getFirmwarePartionSelect() const
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        int val = firmware_partion_select;
        if (_mutex) xSemaphoreGive(_mutex);
        return val;
    }

    // ----- Setters (with auto-save) -----
    void setVersion(const String &v)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        version = v;
        preferences.putString("version", version);
        if (_mutex) xSemaphoreGive(_mutex);
    }
    void setFirmwarePartionSelect(const int &v)
    {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        firmware_partion_select = v;
        preferences.putInt("fw_part_sel", firmware_partion_select);
        if (_mutex) xSemaphoreGive(_mutex);
    }
};


#endif
