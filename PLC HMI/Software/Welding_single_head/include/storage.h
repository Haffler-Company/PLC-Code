#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include "defines.h"
#include "build_time.h"

extern volatile uint8_t g_buzzer_volume;

class Storage
{
private:
    Preferences preferences;
    String version = "1.0.0";
    int firmware_partion_select = 0;
    const char *namespaceName = "AJOSH1";

    int SelectedUser = 1;

    int WildingTime[5] = {20, 19, 18, 17, 16};
    int CoolingTime[5] = {20, 19, 18, 17, 16};
    int TempTarget[5] = {205, 216, 230, 235, 240};

    int TempCaliber = 0;
    int pid = 15;
    bool Speaker = true;
    uint8_t SpeakerVolume = 80;
    bool AutoStart = true;
    bool FinalJack = true;
    bool TempLimit = true;   // Ù¾ÛŒØ´â€ŒÙØ±Ø¶: Ù…Ø­Ø¯ÙˆØ¯ÛŒØª Ø¯Ù…Ø§ Ø±ÙˆØ´Ù†
    bool IsTreePedal = true;
    bool WifiOn = true;      // Ù¾ÛŒØ´â€ŒÙØ±Ø¶: ÙˆØ§ÛŒâ€ŒÙØ§ÛŒ Ø±ÙˆØ´Ù†
    String WifiSsid = "";
    String WifiPassword = "";
    bool SplashAlternate = false;

    // Ø§Ø³Ú©Ø±ÛŒÙ†â€ŒØ³ÛŒÙˆØ±Ù ØµÙØ­Ù‡Ù” Ø§ØµÙ„ÛŒ: Ø¯Ù‚ÛŒÙ‚Ù‡Ù” Ø¨ÛŒâ€ŒÚ©Ø§Ø±ÛŒ ØªØ§ ÙˆØ±ÙˆØ¯ Ø¨Ù‡ Ø­Ø§Ù„ØªÙ Ø§Ø³Ú©Ø±ÛŒÙ†â€ŒØ³ÛŒÙˆØ±. 0 = Ú©Ù„Ø§ ØºÛŒØ±ÙØ¹Ø§Ù„.
    // Ù…ÙˆÙ‚ØªØ§Ù‹ Ø±ÙˆÛŒ Û± Ø¯Ù‚ÛŒÙ‚Ù‡ Ø¨Ø±Ø§ÛŒ ØªØ³ØªÙ Ú©Ø§Ø±Ø¨Ø± â€” Ù¾ÛŒØ´â€ŒÙØ±Ø¶Ù Ù†Ù‡Ø§ÛŒÛŒ Û³Û° Ø¯Ù‚ÛŒÙ‚Ù‡ Ø§Ø³Øª (Ø±ÛŒØ³Øª ÙÚ©ØªÙˆØ±ÛŒ Ù‡Ù…ÛŒØ´Ù‡ Û³Û°
    // Ø¨Ø±Ù…ÛŒâ€ŒÚ¯Ø±Ø¯Ø§Ù†Ø¯ØŒ Ù…Ø³ØªÙ‚Ù„ Ø§Ø² Ø§ÛŒÙ† Ù…Ù‚Ø¯Ø§Ø±Ù Ù¾ÛŒØ´â€ŒÙØ±Ø¶Ù Ø§ÙˆÙ„ÛŒÙ†â€ŒØ¨ÙˆØª â€” Ù†Ú¯Ø§Ù‡ Ú©Ù† restoreDefaults()).
    int ScreensaverMin = 30;
    bool SysInfoBarEnabled = false;

    // ===== Ø°Ø®ÛŒØ±Ù‡â€ŒØ³Ø§Ø²ÛŒ Ú†Ù†Ø¯-Ø´Ø¨Ú©Ù‡Ù” ÙˆØ§ÛŒâ€ŒÙØ§ÛŒ (Ù…Ø«Ù„ Ù¾Ø±ÙˆÚ˜Ù‡Ù” Config) =====
    // Ø¹Ù„Ø§ÙˆÙ‡ Ø¨Ø± WifiSsid/WifiPassword (ØªÚ©â€ŒØ´Ø¨Ú©Ù‡Ù” Â«Ø¬Ø§Ø±ÛŒÂ»)ØŒ Ø±Ù…Ø²Ù Ú†Ù†Ø¯ Ø´Ø¨Ú©Ù‡ Ù†Ú¯Ù‡ Ø¯Ø§Ø´ØªÙ‡ Ù…ÛŒâ€ŒØ´ÙˆØ¯ ØªØ§
    // Ø§ØªØµØ§Ù„Ù Ø®ÙˆØ¯Ú©Ø§Ø± ÙˆÙ‚ØªÛŒ ÙÙ‚Ø· ÛŒÚ© Ø´Ø¨Ú©Ù‡Ù” Ø´Ù†Ø§Ø®ØªÙ‡â€ŒØ´Ø¯Ù‡ Ø¯Ø± Ø¯Ø³ØªØ±Ø³ Ø§Ø³Øª Ø§Ù†Ø¬Ø§Ù… Ø´ÙˆØ¯.
public:
    static const int WIFI_STORE_MAX = 16;
    int wifi_count = 0;
    String wifi_ssids[WIFI_STORE_MAX];
    String wifi_pwds[WIFI_STORE_MAX];
private:

    bool Is_J_type = true;
    bool EmergencySwich = true;
    bool SensorCheck = false; // Ù¾ÛŒØ´â€ŒÙØ±Ø¶: Ø¬Ú© Ø³Ù†Ø³ÙˆØ± Ø®Ø§Ù…ÙˆØ´
    int ReverseTimer = 0;
    String AdminPassword = "123456";
    int Lang = 1; // Ø²Ø¨Ø§Ù†: 1=ÙØ§Ø±Ø³ÛŒ 2=Ø§Ù†Ú¯Ù„ÛŒØ³ÛŒ 3=ØªØ±Ú©ÛŒ â€” Ù¾ÛŒØ´â€ŒÙØ±Ø¶Ù Ø§ÙˆÙ„ÛŒÙ† Ø¨ÙˆØª: Ø§Ù†Ú¯Ù„ÛŒØ³ÛŒ

    // ===== Ø³ÛŒØ³ØªÙ… Ù‚ÙÙ„/ØªØ§ÛŒÙ…Ø± Ù…Ø¹Ú©ÙˆØ³ (Ø¢ÛŒØªÙ… Û´/Ûµ/Û¶/Û¸) =====
    // ØªØ§Ø±ÛŒØ®â€ŒÙ‡Ø§ Ø¨Ù‡â€ŒØµÙˆØ±Øª Â«Ø´Ù…Ø§Ø±Ù‡Ù” Ø±ÙˆØ²Ù Ú˜ÙˆÙ„ÛŒÙ†ÛŒ (JDN)Â» Ø°Ø®ÛŒØ±Ù‡ Ù…ÛŒâ€ŒØ´ÙˆÙ†Ø¯ ØªØ§ Ø¨Ø§ RTC Ù…Ù‚Ø§ÛŒØ³Ù‡ Ø´ÙˆÙ†Ø¯.
    long CountdownEndJDN = 0;   // 0 = ØªØ§ÛŒÙ…Ø± Ù…Ø¹Ú©ÙˆØ³ ØºÛŒØ±ÙØ¹Ø§Ù„Ø› ÙˆÚ¯Ø±Ù†Ù‡ JDNÙ Ø±ÙˆØ²Ù Ù‚ÙÙ„â€ŒØ´Ø¯Ù†
    bool CountdownEnabled = false; // Ø¢ÛŒØ§ Ù†Ù…ÙˆØ¯Ø§Ø±Ù ØªÙ†Ø¸ÛŒÙ…Ù Ø±ÙˆØ² Ù‚Ø§Ø¨Ù„Ùâ€ŒÙ„Ù…Ø³/ÙˆÛŒØ±Ø§ÛŒØ´ Ø§Ø³ØªØŸ (Ù…Ø³ØªÙ‚Ù„Ù Ø§Ø² Ù…Ù‚Ø¯Ø§Ø±Ù ÙØ¹Ù„ÛŒÙ Ø±ÙˆØ²Ù‡Ø§)
    bool TempUnlockUsed = false; // Ø¢ÛŒØ§ Ø±Ù…Ø² Ù…ÙˆÙ‚Øª 112233 ÛŒÚ©â€ŒØ¨Ø§Ø± Ø§Ø³ØªÙØ§Ø¯Ù‡ Ø´Ø¯Ù‡ØŸ
    long TempUnlockEndJDN = 0;   // JDNÙ Ù¾Ø§ÛŒØ§Ù†Ù Ù…Ù‡Ù„ØªÙ Û±Û° Ø±ÙˆØ²Ù‡Ù” Ø±Ù…Ø² Ù…ÙˆÙ‚Øª
    long LastSeenJDN = 0; // anti-rollback RTC day

