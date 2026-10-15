#include "a2_eeprom.h"
#include "EEPROM.h"
#include <string.h>
#include <math.h>

#define EEpromStartAddres 10
#define SavedId 0xea
static int Address = 0;

unsigned char EEprom1_EverSaved = 0;
unsigned char EEprom2_Language = 0;
bool EEprom3_Buzzer = true;
bool EEprom4_TouchAvailable = true;
bool EEprom5_MustSave = false;
char EEprom6_AdminPin[5] = "1234";
unsigned char EEprom7_OutputType = 0;

float EEprom_Process_Qs = 30.0f;
float EEprom_Process_P1 = 1.0f;
float EEprom_Process_P2 = 1.0f;

float EEprom_Glass2_Length = 0.0f;
float EEprom_Glass2_Height = 0.0f;
float EEprom_Glass2_Depth = 0.0f;
float EEprom_Glass1_Length = 0.0f;
float EEprom_Glass1_Height = 0.0f;
float EEprom_Glass1_Depth = 0.0f;

unsigned long EEprom_TotalFilled_Glass1 = 0;
unsigned long EEprom_TotalFilled_Glass2 = 0;
unsigned long EEprom_TotalSeconds_Glass1 = 0;
unsigned long EEprom_TotalSeconds_Glass2 = 0;

struct eeprom_shadow_t {
    unsigned char language;
    bool buzzer;
    bool touch;
    char pin[5];
    unsigned char output_type;
    float qs, p1, p2;
    float g2_l, g2_h, g2_d;
    float g1_l, g1_h, g1_d;
    unsigned long total_filled_g1;
    unsigned long total_filled_g2;
    unsigned long total_sec_g1;
    unsigned long total_sec_g2;
};
static eeprom_shadow_t s_shadow;
static bool s_shadow_valid = false;

static void update_shadow(void) {
    s_shadow.language = EEprom2_Language;
    s_shadow.buzzer = EEprom3_Buzzer;
    s_shadow.touch = EEprom4_TouchAvailable;
    memcpy(s_shadow.pin, EEprom6_AdminPin, 5);
    s_shadow.output_type = EEprom7_OutputType;
    s_shadow.qs = EEprom_Process_Qs;
    s_shadow.p1 = EEprom_Process_P1;
    s_shadow.p2 = EEprom_Process_P2;
    s_shadow.g2_l = EEprom_Glass2_Length;
    s_shadow.g2_h = EEprom_Glass2_Height;
    s_shadow.g2_d = EEprom_Glass2_Depth;
    s_shadow.g1_l = EEprom_Glass1_Length;
    s_shadow.g1_h = EEprom_Glass1_Height;
    s_shadow.g1_d = EEprom_Glass1_Depth;
    s_shadow.total_filled_g1 = EEprom_TotalFilled_Glass1;
    s_shadow.total_filled_g2 = EEprom_TotalFilled_Glass2;
    s_shadow.total_sec_g1 = EEprom_TotalSeconds_Glass1;
    s_shadow.total_sec_g2 = EEprom_TotalSeconds_Glass2;
    s_shadow_valid = true;
}

static bool is_dirty(void) {
    if (!s_shadow_valid) return true;
    if (s_shadow.language != EEprom2_Language) return true;
    if (s_shadow.buzzer != EEprom3_Buzzer) return true;
    if (s_shadow.touch != EEprom4_TouchAvailable) return true;
    if (strcmp(s_shadow.pin, EEprom6_AdminPin) != 0) return true;
    if (s_shadow.output_type != EEprom7_OutputType) return true;
    if (fabsf(s_shadow.qs - EEprom_Process_Qs) > 0.001f) return true;
    if (fabsf(s_shadow.p1 - EEprom_Process_P1) > 0.001f) return true;
    if (fabsf(s_shadow.p2 - EEprom_Process_P2) > 0.001f) return true;
    if (fabsf(s_shadow.g2_l - EEprom_Glass2_Length) > 0.001f) return true;
    if (fabsf(s_shadow.g2_h - EEprom_Glass2_Height) > 0.001f) return true;
    if (fabsf(s_shadow.g2_d - EEprom_Glass2_Depth) > 0.001f) return true;
    if (fabsf(s_shadow.g1_l - EEprom_Glass1_Length) > 0.001f) return true;
    if (fabsf(s_shadow.g1_h - EEprom_Glass1_Height) > 0.001f) return true;
    if (fabsf(s_shadow.g1_d - EEprom_Glass1_Depth) > 0.001f) return true;
    if (s_shadow.total_filled_g1 != EEprom_TotalFilled_Glass1) return true;
    if (s_shadow.total_filled_g2 != EEprom_TotalFilled_Glass2) return true;
    if (s_shadow.total_sec_g1 != EEprom_TotalSeconds_Glass1) return true;
    if (s_shadow.total_sec_g2 != EEprom_TotalSeconds_Glass2) return true;
    return false;
}

static float sanitize_float(float val, float min_val, float max_val, float def_val)
{
    if (isnan(val) || !isfinite(val) || val < min_val || val > max_val) {
        return def_val;
    }
    return val;
}

