#ifndef A2_EEPROM_H
#define A2_EEPROM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char EEprom1_EverSaved;
extern unsigned char EEprom2_Language;
extern bool EEprom3_Buzzer;
extern bool EEprom4_TouchAvailable;
extern char EEprom6_AdminPin[5];
extern unsigned char EEprom7_OutputType;
extern bool EEprom5_MustSave;

/* Process parameters (Setting -> Process) */
extern float EEprom_Process_Qs;
extern float EEprom_Process_P1;
extern float EEprom_Process_P2;

/* Last-used glass dimensions (Main screen) */
extern float EEprom_Glass2_Length;
extern float EEprom_Glass2_Height;
extern float EEprom_Glass2_Depth;
extern float EEprom_Glass1_Length;
extern float EEprom_Glass1_Height;
extern float EEprom_Glass1_Depth;

/* Production statistics */
extern unsigned long EEprom_TotalFilled_Glass1;
extern unsigned long EEprom_TotalFilled_Glass2;
extern unsigned long EEprom_TotalSeconds_Glass1;
extern unsigned long EEprom_TotalSeconds_Glass2;

void a2_eeprom_init(void);
void a2_eeprom_read_all(void);
void a2_eeprom_write_all(void);
void a2_eeprom_factory_reset(void);
void a2_eeprom_reset_production_counters(void);

#ifdef __cplusplus
}
#endif

#endif /* A2_EEPROM_H */
