#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include "defines.h"

class Storage
{
private:
    Preferences preferences;
    String version = "1.0.0";

    int firmware_partion_select = 0;

    int auto_weight = 11;
    int auto_speed = 22;
    int chone_counter = 33;

    int setting_max_piston = 44;
    int setting_max_funnle = 55;

    bool activeCounterSound = true;
    uint8_t language = 0;   // 0: FA, 1: EN, 2: TR, 3: RU
    bool wifi_enabled = true;
    String wifi_ssid = "";
    String wifi_pass = "";
    unsigned long last_counter_save_ms = 0;
    int unsaved_counter_delta = 0;

public:
    void begin()
    {
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
    }

    void load()
    {
        version = preferences.getString("version", "1.0.0");
        auto_speed = preferences.getInt("auto_speed", 22);
        auto_weight = preferences.getInt("auto_weight", 11);
        chone_counter = preferences.getInt("chone_counter", 0);

        setting_max_piston = preferences.getInt("max_piston", 99);
        setting_max_funnle = preferences.getInt("max_funnle", 99);

        activeCounterSound = preferences.getBool("ACSound", true);
        language = preferences.getUChar("lang", 0);
        wifi_enabled = preferences.getBool("wifi_en", true);
        wifi_ssid = preferences.getString("wf_ssid", "");
        wifi_pass = preferences.getString("wf_pass", "");

        firmware_partion_select = preferences.getInt("firmware_partion_select", 0);
    }

    void save()
    {
        preferences.putString("version", version);
        preferences.putInt("auto_speed", auto_speed);
        preferences.putInt("auto_weight", auto_weight);
        preferences.putInt("chone_counter", chone_counter);

        preferences.putInt("max_piston", setting_max_piston);
        preferences.putInt("max_funnle", setting_max_funnle);

        preferences.putInt("ACSound", activeCounterSound ? 1 : 0);
        preferences.putUChar("lang", language);
        preferences.putBool("wifi_en", wifi_enabled);
        preferences.putString("wf_ssid", wifi_ssid);
        preferences.putString("wf_pass", wifi_pass);

        preferences.putInt("firmware_partion_select", firmware_partion_select);
    }

    // ----- Getters -----
    String getVersion() const { return version; }
    int getAutoSpeed() const { return auto_speed; }
    int getAutoWeight() const { return auto_weight; }
    int getChoneCounter() const { return chone_counter; }
    int getMaxPiston() const { return setting_max_piston; }
    int get_setting_max_funnle() const { return setting_max_funnle; }
    int get_activeCounterSound() const { return activeCounterSound; }
    uint8_t getLanguage() const { return language; }
    bool getWifiEnabled() const { return wifi_enabled; }
    String getWifiSsid() const { return wifi_ssid; }
    String getWifiPassword() const { return wifi_pass; }
    int getFirmwarePartionSelect() const { return firmware_partion_select; }

    void setWifiCredentials(const String &ssid, const String &pass)
    {
        if (wifi_ssid != ssid || wifi_pass != pass)
        {
            wifi_ssid = ssid;
            wifi_pass = pass;
            preferences.putString("wf_ssid", wifi_ssid);
            preferences.putString("wf_pass", wifi_pass);
        }
    }

    // ----- Setters (with auto-save & dirty-check) -----
    void setVersion(const String &v)
    {
        if (version != v)
        {
            version = v;
            preferences.putString("version", version);
        }
    }

    void setAutoSpeed(int t)
    {
        if (auto_speed != t)
        {
            auto_speed = t;
            preferences.putInt("auto_speed", auto_speed);
        }
    }

    void setAutoWeight(int w)
    {
        if (auto_weight != w)
        {
            auto_weight = w;
            preferences.putInt("auto_weight", auto_weight);
        }
    }

    void commitCounter()
    {
        if (unsaved_counter_delta != 0)
        {
            preferences.putInt("chone_counter", chone_counter);
            unsaved_counter_delta = 0;
            last_counter_save_ms = millis();
        }
    }

    void setChoneCounter(int c)
    {
        chone_counter = c;
        unsaved_counter_delta++;
        // Commit immediately if reset to 0, every 5 counts, or after 10 seconds
        if (c == 0 || unsaved_counter_delta >= 5 || (millis() - last_counter_save_ms > 10000UL))
        {
            commitCounter();
        }
    }

    void set_setting_max_piston(int c)
    {
        if (setting_max_piston != c)
        {
            setting_max_piston = c;
            preferences.putInt("max_piston", setting_max_piston);
        }
    }

    void set_setting_max_funnle(int c)
    {
        if (setting_max_funnle != c)
        {
            setting_max_funnle = c;
            preferences.putInt("max_funnle", setting_max_funnle);
        }
    }

    void set_ActiveCounterSound(int c)
    {
        if (activeCounterSound != (bool)c)
        {
            activeCounterSound = (bool)c;
            preferences.putInt("ACSound", activeCounterSound ? 1 : 0);
        }
    }

    void setLanguage(uint8_t l)
    {
        if (language != l)
        {
            language = l;
            preferences.putUChar("lang", language);
        }
    }

    void setWifiEnabled(bool en)
    {
        if (wifi_enabled != en)
        {
            wifi_enabled = en;
            preferences.putBool("wifi_en", wifi_enabled);
        }
    }

    void setFirmwarePartionSelect(const int &v)
    {
        if (firmware_partion_select != v)
        {
            firmware_partion_select = v;
            preferences.putInt("firmware_partion_select", firmware_partion_select);
        }
    }
};

#endif