void a2_eeprom_init(void)
{
    if (!EEPROM.begin(1000)) {
        Serial.println("Failed to initialize EEPROM");
        Serial.println("Restarting...");
        delay(1000);
    }
}

void a2_eeprom_read_all(void)
{
    unsigned char eversaved_defalte = 0;
    Address = EEpromStartAddres;

    eversaved_defalte = EEPROM.readUChar(Address);
    if(eversaved_defalte == SavedId)
    {
        Address += sizeof(unsigned char);
        EEprom2_Language = EEPROM.readUChar(Address);
        Address += sizeof(unsigned char);
        
        EEprom3_Buzzer = EEPROM.readBool(Address);
        Address += sizeof(bool);
        
        EEprom4_TouchAvailable = EEPROM.readBool(Address);
        Address += sizeof(bool);

        bool pin_ok = true;
        for (int i = 0; i < 4; i++) {
            unsigned char c = EEPROM.readUChar(Address++);
            if (c < '0' || c > '9') pin_ok = false;
            EEprom6_AdminPin[i] = (char)c;
        }
        EEprom6_AdminPin[4] = '\0';
        if (!pin_ok) {
            strcpy(EEprom6_AdminPin, "1234");
        }
        EEprom7_OutputType = EEPROM.readUChar(Address++);
        if (EEprom7_OutputType > 1) EEprom7_OutputType = 0;

        /* Process parameters */
        EEprom_Process_Qs = sanitize_float(EEPROM.readFloat(Address), 0.01f, 500.0f, 30.0f);
        Address += sizeof(float);
        EEprom_Process_P1 = sanitize_float(EEPROM.readFloat(Address), 0.01f, 100.0f, 1.0f);
        Address += sizeof(float);
        EEprom_Process_P2 = sanitize_float(EEPROM.readFloat(Address), 0.01f, 100.0f, 1.0f);
        Address += sizeof(float);

        /* Glass 2 dimensions (Left channel) */
        EEprom_Glass2_Length = sanitize_float(EEPROM.readFloat(Address), 0.0f, 1000.0f, 0.0f);
        Address += sizeof(float);
        EEprom_Glass2_Height = sanitize_float(EEPROM.readFloat(Address), 0.0f, 1000.0f, 0.0f);
        Address += sizeof(float);
        EEprom_Glass2_Depth = sanitize_float(EEPROM.readFloat(Address), 0.0f, 100.0f, 0.0f);
        Address += sizeof(float);

        /* Glass 1 dimensions (Right channel) */
        EEprom_Glass1_Length = sanitize_float(EEPROM.readFloat(Address), 0.0f, 1000.0f, 0.0f);
        Address += sizeof(float);
        EEprom_Glass1_Height = sanitize_float(EEPROM.readFloat(Address), 0.0f, 1000.0f, 0.0f);
        Address += sizeof(float);
        EEprom_Glass1_Depth = sanitize_float(EEPROM.readFloat(Address), 0.0f, 100.0f, 0.0f);
        Address += sizeof(float);

        /* Production statistics */
        EEprom_TotalFilled_Glass1 = EEPROM.readULong(Address);
        Address += sizeof(unsigned long);
        if (EEprom_TotalFilled_Glass1 == 0xFFFFFFFF) EEprom_TotalFilled_Glass1 = 0;

        EEprom_TotalFilled_Glass2 = EEPROM.readULong(Address);
        Address += sizeof(unsigned long);
        if (EEprom_TotalFilled_Glass2 == 0xFFFFFFFF) EEprom_TotalFilled_Glass2 = 0;

        EEprom_TotalSeconds_Glass1 = EEPROM.readULong(Address);
        Address += sizeof(unsigned long);
        if (EEprom_TotalSeconds_Glass1 == 0xFFFFFFFF) EEprom_TotalSeconds_Glass1 = 0;

        EEprom_TotalSeconds_Glass2 = EEPROM.readULong(Address);
        Address += sizeof(unsigned long);
        if (EEprom_TotalSeconds_Glass2 == 0xFFFFFFFF) EEprom_TotalSeconds_Glass2 = 0;

        if (!pin_ok) a2_eeprom_write_all();
    }
    else
    {
        EEprom1_EverSaved = SavedId;
        EEPROM.writeUChar(Address, EEprom1_EverSaved); 
        Address += sizeof(unsigned char);
        
        EEprom2_Language = 0;
        EEPROM.writeUChar(Address, EEprom2_Language); 
        Address += sizeof(unsigned char);

        EEprom3_Buzzer = true;
        EEPROM.writeBool(Address, EEprom3_Buzzer); 
        Address += sizeof(bool);

        EEprom4_TouchAvailable = true;
        EEPROM.writeBool(Address, EEprom4_TouchAvailable); 
        Address += sizeof(bool);

        strcpy(EEprom6_AdminPin, "1234");
        for (int i = 0; i < 4; i++) EEPROM.writeUChar(Address++, (unsigned char)EEprom6_AdminPin[i]);
        EEprom7_OutputType = 0;
        EEPROM.writeUChar(Address++, EEprom7_OutputType);

        EEprom_Process_Qs = 30.0f;
        EEPROM.writeFloat(Address, EEprom_Process_Qs);
        Address += sizeof(float);
        EEprom_Process_P1 = 1.0f;
        EEPROM.writeFloat(Address, EEprom_Process_P1);
        Address += sizeof(float);
        EEprom_Process_P2 = 1.0f;
        EEPROM.writeFloat(Address, EEprom_Process_P2);
        Address += sizeof(float);

        EEprom_Glass2_Length = 0.0f;
        EEPROM.writeFloat(Address, EEprom_Glass2_Length);
        Address += sizeof(float);
        EEprom_Glass2_Height = 0.0f;
        EEPROM.writeFloat(Address, EEprom_Glass2_Height);
        Address += sizeof(float);
        EEprom_Glass2_Depth = 0.0f;
        EEPROM.writeFloat(Address, EEprom_Glass2_Depth);
        Address += sizeof(float);

        EEprom_Glass1_Length = 0.0f;
        EEPROM.writeFloat(Address, EEprom_Glass1_Length);
        Address += sizeof(float);
        EEprom_Glass1_Height = 0.0f;
        EEPROM.writeFloat(Address, EEprom_Glass1_Height);
        Address += sizeof(float);
        EEprom_Glass1_Depth = 0.0f;
        EEPROM.writeFloat(Address, EEprom_Glass1_Depth);
        Address += sizeof(float);

        EEprom_TotalFilled_Glass1 = 0;
        EEPROM.writeULong(Address, EEprom_TotalFilled_Glass1);
        Address += sizeof(unsigned long);
        EEprom_TotalFilled_Glass2 = 0;
        EEPROM.writeULong(Address, EEprom_TotalFilled_Glass2);
        Address += sizeof(unsigned long);
        EEprom_TotalSeconds_Glass1 = 0;
        EEPROM.writeULong(Address, EEprom_TotalSeconds_Glass1);
        Address += sizeof(unsigned long);
        EEprom_TotalSeconds_Glass2 = 0;
        EEPROM.writeULong(Address, EEprom_TotalSeconds_Glass2);
        Address += sizeof(unsigned long);
        
        EEPROM.commit();
    }
    update_shadow();
}