    // ===== Ø²Ù†Ø¬ÛŒØ±Ù‡Ù” ØªØ§ÛŒÙ…Ø±Ù‡Ø§ÛŒ Ú†Ú© (Ûµ Ù…Ø±Ø­Ù„Ù‡ â€” T1 Ø±ÙˆÛŒ ØµÙØ­Ù‡Ù” Ù…Ø¯ÛŒØ±ØŒ T2..T5 Ø¯Ø± Ø¯ÛŒØ§Ù„ÙˆÚ¯) =====
    uint8_t ActiveTimerIndex = 0;   // 1..5 = Ú©Ø¯Ø§Ù… ØªØ§ÛŒÙ…Ø± ÙØ¹Ø§Ù„ Ø§Ø³ØªØ› 0 = Ø²Ù†Ø¬ÛŒØ±Ù‡ Ø®Ø§Ù…ÙˆØ´
    long ChainStartJDN = 0;         // JDNÙ Ø´Ø±ÙˆØ¹Ù Ø²Ù†Ø¬ÛŒØ±Ù‡ (Ø¨Ø±Ø§ÛŒ Ù¾ÛŒØ´â€ŒÙ†Ù…Ø§ÛŒØ´Ù ØªØ§Ø±ÛŒØ®Ù Ù‚ÙÙ„)
    int MultiTimerDays[4] = {0, 0, 0, 0}; // Ø±ÙˆØ²Ù‡Ø§ÛŒ T2..T5 (Ù‡Ø± Ú©Ø¯Ø§Ù… Û°..ÛµÛ°)

    // Ù‚ÙÙ„ Ø±Ù…Ø² Ù…Ø¯ÛŒØ±: Ø¨Ø¹Ø¯ Ø§Ø² Û³ Ø¨Ø§Ø± Ø§Ø´ØªØ¨Ø§Ù‡ØŒ Û±Û° Ø¯Ù‚ÛŒÙ‚Ù‡ Ù‚ÙÙ„. Ø²Ù…Ø§Ù†Ù Ù¾Ø§ÛŒØ§Ù†Ù Ù‚ÙÙ„ Ø¨Ù‡â€ŒØµÙˆØ±Øª epoch (Ø«Ø§Ù†ÛŒÙ‡Ù” RTC) Ø°Ø®ÛŒØ±Ù‡ Ù…ÛŒâ€ŒØ´ÙˆØ¯ ØªØ§ Ø¨Ø§ Ø®Ø§Ù…ÙˆØ´/Ø±ÙˆØ´Ù† Ù‡Ù… Ø¨Ø§Ù‚ÛŒ Ø¨Ù…Ø§Ù†Ø¯.
    unsigned long AdminLockUntilEpoch = 0; // 0 = Ø¨Ø¯ÙˆÙ† Ù‚ÙÙ„

    int jack_safhe_raft = 600;
    int jack_safhe_bargasht = 1000;

    int jack_shablon_raft = 1200;
    int jack_shablon_bargasht = 1200;

    int jack_prifil_raft = 3000;
    int jack_prifil_bargasht = 6000;

    int jack_josh_raft = 2100;
    int jack_josh_bargasht = 2100;

    int jack_zarbe_1_raft = 0;
    int jack_zarbe_2_raft = 0;

    int frameNumbers=0;
    int frameNumbersCur=0; // Ú†Ù‡Ø§Ø±Ú†ÙˆØ¨Ù Â«Ø¬Ø§Ø±ÛŒÂ» (Ù‚Ø§Ø¨Ù„Ùâ€ŒØ±ÛŒØ³Øª ØªÙˆØ³Ø· Ú©Ø§Ø±Ø¨Ø±ØŒ Ù…ÙˆÙ†Ø¯Ú¯Ø§Ø±)
    int dailyFrameNumbers = 0;
    unsigned long dailyWeldTimeMs = 0;
    long dailyStatsJDN = 0;

    String WarrantyDate = ""; // ØªØ§Ø±ÛŒØ® ÙØ¹Ø§Ù„â€ŒØ³Ø§Ø²ÛŒ Ú¯Ø§Ø±Ø§Ù†ØªÛŒ (ÛŒÚ©â€ŒØ¨Ø§Ø± Ø«Ø¨ØªØŒ Ø¯ÛŒÚ¯Ø± ØªØºÛŒÛŒØ± Ù†Ù…ÛŒâ€ŒÚ©Ù†Ø¯)

    // ===== Ù†Ø§Ù… Ø´Ø±Ú©Øª (Ø¢ÛŒØªÙ… Û³) â€” ÛŒÚ©â€ŒØ¨Ø§Ø± Ø¯Ø± Ø§ÙˆÙ„ÛŒÙ† Ø¨ÙˆØª Ú¯Ø±ÙØªÙ‡ Ù…ÛŒâ€ŒØ´ÙˆØ¯ =====
    String CompanyName = "";   // Ø®Ø§Ù„ÛŒ = Ù¾ÛŒØ´â€ŒÙØ±Ø¶Ù Ù†Ù…Ø§ÛŒØ´ Â«HafflerÂ»
    bool CompanyAsked = false; // Ø¢ÛŒØ§ Ø¯Ø± Ø§ÙˆÙ„ÛŒÙ† Ø¨ÙˆØª Ù¾Ø±Ø³ÛŒØ¯Ù‡ Ø´Ø¯Ù‡ØŸ (ØªØ§ Ø¯ÙˆØ¨Ø§Ø±Ù‡ Ù†Ù¾Ø±Ø³Ø¯)
    bool AdminAsked = false;   // Ø¢ÛŒØ§ Ø±Ù…Ø² Ù…Ø¯ÛŒØ± Ø¯Ø± Ø§ÙˆÙ„ÛŒÙ† Ø¨ÙˆØª Ú¯Ø±ÙØªÙ‡ Ø´Ø¯Ù‡ØŸ (Ù…Ø±Ø­Ù„Ù‡Ù” Ø¨Ø¹Ø¯ Ø§Ø² Ù†Ø§Ù… Ø´Ø±Ú©Øª)

    String ManufacturerPhone = "";
    bool MfgPhoneAsked = false;

    static int clampProcessTime(int value)
    {
        if (value < 1) return 1;
        if (value > 9999) return 9999;
        return value;
    }

    static int clampTargetTemp(int value)
    {
        if (value < 0) return 0;
        if (value > 350) return 350;
        return value;
    }

public:
    // قطعِ مجازیِ هر ردیفِ ورودی (I0..I7) — مستقل از پایهٔ ADC مشترک
    bool inRowNc[8] = {false, false, false, false, false, false, false, false};
    // GPIO اختصاصی هر ردیف — هر ردیف می‌تواند به ترمینال I جدا وصل شود
    int inRowPin[8] = {
        DEFAULT_SENSOR_ELEMENT, DEFAULT_SENSOR_ELEMENT,
        DEFAULT_SENSOR_SHABLON, DEFAULT_SENSOR_SHABLON,
        DEFAULT_SENSOR_TABLE,   DEFAULT_SENSOR_TABLE,
        DEFAULT_EMERGENCY,      DEFAULT_PEDAL
    };
    // شمارهٔ چاپ‌شدهٔ I که کاربر برای این ردیف انتخاب کرده (۰..۷) — مستقل از زوج/فرد بودن ردیف
    int inRowSilk[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    void put_int_safe(const char *key, int32_t val) {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putInt(key, val);
            p.end();
        }
    }
    void put_bool_safe(const char *key, bool val) {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putBool(key, val);
            p.end();
        }
    }
    void put_string_safe(const char *key, const String &val) {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putString(key, val);
            p.end();
        }
    }
    void put_long_safe(const char *key, int32_t val) {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putLong(key, val);
            p.end();
        }
    }
    void put_ulong_safe(const char *key, uint32_t val) {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putULong(key, val);
            p.end();
        }
    }
    void put_uchar_safe(const char *key, uint8_t val) {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putUChar(key, val);
            p.end();
        }
    }

    void begin()
    {
        Preferences p;
        p.begin(namespaceName, false);
        if (p.getULong("magic", 0x00) != MAGIC_NUMBER)
        {
            p.putULong("magic", MAGIC_NUMBER);
            p.end();
            save();
        }
        else
        {
            p.end();
            load();
        }
    }

    void end()
    {
    }

    void loadPinMap()
    {
        static const int defInRowPin[8] = {
            DEFAULT_SENSOR_ELEMENT, DEFAULT_SENSOR_ELEMENT,
            DEFAULT_SENSOR_SHABLON, DEFAULT_SENSOR_SHABLON,
            DEFAULT_SENSOR_TABLE,   DEFAULT_SENSOR_TABLE,
            DEFAULT_EMERGENCY,      DEFAULT_PEDAL
        };
        Preferences p;
        p.begin(namespaceName, false);
        if (!p.isKey("pTABLE"))   p.putInt("pTABLE",   DEFAULT_TABLE);
        if (!p.isKey("pPRESSL"))  p.putInt("pPRESSL",  DEFAULT_PRESS_LEFT);
        if (!p.isKey("pPRESSR"))  p.putInt("pPRESSR",  DEFAULT_PRESS_RIGHT);
        if (!p.isKey("pELEMENT")) p.putInt("pELEMENT", DEFAULT_ELEMENT);
        if (!p.isKey("pSHABLON")) p.putInt("pSHABLON", DEFAULT_SHABLON);
        if (!p.isKey("pSSR1"))    p.putInt("pSSR1",    DEFAULT_SSR_1);
        if (!p.isKey("pZARBEH1")) p.putInt("pZARBEH1", DEFAULT_ZARBEH1);
        if (!p.isKey("pZARBEH2")) p.putInt("pZARBEH2", DEFAULT_ZARBEH2);
        if (!p.isKey("pSNELEM"))  p.putInt("pSNELEM",  DEFAULT_SENSOR_ELEMENT);
        if (!p.isKey("pSNSHBL"))  p.putInt("pSNSHBL",  DEFAULT_SENSOR_SHABLON);
        if (!p.isKey("pSNTABLE")) p.putInt("pSNTABLE", DEFAULT_SENSOR_TABLE);
        if (!p.isKey("pEMERG"))   p.putInt("pEMERG",   DEFAULT_EMERGENCY);
        if (!p.isKey("pPEDAL"))   p.putInt("pPEDAL",   DEFAULT_PEDAL);

        TABLE          = p.getInt("pTABLE",   DEFAULT_TABLE);
        PRESS_LEFT     = p.getInt("pPRESSL",  DEFAULT_PRESS_LEFT);
        PRESS_RIGHT    = p.getInt("pPRESSR",  DEFAULT_PRESS_RIGHT);
        ELEMENT        = p.getInt("pELEMENT", DEFAULT_ELEMENT);
        SHABLON        = p.getInt("pSHABLON", DEFAULT_SHABLON);
        SSR_1          = p.getInt("pSSR1",    DEFAULT_SSR_1);
        ZARBEH1        = p.getInt("pZARBEH1", DEFAULT_ZARBEH1);
        ZARBEH2        = p.getInt("pZARBEH2", DEFAULT_ZARBEH2);
        SENSOR_ELEMENT = p.getInt("pSNELEM",  DEFAULT_SENSOR_ELEMENT);
        SENSOR_SHABLON = p.getInt("pSNSHBL",  DEFAULT_SENSOR_SHABLON);
        SENSOR_TABLE   = p.getInt("pSNTABLE", DEFAULT_SENSOR_TABLE);
        EMERGENCY      = p.getInt("pEMERG",   DEFAULT_EMERGENCY);
        PEDAL          = p.getInt("pPEDAL",   DEFAULT_PEDAL);

        for (int i = 0; i < 8; i++) {
            inRowNc[i] = p.getBool(("pINROW" + String(i)).c_str(), false);
            int pinVal = p.getInt(("pINRP" + String(i)).c_str(), -999);
            if (pinVal == -999)
                inRowPin[i] = inRowNc[i] ? PIN_NC : defInRowPin[i];
            else
                inRowPin[i] = pinVal;
            int s = p.getInt(("pINRS" + String(i)).c_str(), -999);
            inRowSilk[i] = (s >= 0 && s <= 7) ? s : i;
        }

        for (int i = 0; i < 8; i++) {
            if (inRowNc[i]) {
                inRowPin[i] = PIN_NC;
                p.putInt(("pINRP" + String(i)).c_str(), PIN_NC);
                continue;
            }
            int gp = gpio_for_input_silk(inRowSilk[i]);
            if (gp != PIN_NC && inRowPin[i] != gp) {
                inRowPin[i] = gp;
                p.putInt(("pINRP" + String(i)).c_str(), gp);
            }
        }
        p.end();
    }

    // قطعِ مستقلِ یک ردیفِ ورودی (I0/I1 روی یک ADC — بدون قطعِ ردیفِ دیگر)
    void saveInRowNc(int row, bool nc)
    {
        if (row < 0 || row > 7) return;
        put_bool_safe(("pINROW" + String(row)).c_str(), nc);
        inRowNc[row] = nc;
    }

    void saveInRowPin(int row, int pin)
    {
        if (row < 0 || row > 7) return;
        put_int_safe(("pINRP" + String(row)).c_str(), pin);
        inRowPin[row] = pin;
    }

    void saveInRowSilk(int row, int silk)
    {
        if (row < 0 || row > 7) return;
        if (silk < 0 || silk > 7) silk = row;
        put_int_safe(("pINRS" + String(row)).c_str(), silk);
        inRowSilk[row] = silk;
        // اگر ردیف وصل است، GPIO را از روی همان I قفل کن (نه مقدار قدیمیِ ناهم‌خوان)
        if (!inRowNc[row]) {
            int gp = gpio_for_input_silk(silk);
            if (gp != PIN_NC) {
                put_int_safe(("pINRP" + String(row)).c_str(), gp);
                inRowPin[row] = gp;
            }
        }
    }

    void clearAllInRowNc()
    {
        for (int i = 0; i < 8; i++) {
            put_bool_safe(("pINROW" + String(i)).c_str(), false);
            inRowNc[i] = false;
        }
    }

    void clearAllInRowPins()
    {
        static const int defInRowPin[8] = {
            DEFAULT_SENSOR_ELEMENT, DEFAULT_SENSOR_ELEMENT,
            DEFAULT_SENSOR_SHABLON, DEFAULT_SENSOR_SHABLON,
            DEFAULT_SENSOR_TABLE,   DEFAULT_SENSOR_TABLE,
            DEFAULT_EMERGENCY,      DEFAULT_PEDAL
        };
        for (int i = 0; i < 8; i++) {
            inRowPin[i] = defInRowPin[i];
            inRowSilk[i] = i;
            put_int_safe(("pINRP" + String(i)).c_str(), defInRowPin[i]);
            put_int_safe(("pINRS" + String(i)).c_str(), i);
        }
    }

    // ذخیرهٔ پایهٔ جدیدِ یک ورودی/خروجی (کلیدها بالا). PIN_NC (-1) هم مجاز است («وصل نیست»).
    void savePinMapping(const char *key, int pin)
    {
        put_int_safe(key, pin);
    }

    // آخرین نام/اسلاتی که روی این GPIO سیم‌کشی شده بود (برای لیست پایه‌های آزاد)
    void savePinLastOwner(int gpio, int slot)
    {
        if (gpio < 0 || gpio > 48 || slot < 0 || slot > 12) return;
        put_int_safe(("pPLST" + String(gpio)).c_str(), slot);
    }

    int getPinLastOwner(int gpio, int fallbackSlot)
    {
        if (gpio < 0 || gpio > 48) return fallbackSlot;
        Preferences p;
        if (p.begin(namespaceName, true)) {
            int v = p.getInt(("pPLST" + String(gpio)).c_str(), fallbackSlot);
            p.end();
            return v;
        }
        return fallbackSlot;
    }

    void load()
    {
        Preferences p;
        p.begin(namespaceName, true);
        version = p.getString("version", "1.0.0");
        firmware_partion_select = p.getInt("fw_part_sel", 0);
        SelectedUser = p.getInt("SeUser", 1);

        for (int i = 0; i < 5; i++)
        {
            WildingTime[i] = clampProcessTime(p.getInt(("WT" + String(i + 1)).c_str(), WildingTime[i]));
            CoolingTime[i] = clampProcessTime(p.getInt(("CT" + String(i + 1)).c_str(), CoolingTime[i]));
            TempTarget[i] = clampTargetTemp(p.getInt(("TT" + String(i + 1)).c_str(), TempTarget[i]));
        }

        TempCaliber = p.getInt("TpCal", 0);
        pid = p.getInt("pid", 15);
        Speaker = p.getBool("Speaker", true);
        SpeakerVolume = (uint8_t)p.getInt("SpkVol", 80);
        if (SpeakerVolume > 100) SpeakerVolume = 80;
        g_buzzer_volume = SpeakerVolume;
        AutoStart = p.getBool("AStart", true);
        FinalJack = p.getBool("FJack", true);
        TempLimit = p.getBool("TpLimit", true);
        IsTreePedal = p.getBool("IsTreePedal", true);
        WifiOn = p.getBool("WifiOn", true);
        WifiSsid = p.getString("WifiSsid", "");
        WifiPassword = p.getString("WifiPass", "");
        p.end();

        loadWifi();   // بارگذاری شبکه‌های ذخیره‌شدهٔ چندتایی

        p.begin(namespaceName, true);
        SplashAlternate = p.getBool("SplashAlt", false);
        ScreensaverMin = p.getInt("ScrSvMin", 30);
        SysInfoBarEnabled = p.getBool("SysInfoBar", false);

        Is_J_type = p.getBool("IsJtype", true);
        EmergencySwich = p.getBool("EmySh", true);
        SensorCheck = p.getBool("SensCk", false);

        ReverseTimer = p.getInt("RsTr", 0);
        AdminPassword = p.getString("AdPs", "123456");

        CountdownEndJDN = p.getLong("CdEnd", 0);
        CountdownEnabled = p.getBool("CdEn", false);
        TempUnlockUsed = p.getBool("TuUsed", false);
        TempUnlockEndJDN = p.getLong("TuEnd", 0);
        LastSeenJDN = p.getLong("LastJDN", 0);
        ActiveTimerIndex = p.getUChar("MtAct", 0);
        ChainStartJDN = p.getLong("MtStart", 0);
        for (int i = 0; i < 4; i++)
            MultiTimerDays[i] = p.getInt(("MtD" + String(i + 2)).c_str(), 0);
        if (ActiveTimerIndex > 5) {
            ActiveTimerIndex = 5;
        }
        for (int i = 0; i < 4; i++) {
            if (MultiTimerDays[i] < 0) MultiTimerDays[i] = 0;
            if (MultiTimerDays[i] > 50) MultiTimerDays[i] = 50;
        }
        AdminLockUntilEpoch = p.getULong("AdLkUntil", 0);
        Lang = p.getInt("Lang", 1);

        jack_safhe_raft = p.getInt("JASARA", 600);
        jack_safhe_bargasht = p.getInt("JASABA", 1000);

        jack_shablon_raft = p.getInt("JASHRA", 1200);
        jack_shablon_bargasht = p.getInt("JASHBA", 1200);

        jack_prifil_raft = p.getInt("JAPRRA", 3000);
        jack_prifil_bargasht = p.getInt("JAPRBA", 6000);

        jack_josh_raft = p.getInt("JAJORA", 2100);
        jack_josh_bargasht = p.getInt("JAJOBA", 2100);

        jack_zarbe_1_raft = p.getInt("JAZ1RA", 0);
        jack_zarbe_2_raft = p.getInt("JAZ2RA", 0);
        frameNumbers = p.getInt("FRAMENUMBER", 0);
        frameNumbersCur = p.getInt("FRAMECUR", 0);
        dailyFrameNumbers = p.getInt("DAFRAMEN", 0);
        dailyWeldTimeMs = p.getULong("DAWELDT", 0);
        dailyStatsJDN = p.getLong("DASTATJD", 0);
        WarrantyDate = p.getString("WarrDate", "");
        CompanyName = p.getString("CoName", "");
        CompanyAsked = p.getBool("CoAsk", false);
        AdminAsked = p.getBool("AdAsk", false);
        ManufacturerPhone = p.getString("MfgPhone", "");
        MfgPhoneAsked = p.getBool("MfgPhAsk", false);
        p.end();
    }

    void save()
    {
        Preferences p;
        p.begin(namespaceName, false);
        p.putString("version", version);
        p.putInt("fw_part_sel", firmware_partion_select);
        p.putInt("SeUser", SelectedUser);

        for (int i = 0; i < 5; i++)
        {
            p.putInt(("WT" + String(i + 1)).c_str(), WildingTime[i]);
            p.putInt(("CT" + String(i + 1)).c_str(), CoolingTime[i]);
            p.putInt(("TT" + String(i + 1)).c_str(), TempTarget[i]);
        }

        p.putInt("TpCal", TempCaliber);
        p.putInt("pid", pid);

        p.putBool("Speaker", Speaker);
        p.putInt("SpkVol", (int)SpeakerVolume);
        p.putBool("AStart", AutoStart);
        p.putBool("FJack", FinalJack);
        p.putBool("TpLimit", TempLimit);
        p.putBool("IsTreePedal", IsTreePedal);
        p.putBool("WifiOn", WifiOn);

        p.putBool("IsJtype", Is_J_type);
        p.putBool("EmySh", EmergencySwich);
        p.putBool("SensCk", SensorCheck);

        p.putInt("RsTr", ReverseTimer);
        p.putString("AdPs", AdminPassword);

        p.putLong("CdEnd", CountdownEndJDN);
        p.putBool("TuUsed", TempUnlockUsed);
        p.putLong("TuEnd", TempUnlockEndJDN);
        p.putLong("LastJDN", LastSeenJDN);
        p.putUChar("MtAct", ActiveTimerIndex);
        p.putLong("MtStart", ChainStartJDN);
        for (int i = 0; i < 4; i++)
            p.putInt(("MtD" + String(i + 2)).c_str(), MultiTimerDays[i]);
        p.putULong("AdLkUntil", AdminLockUntilEpoch);
        p.putInt("Lang", Lang);

        p.putInt("JASARA", jack_safhe_raft);
        p.putInt("JASABA", jack_safhe_bargasht);

        p.putInt("JASHRA", jack_shablon_raft);
        p.putInt("JASHBA", jack_shablon_bargasht);

        p.putInt("JAPRRA", jack_prifil_raft);
        p.putInt("JAPRBA", jack_prifil_bargasht);

        p.putInt("JAJORA", jack_josh_raft);
        p.putInt("JAJOBA", jack_josh_bargasht);
        p.putInt("FRAMENUMBER", frameNumbers);
        p.putInt("DAFRAMEN", dailyFrameNumbers);
        p.putULong("DAWELDT", dailyWeldTimeMs);
        p.putLong("DASTATJD", dailyStatsJDN);

        p.putInt("JAZ1RA", jack_zarbe_1_raft);
        p.putInt("JAZ2RA", jack_zarbe_2_raft);
        p.putString("WarrDate", WarrantyDate);
        p.putString("CoName", CompanyName);
        p.putBool("CoAsk", CompanyAsked);
        p.putBool("AdAsk", AdminAsked);
        p.putString("MfgPhone", ManufacturerPhone);
        p.putBool("MfgPhAsk", MfgPhoneAsked);
        p.end();
    }

    // ===== نام شرکت =====
    String getCompanyName() { return CompanyName; }
    bool isCompanyAsked() { return CompanyAsked; }
    void setCompanyName(const String &n) {
        CompanyName = n;
        CompanyAsked = true;
        put_string_safe("CoName", CompanyName);
        put_bool_safe("CoAsk", CompanyAsked);
    }
    void markCompanyAsked() {
        CompanyAsked = true;
        put_bool_safe("CoAsk", CompanyAsked);
    }
    bool isAdminAsked() { return AdminAsked; }
    void markAdminAsked() {
        AdminAsked = true;
        put_bool_safe("AdAsk", AdminAsked);
    }

    // ===== شماره سازنده دستگاه و احراز پیامکی =====
    String getManufacturerPhone() { return ManufacturerPhone; }
    bool isMfgPhoneAsked() { return MfgPhoneAsked; }
    void setManufacturerPhone(const String &ph) {
        ManufacturerPhone = ph;
        MfgPhoneAsked = true;
        put_string_safe("MfgPhone", ManufacturerPhone);
        put_bool_safe("MfgPhAsk", MfgPhoneAsked);
    }
    void markMfgPhoneAsked() {
        MfgPhoneAsked = true;
        put_bool_safe("MfgPhAsk", MfgPhoneAsked);
    }

    // ----- Getters -----
    String getVersion() const { return version; }
    int getFirmwarePartionSelect() const { return firmware_partion_select; }

    int getSelectedUser()
    {
        return SelectedUser;
    }
    int getWildingTime(const int userNumber)
    {
        return clampProcessTime(WildingTime[userNumber - 1]);
    }
    int getCoolingTime(const int userNumber)
    {
        return clampProcessTime(CoolingTime[userNumber - 1]);
    }
    int getTempTarget(const int userNumber)
    {
        return clampTargetTemp(TempTarget[userNumber - 1]);
    }
    int getTempCaliber()
    {
        return TempCaliber;
    }
    int getPID()
    {
        return pid;
    }
    bool getSpeaker()
    {
        return Speaker;
    }
    uint8_t getSpeakerVolume() const
    {
        return SpeakerVolume;
    }
    bool getAutoStart()
    {
        return AutoStart;
    }
    bool getFinalJack()
    {
        return FinalJack;
    }
    bool getTempLimit()
    {
        return TempLimit;
    }
    bool getIsTreePedal()
    {
        return IsTreePedal;
    }
    bool getWifiOn()
    {
        return WifiOn;
    }
    void setWifiOn(const bool &data)
    {
        WifiOn = data;
        put_bool_safe("WifiOn", WifiOn);
    }
    int getScreensaverMin()
    {
        return ScreensaverMin;
    }
    void setScreensaverMin(int minutes)
    {
        ScreensaverMin = minutes;
        put_int_safe("ScrSvMin", ScreensaverMin);
    }
    bool getSysInfoBarEnabled()
    {
        return SysInfoBarEnabled;
    }
    void setSysInfoBarEnabled(bool en)
    {
        SysInfoBarEnabled = en;
        put_bool_safe("SysInfoBar", SysInfoBarEnabled);
    }
    String getWifiSsid()
    {
        return WifiSsid;
    }
    String getWifiPassword()
    {
        return WifiPassword;
    }
    void setWifiCredentials(const String &ssid, const String &password)
    {
        WifiSsid = ssid;
        WifiPassword = password;
        put_string_safe("WifiSsid", WifiSsid);
        put_string_safe("WifiPass", WifiPassword);
    }

    // ===== متدهای چند-شبکهٔ وای‌فای =====
    void loadWifi()
    {
        Preferences p;
        if (p.begin(namespaceName, true)) {
            wifi_count = p.getInt("wc", 0);
            if (wifi_count < 0) wifi_count = 0;
            if (wifi_count > WIFI_STORE_MAX) wifi_count = WIFI_STORE_MAX;
            for (int i = 0; i < wifi_count; i++)
            {
                wifi_ssids[i] = p.getString((String("ws") + i).c_str(), "");
                wifi_pwds[i]  = p.getString((String("wp") + i).c_str(), "");
            }
            p.end();
        }
        if (wifi_count == 0 && WifiSsid.length() > 0)
        {
            wifi_ssids[0] = WifiSsid;
            wifi_pwds[0]  = WifiPassword;
            wifi_count = 1;
            persistWifi();
        }
        // همیشه بررسی کن آیا شبکهٔ جدیدی از بستنی یا چونه قابل ایمپورت است
        importWifiFromInstaller();

        if (!hasWifi("silver") || getWifiPassword("silver") != "1234567A")
        {
            saveWifi("silver", "1234567A");
        }
        if (WifiSsid.length() == 0)
        {
            setWifiCredentials("silver", "1234567A");
        }
    }

    void importWifiFromInstaller()
    {
        // ۱. بررسی فضای نام app (استفاده‌شده در پروژه بستنی و پروژه‌های ESP-IDF)
        Preferences appCfg;
        if (appCfg.begin("app", true)) {
            String appSsid = appCfg.getString("wifi_sta_ssid", "");
            String appPass = appCfg.getString("wifi_sta_pass", "");
            appCfg.end();
            if (appSsid.length() > 0) {
                saveWifi(appSsid, appPass);
                if (WifiSsid.length() == 0 || WifiSsid == "silver") {
                    setWifiCredentials(appSsid, appPass);
                }
                Serial.printf("[STORAGE] Imported WiFi from 'app' namespace: ssid='%s'\n", appSsid.c_str());
            }
        }

        // ۲. بررسی فضای نام chone (پروژه‌های نصب و چونه‌گیر)
        Preferences cfg;
        if (!cfg.begin("chone", true)) return;
        int cnt = cfg.getInt("wc", 0);
        if (cnt < 0) cnt = 0;
        if (cnt > WIFI_STORE_MAX) cnt = WIFI_STORE_MAX;
        String lastSsid = "", lastPwd = "";
        for (int i = 0; i < cnt; i++)
        {
            String ssid = cfg.getString((String("ws") + i).c_str(), "");
            String pwd  = cfg.getString((String("wp") + i).c_str(), "");
            if (ssid.length() == 0) continue;
            saveWifi(ssid, pwd);
            lastSsid = ssid;
            lastPwd = pwd;
        }
        cfg.end();
        if (lastSsid.length() > 0 && (WifiSsid.length() == 0 || WifiSsid == "silver")) {
            setWifiCredentials(lastSsid, lastPwd);
        }
    }

    void persistWifi()
    {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putInt("wc", wifi_count);
            for (int i = 0; i < wifi_count; i++)
            {
                p.putString((String("ws") + i).c_str(), wifi_ssids[i]);
                p.putString((String("wp") + i).c_str(), wifi_pwds[i]);
            }
            p.end();
        }
    }

    bool hasWifi(const String &ssid) const
    {
        for (int i = 0; i < wifi_count; i++)
            if (wifi_ssids[i] == ssid) return true;
        return false;
    }

    String getWifiPassword(const String &ssid) const
    {
        for (int i = 0; i < wifi_count; i++)
            if (wifi_ssids[i] == ssid) return wifi_pwds[i];
        return String("");
    }

    void saveWifi(const String &ssid, const String &pwd)
    {
        if (ssid.length() == 0) return;
        for (int i = 0; i < wifi_count; i++)
        {
            if (wifi_ssids[i] == ssid) { wifi_pwds[i] = pwd; persistWifi(); return; }
        }
        if (wifi_count < WIFI_STORE_MAX)
        {
            wifi_ssids[wifi_count] = ssid;
            wifi_pwds[wifi_count] = pwd;
            wifi_count++;
        }
        else
        {
            for (int i = 1; i < WIFI_STORE_MAX; i++)
            {
                wifi_ssids[i - 1] = wifi_ssids[i];
                wifi_pwds[i - 1] = wifi_pwds[i];
            }
            wifi_ssids[WIFI_STORE_MAX - 1] = ssid;
            wifi_pwds[WIFI_STORE_MAX - 1] = pwd;
        }
        persistWifi();
    }

    bool consumeSplashAlternate()
    {
        bool current = SplashAlternate;
        SplashAlternate = !SplashAlternate;
        put_bool_safe("SplashAlt", SplashAlternate);
        return current;
    }

    bool getIs_J_type() const { return Is_J_type; }
    bool getEmergencySwich() const { return EmergencySwich; }
    bool getSensorCheck() const { return SensorCheck; }
    int getReverseTimer() const { return ReverseTimer; }
    String getAdminPassword() const { return AdminPassword; }

    int get_Jack_safhe_raft() const { return jack_safhe_raft; }
    int get_Jack_safhe_bargasht() const { return jack_safhe_bargasht; }
    int get_Jack_shablon_raft() const { return jack_shablon_raft; }
    int get_Jack_shablon_bargasht() const { return jack_shablon_bargasht; }
    int get_Jack_prifil_raft() const { return jack_prifil_raft; }
    int get_Jack_prifil_bargasht() const { return jack_prifil_bargasht; }
    int get_Jack_josh_raft() const { return jack_josh_raft; }
    int get_Jack_josh_bargasht() const { return jack_josh_bargasht; }
    int get_frameNumbers() const { return frameNumbers; }
    int get_Jack_zarbe_1_raft() const { return jack_zarbe_1_raft; }
    int get_Jack_zarbe_2_raft() const { return jack_zarbe_2_raft; }

    // ----- Setters (with auto-save) -----
    void setVersion(const String &v)
    {
        version = v;
        put_string_safe("version", version);
    }
    void setFirmwarePartionSelect(const int &v)
    {
        firmware_partion_select = v;
        put_int_safe("fw_part_sel", firmware_partion_select);
    }
    void setSelectedUser(const int &v)
    {
        SelectedUser = v;
        put_int_safe("SeUser", SelectedUser);
    }
    void setWildingTime(const int &data, const int userNumber)
    {
        const int safeValue = clampProcessTime(data);
        if (userNumber >= 1 && userNumber <= 5) {
            WildingTime[userNumber - 1] = safeValue;
            put_int_safe(("WT" + String(userNumber)).c_str(), safeValue);
        }
    }
    void setCoolingTime(const int &data, const int userNumber)
    {
        const int safeValue = clampProcessTime(data);
        if (userNumber >= 1 && userNumber <= 5) {
            CoolingTime[userNumber - 1] = safeValue;
            put_int_safe(("CT" + String(userNumber)).c_str(), safeValue);
        }
    }
    void setTempTarget(const int &data, const int userNumber)
    {
        const int safeValue = clampTargetTemp(data);
        if (userNumber >= 1 && userNumber <= 5) {
            TempTarget[userNumber - 1] = safeValue;
            put_int_safe(("TT" + String(userNumber)).c_str(), safeValue);
        }
    }
    void setTempCaliber(const int &data)
    {
        TempCaliber = data;
        put_int_safe("TpCal", TempCaliber);
    }
    void setPID(const int &data)
    {
        pid = data;
        put_int_safe("pid", pid);
    }
    void setSpeaker(const bool &data)
    {
        Speaker = data;
        put_bool_safe("Speaker", Speaker);
    }
    void setSpeakerVolume(uint8_t vol)
    {
        if (vol > 100) vol = 100;
        SpeakerVolume = vol;
        g_buzzer_volume = vol;
        put_int_safe("SpkVol", (int)SpeakerVolume);
    }
    void setAutoStart(const bool &data)
    {
        AutoStart = data;
        put_bool_safe("AStart", AutoStart);
    }
    void setFinalJack(const bool &data)
    {
        FinalJack = data;
        put_bool_safe("FJack", FinalJack);
    }
    void setTempLimit(const bool &data)
    {
        TempLimit = data;
        put_bool_safe("TpLimit", TempLimit);
    }
    void setIsTreePedal(const bool &data)
    {
        IsTreePedal = data;
        put_bool_safe("IsTreePedal", IsTreePedal);
    }
    void setIs_J_type(const bool &data)
    {
        Is_J_type = data;
        put_bool_safe("IsJtype", Is_J_type);
    }
    void setEmergencySwich(const bool &data)
    {
        EmergencySwich = data;
        put_bool_safe("EmySh", EmergencySwich);
    }
    void setSensorCheck(const bool &data)
    {
        SensorCheck = data;
        put_bool_safe("SensCk", SensorCheck);
    }
    void setReverseTimer(const int &data)
    {
        ReverseTimer = data;
        put_int_safe("RsTr", ReverseTimer);
    }
    void setAdminPassword(const String &data)
    {
        AdminPassword = data;
        put_string_safe("AdPs", AdminPassword);
    }

    // ===== سیستم قفل/تایمر معکوس =====
    static long gregorianToJDN(int y, int m, int d)
    {
        long a = (14 - m) / 12;
        long yy = y + 4800 - a;
        long mm = m + 12 * a - 3;
        return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
    }

    static void jdnToGregorian(long jdn, int &y, int &m, int &d)
    {
        long a = jdn + 32044;
        long b = (4 * a + 3) / 146097;
        long c = a - (146097 * b) / 4;
        long e = (4 * c + 3) / 1461;
        long f = c - (1461 * e) / 4 + 31;
        long g = (80 * f) / 2447;
        d = (int)(f - (2447 * g) / 80);
        m = (int)(g + 2 - 12 * (g / 10));
        y = (int)(100 * b + e - 4800 + g / 10);
    }

    uint8_t getActiveTimerIndex() { return ActiveTimerIndex; }
    long getChainStartJDN() { return ChainStartJDN; }

    int getTimerConfigDays(int timerNum)
    {
        if (timerNum == 1) return ReverseTimer;
        if (timerNum >= 2 && timerNum <= 5) return MultiTimerDays[timerNum - 2];
        return 0;
    }

    int getMultiTimerDay(int timerNum)
    {
        if (timerNum < 2 || timerNum > 5) return 0;
        return MultiTimerDays[timerNum - 2];
    }

    void setMultiTimerDay(int timerNum, int days)
    {
        if (timerNum < 2 || timerNum > 5) return;
        if (days < 0) days = 0;
        if (days > 50) days = 50;
        MultiTimerDays[timerNum - 2] = days;
        put_int_safe(("MtD" + String(timerNum)).c_str(), days);
    }

    void persistMultiTimerMeta()
    {
        put_uchar_safe("MtAct", ActiveTimerIndex);
        put_long_safe("MtStart", ChainStartJDN);
    }

    void startTimerChain(int t1Days, long todayJDN)
    {
        ActiveTimerIndex = (t1Days > 0) ? 1 : 0;
        ChainStartJDN = (t1Days > 0 && todayJDN > 0) ? todayJDN : 0;
        startCountdown(t1Days, todayJDN);
        persistMultiTimerMeta();
    }

    bool advanceTimerChain(long todayJDN)
    {
        if (todayJDN <= 0 || ActiveTimerIndex < 1) return false;
        int next = (int)ActiveTimerIndex + 1;
        while (next <= 5) {
            int days = getTimerConfigDays(next);
            if (days > 0) {
                ActiveTimerIndex = (uint8_t)next;
                CountdownEndJDN = todayJDN + days;
                CountdownEnabled = true;
                put_long_safe("CdEnd", CountdownEndJDN);
                put_bool_safe("CdEn", CountdownEnabled);
                persistMultiTimerMeta();
                return true;
            }
            next++;
        }
        clearLockAndCountdown();
        return true;
    }

    long previewLockJDN(int timerNum)
    {
        if (ChainStartJDN <= 0 || timerNum < 1 || timerNum > 5) return 0;
        long sum = 0;
        for (int i = 1; i <= timerNum; i++)
            sum += getTimerConfigDays(i);
        if (sum <= 0) return 0;
        return ChainStartJDN + sum;
    }

    int previewCumulativeDays(int timerNum)
    {
        int sum = 0;
        for (int i = 1; i <= timerNum && i <= 5; i++)
            sum += getTimerConfigDays(i);
        return sum;
    }

    long getCountdownEndJDN() { return CountdownEndJDN; }
    bool getTempUnlockUsed() { return TempUnlockUsed; }
    long getLastSeenJDN() { return LastSeenJDN; }
    void resetLastSeenJDN(long todayJDN) {
        LastSeenJDN = todayJDN;
        put_long_safe("LastJDN", LastSeenJDN);
    }
    bool countdownActive() { return CountdownEndJDN != 0; }
    bool isCountdownEnabled() { return CountdownEnabled; }
    void setCountdownEnabled(bool en) {
        CountdownEnabled = en;
        put_bool_safe("CdEn", CountdownEnabled);
    }

    bool noteRtcDay(long todayJDN)
    {
        if (todayJDN <= 0) return false;
        if (LastSeenJDN != 0 && todayJDN < LastSeenJDN) return false;
        if (todayJDN > LastSeenJDN) {
            LastSeenJDN = todayJDN;
            put_long_safe("LastJDN", LastSeenJDN);
        }
        return true;
    }

    void startCountdown(int days, long todayJDN)
    {
        CountdownEndJDN = (days > 0) ? (todayJDN + days) : 0;
        CountdownEnabled = (days > 0);
        ReverseTimer = (days > 0) ? days : 0;
        TempUnlockUsed = false;
        TempUnlockEndJDN = 0;
        if (days <= 0) {
            ActiveTimerIndex = 0;
            ChainStartJDN = 0;
        }
        if (todayJDN > LastSeenJDN) LastSeenJDN = todayJDN;
        put_long_safe("CdEnd", CountdownEndJDN);
        put_bool_safe("CdEn", CountdownEnabled);
        put_int_safe("RsTr", ReverseTimer);
        put_bool_safe("TuUsed", TempUnlockUsed);
        put_long_safe("TuEnd", TempUnlockEndJDN);
        put_long_safe("LastJDN", LastSeenJDN);
        persistMultiTimerMeta();
    }

    void clearLockAndCountdown()
    {
        CountdownEndJDN = 0;
        CountdownEnabled = false;
        ReverseTimer = 0;
        TempUnlockUsed = false;
        TempUnlockEndJDN = 0;
        ActiveTimerIndex = 0;
        ChainStartJDN = 0;
        for (int i = 0; i < 4; i++) {
            MultiTimerDays[i] = 0;
            put_int_safe(("MtD" + String(i + 2)).c_str(), 0);
        }
        put_long_safe("CdEnd", CountdownEndJDN);
        put_bool_safe("CdEn", CountdownEnabled);
        put_int_safe("RsTr", ReverseTimer);
        put_bool_safe("TuUsed", TempUnlockUsed);
        put_long_safe("TuEnd", TempUnlockEndJDN);
        persistMultiTimerMeta();
    }

    bool isLockedNow(long todayJDN)
    {
        if (CountdownEndJDN == 0) return false;
        if (todayJDN < CountdownEndJDN) return false;
        if (TempUnlockUsed && todayJDN < TempUnlockEndJDN) return false;
        return true;
    }

    int remainingDays(long todayJDN)
    {
        if (CountdownEndJDN == 0) return -1;
        if (todayJDN < CountdownEndJDN) return (int)(CountdownEndJDN - todayJDN);
        if (TempUnlockUsed && todayJDN < TempUnlockEndJDN) return (int)(TempUnlockEndJDN - todayJDN);
        return 0;
    }

    bool useTempUnlock(long todayJDN)
    {
        if (TempUnlockUsed || todayJDN <= 0) return false;
        TempUnlockUsed = true;
        CountdownEndJDN = todayJDN + 10;
        TempUnlockEndJDN = todayJDN + 10;
        put_bool_safe("TuUsed", TempUnlockUsed);
        put_long_safe("TuEnd", TempUnlockEndJDN);
        put_long_safe("CdEnd", CountdownEndJDN);
        put_long_safe("LastJDN", LastSeenJDN);
        return true;
    }

    int getLang() const { return (Lang >= 1 && Lang <= 3) ? Lang : 1; }
    void setLang(int v) { if (v < 1 || v > 3) v = 1; Lang = v; put_int_safe("Lang", Lang); }

    unsigned long getAdminLockUntilEpoch() { return AdminLockUntilEpoch; }

    void lockAdminFor(unsigned long nowEpoch, unsigned long seconds)
    {
        AdminLockUntilEpoch = nowEpoch + seconds;
        put_ulong_safe("AdLkUntil", AdminLockUntilEpoch);
    }

    void refreshAdminLockOnBoot(unsigned long nowEpoch, unsigned long seconds)
    {
        if (AdminLockUntilEpoch != 0 && nowEpoch < AdminLockUntilEpoch) {
            AdminLockUntilEpoch = nowEpoch + seconds;
            put_ulong_safe("AdLkUntil", AdminLockUntilEpoch);
        }
    }

    void clearAdminLock()
    {
        AdminLockUntilEpoch = 0;
        put_ulong_safe("AdLkUntil", AdminLockUntilEpoch);
    }

    bool isAdminLocked(unsigned long nowEpoch)
    {
        if (AdminLockUntilEpoch == 0) return false;
        if (nowEpoch >= AdminLockUntilEpoch) return false;
        return true;
    }

    unsigned long adminLockRemainingSec(unsigned long nowEpoch)
    {
        if (AdminLockUntilEpoch == 0 || nowEpoch >= AdminLockUntilEpoch) return 0;
        return AdminLockUntilEpoch - nowEpoch;
    }

    void set_jack_safhe_raft(const int &data)
    {
        jack_safhe_raft = data;
        put_int_safe("JASARA", jack_safhe_raft);
    }
    void set_jack_safhe_bargasht(const int &data)
    {
        jack_safhe_bargasht = data;
        put_int_safe("JASABA", jack_safhe_bargasht);
    }

    void set_jack_shablon_raft(const int &data)
    {
        jack_shablon_raft = data;
        put_int_safe("JASHRA", jack_shablon_raft);
    }
    void set_jack_shablon_bargasht(const int &data)
    {
        jack_shablon_bargasht = data;
        put_int_safe("JASHBA", jack_shablon_bargasht);
    }

    void set_jack_prifil_raft(const int &data)
    {
        jack_prifil_raft = data;
        put_int_safe("JAPRRA", jack_prifil_raft);
    }
    void set_jack_prifil_bargasht(const int &data)
    {
        jack_prifil_bargasht = data;
        put_int_safe("JAPRBA", jack_prifil_bargasht);
    }

    void set_jack_josh_raft(const int &data)
    {
        jack_josh_raft = data;
        put_int_safe("JAJORA", jack_josh_raft);
    }
    void set_jack_josh_bargasht(const int &data)
    {
        jack_josh_bargasht = data;
        put_int_safe("JAJOBA", jack_josh_bargasht);
    }

    void set_frameNumbers(const int &data)
    {
        frameNumbers = data;
        put_int_safe("FRAMENUMBER", frameNumbers);
    }
    int get_frameNumbersCur() { return frameNumbersCur; }
    void set_frameNumbersCur(const int &data)
    {
        frameNumbersCur = data;
        put_int_safe("FRAMECUR", frameNumbersCur);
    }
    void syncDailyStats(long todayJDN)
    {
        if (todayJDN != 0 && dailyStatsJDN != todayJDN) {
            dailyFrameNumbers = 0;
            dailyWeldTimeMs = 0;
            dailyStatsJDN = todayJDN;
            save();
        }
    }
    void addDailyWeldStats(long todayJDN, uint32_t weldMs, int frames)
    {
        syncDailyStats(todayJDN);
        dailyFrameNumbers += frames;
        dailyWeldTimeMs += weldMs;
        dailyStatsJDN = todayJDN;
        save();
    }
    int getDailyFrameNumbers() const { return dailyFrameNumbers; }
    unsigned long getDailyWeldTimeMs() const { return dailyWeldTimeMs; }

    void set_jack_zarbe_1_raft(const int &data)
    {
        jack_zarbe_1_raft = data;
        put_int_safe("JAZ1RA", jack_zarbe_1_raft);
    }
    void set_jack_zarbe_2_raft(const int &data)
    {
        jack_zarbe_2_raft = data;
        put_int_safe("JAZ2RA", jack_zarbe_2_raft);
    }

    // ===== گارانتی =====
    String getWarrantyDate() { return WarrantyDate; }
    bool isWarrantyActive() { return WarrantyDate.length() > 0; }
    void setWarrantyDate(const String &d) {
        WarrantyDate = d;
        put_string_safe("WarrDate", WarrantyDate);
    }
    void consumeWarrantyClearOnce(uint8_t token) {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            if (p.getUChar("warrClrTok", 0) != token) {
                WarrantyDate = "";
                p.remove("WarrDate");
                p.putUChar("warrClrTok", token);
            }
            p.end();
        }
    }
    bool consumeClockForceSetOnce(uint8_t token) {
        Preferences p;
        bool res = false;
        if (p.begin(namespaceName, false)) {
            if (p.getUChar("clkForceTok", 0) != token) {
                p.putUChar("clkForceTok", token);
                res = true;
            }
            p.end();
        }
        return res;
    }
    bool consumeFirstBootResetOnce(uint8_t token) {
        Preferences p;
        bool res = false;
        if (p.begin(namespaceName, false)) {
            if (p.getUChar("fbResetTok", 0) != token) {
                p.putUChar("fbResetTok", token);
                CompanyName = "";
                CompanyAsked = false;
                AdminAsked = false;
                ManufacturerPhone = "";
                MfgPhoneAsked = false;
                p.putString("CoName", CompanyName);
                p.putBool("CoAsk", CompanyAsked);
                p.putBool("AdAsk", AdminAsked);
                p.putString("MfgPhone", ManufacturerPhone);
                p.putBool("MfgPhAsk", MfgPhoneAsked);
                res = true;
            }
            p.end();
        }
        return res;
    }

    // ===== Snapshot Â«Ù¾ÛŒØ´â€ŒÙØ±Ø¶Ù Ù…Ø¯ÛŒØ±Â» (Ø¢ÛŒØªÙ… Û±Û° Ùˆ Û±Û± Ù„ÛŒØ³Øª Ø§ØµÙ„Ø§Ø­Ø§Øª) =====
    // Ø°Ø®ÛŒØ±Ù‡Ù” ØªÙ†Ø¸ÛŒÙ…Ø§Øª Ø¹Ù…Ù„ÛŒØ§ØªÛŒÙ ÙØ¹Ù„ÛŒ Ø¯Ø± ÛŒÚ© namespace Ø¬Ø¯Ø§Ú¯Ø§Ù†Ù‡ Ø¨Ù‡â€ŒØ¹Ù†ÙˆØ§Ù† Â«Ù¾ÛŒØ´â€ŒÙØ±Ø¶Â».
    // Ø±ÛŒØ³Øªâ€ŒÙÚ©ØªÙˆØ±ÛŒ Ø¨Ø§ÛŒØ¯ Ø¨Ù‡ Ù‡Ù…ÛŒÙ† Ù…Ù‚Ø§Ø¯ÛŒØ± Ø¨Ø±Ú¯Ø±Ø¯Ø¯ØŒ Ù†Ù‡ Ø¨Ù‡ Ø§Ø¹Ø¯Ø§Ø¯ Ø«Ø§Ø¨ØªÙ Ø¯Ø§Ø®Ù„ Ú©Ø¯.
    void saveAsDefault()
    {
        Preferences def;
        def.begin("AJOSH_DEF", false);
        for (int i = 0; i < 5; i++)
        {
            def.putInt(("WT" + String(i + 1)).c_str(), WildingTime[i]);
            def.putInt(("CT" + String(i + 1)).c_str(), CoolingTime[i]);
            def.putInt(("TT" + String(i + 1)).c_str(), TempTarget[i]);
        }
        def.putInt("TpCal", TempCaliber);
        def.putInt("pid", pid);
        def.putInt("Lang", Lang);
        def.putBool("Speaker", Speaker);
        def.putInt("SpkVol", (int)SpeakerVolume);
        def.putBool("AStart", AutoStart);
        def.putBool("FJack", FinalJack);
        def.putBool("TpLimit", TempLimit);
        def.putBool("IsTreePedal", IsTreePedal);
        def.putBool("WifiOn", WifiOn);
        def.putBool("IsJtype", Is_J_type);
        def.putBool("EmySh", EmergencySwich);
        def.putBool("SensCk", SensorCheck);
        def.putInt("RsTr", 0); // پیش‌فرض تایمر معکوس همیشه ۰
        def.putInt("JASARA", jack_safhe_raft);
        def.putInt("JASABA", jack_safhe_bargasht);
        def.putInt("JASHRA", jack_shablon_raft);
        def.putInt("JASHBA", jack_shablon_bargasht);
        def.putInt("JAPRRA", jack_prifil_raft);
        def.putInt("JAPRBA", jack_prifil_bargasht);
        def.putInt("JAJORA", jack_josh_raft);
        def.putInt("JAJOBA", jack_josh_bargasht);
        def.putInt("JAZ1RA", jack_zarbe_1_raft);
        def.putInt("JAZ2RA", jack_zarbe_2_raft);
        def.putInt("ScrSvMin", ScreensaverMin);
        def.putBool("hasDef", true);
        def.end();
    }

    bool hasDefault()
    {
        Preferences def;
        def.begin("AJOSH_DEF", true);
        bool h = def.getBool("hasDef", false);
        def.end();
        return h;
    }

    // بازگردانی تنظیمات به «پیش‌فرض مدیر». اگر مدیر هرگز پیش‌فرض ذخیره نکرده باشد
    // false برمی‌گرداند و چیزی تغییر نمی‌کند (ایمن).
    bool restoreDefaults()
    {
        Preferences def;
        bool hasDef = false;
        if (def.begin("AJOSH_DEF", true)) {
            hasDef = def.getBool("hasDef", false);
        }

        if (hasDef) {
            for (int i = 0; i < 5; i++)
            {
                WildingTime[i] = clampProcessTime(def.getInt(("WT" + String(i + 1)).c_str(), WildingTime[i]));
                CoolingTime[i] = clampProcessTime(def.getInt(("CT" + String(i + 1)).c_str(), CoolingTime[i]));
                TempTarget[i] = clampTargetTemp(def.getInt(("TT" + String(i + 1)).c_str(), TempTarget[i]));
            }
            TempCaliber = def.getInt("TpCal", TempCaliber);
            pid = def.getInt("pid", pid);
            int defLang = def.getInt("Lang", Lang);
            Lang = (defLang >= 1 && defLang <= 3) ? defLang : 1;
            Speaker = def.getBool("Speaker", Speaker);
            SpeakerVolume = (uint8_t)def.getInt("SpkVol", 80);
            if (SpeakerVolume > 100) SpeakerVolume = 80;
            AutoStart = def.getBool("AStart", AutoStart);
            FinalJack = def.getBool("FJack", FinalJack);
            TempLimit = def.getBool("TpLimit", TempLimit);
            IsTreePedal = def.getBool("IsTreePedal", IsTreePedal);
            WifiOn = def.getBool("WifiOn", WifiOn);
            Is_J_type = def.getBool("IsJtype", Is_J_type);
            EmergencySwich = def.getBool("EmySh", EmergencySwich);
            SensorCheck = def.getBool("SensCk", SensorCheck);
            jack_safhe_raft = def.getInt("JASARA", jack_safhe_raft);
            jack_safhe_bargasht = def.getInt("JASABA", jack_safhe_bargasht);
            jack_shablon_raft = def.getInt("JASHRA", jack_shablon_raft);
            jack_shablon_bargasht = def.getInt("JASHBA", jack_shablon_bargasht);
            jack_prifil_raft = def.getInt("JAPRRA", jack_prifil_raft);
            jack_prifil_bargasht = def.getInt("JAPRBA", jack_prifil_bargasht);
            jack_josh_raft = def.getInt("JAJORA", jack_josh_raft);
            jack_josh_bargasht = def.getInt("JAJOBA", jack_josh_bargasht);
            jack_zarbe_1_raft = def.getInt("JAZ1RA", jack_zarbe_1_raft);
            jack_zarbe_2_raft = def.getInt("JAZ2RA", jack_zarbe_2_raft);
            int defScr = def.getInt("ScrSvMin", 30);
            setScreensaverMin(defScr);
            def.end();
        } else {
            def.end();
            int defaultWildingTime[5] = {25, 25, 25, 25, 25};
            int defaultCoolingTime[5] = {30, 30, 30, 30, 30};
            int defaultTempTarget[5]  = {250, 250, 250, 250, 250};
            for (int i = 0; i < 5; i++) {
                WildingTime[i] = defaultWildingTime[i];
                CoolingTime[i] = defaultCoolingTime[i];
                TempTarget[i] = defaultTempTarget[i];
            }
            TempCaliber = 0;
            pid = 15;
            Lang = 1;
            Speaker = true;
            SpeakerVolume = 80;
            AutoStart = false;
            FinalJack = false;
            TempLimit = false;
            IsTreePedal = false;
            WifiOn = true;
            Is_J_type = false;
            EmergencySwich = true;
            SensorCheck = true;
            jack_safhe_raft = 1000;
            jack_safhe_bargasht = 1000;
            jack_shablon_raft = 1000;
            jack_shablon_bargasht = 1000;
            jack_prifil_raft = 1000;
            jack_prifil_bargasht = 1000;
            jack_josh_raft = 1000;
            jack_josh_bargasht = 1000;
            jack_zarbe_1_raft = 12;
            jack_zarbe_2_raft = 12;
            setScreensaverMin(30);
        }
        save();
        return true;
    }

    uint32_t get_teflon_cycles()
    {
        Preferences p;
        uint32_t c = 0;
        if (p.begin(namespaceName, true)) {
            c = p.getUInt("TefCyc", 0);
            p.end();
        }
        return c;
    }
    void set_teflon_cycles(uint32_t cycles)
    {
        Preferences p;
        if (p.begin(namespaceName, false)) {
            p.putUInt("TefCyc", cycles);
            p.end();
        }
    }
};

#endif