void a2_eeprom_write_all(void)
{
    if (!is_dirty()) {
        return; /* Zero flash wear, zero UI latency */
    }

    Address = EEpromStartAddres;
    EEprom1_EverSaved = SavedId;
    EEPROM.writeUChar(Address, EEprom1_EverSaved); 
    Address += sizeof(unsigned char);

    EEPROM.writeUChar(Address, EEprom2_Language); 
    Address += sizeof(unsigned char);

    EEPROM.writeBool(Address, EEprom3_Buzzer); 
    Address += sizeof(bool);

    EEPROM.writeBool(Address, EEprom4_TouchAvailable); 
    Address += sizeof(bool);

    for (int i = 0; i < 4; i++) EEPROM.writeUChar(Address++, (unsigned char)EEprom6_AdminPin[i]);
    EEPROM.writeUChar(Address++, EEprom7_OutputType);

    EEPROM.writeFloat(Address, EEprom_Process_Qs);
    Address += sizeof(float);
    EEPROM.writeFloat(Address, EEprom_Process_P1);
    Address += sizeof(float);
    EEPROM.writeFloat(Address, EEprom_Process_P2);
    Address += sizeof(float);

    EEPROM.writeFloat(Address, EEprom_Glass2_Length);
    Address += sizeof(float);
    EEPROM.writeFloat(Address, EEprom_Glass2_Height);
    Address += sizeof(float);
    EEPROM.writeFloat(Address, EEprom_Glass2_Depth);
    Address += sizeof(float);

    EEPROM.writeFloat(Address, EEprom_Glass1_Length);
    Address += sizeof(float);
    EEPROM.writeFloat(Address, EEprom_Glass1_Height);
    Address += sizeof(float);
    EEPROM.writeFloat(Address, EEprom_Glass1_Depth);
    Address += sizeof(float);

    EEPROM.writeULong(Address, EEprom_TotalFilled_Glass1);
    Address += sizeof(unsigned long);
    EEPROM.writeULong(Address, EEprom_TotalFilled_Glass2);
    Address += sizeof(unsigned long);
    EEPROM.writeULong(Address, EEprom_TotalSeconds_Glass1);
    Address += sizeof(unsigned long);
    EEPROM.writeULong(Address, EEprom_TotalSeconds_Glass2);
    Address += sizeof(unsigned long);

    EEPROM.commit();
    update_shadow();
}

void a2_eeprom_reset_production_counters(void)
{
    EEprom_TotalFilled_Glass1 = 0;
    EEprom_TotalFilled_Glass2 = 0;
    EEprom_TotalSeconds_Glass1 = 0;
    EEprom_TotalSeconds_Glass2 = 0;
    a2_eeprom_write_all();
}

void a2_eeprom_factory_reset(void)
{
    EEPROM.writeUChar(EEpromStartAddres, 0);
    EEPROM.commit();
    delay(400);
    ESP.restart();
}

