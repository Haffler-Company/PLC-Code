#ifndef PANEL_KEYS_H
#define PANEL_KEYS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KEY_NONE = 0,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_MODE,
    KEY_START,
    KEY_STOP,
    KEY_SAVE,
    KEY_CANCEL,
    KEY_OPTO_START,
    KEY_OPTO_STOP
} panel_key_id_t;

typedef enum {
    STATION_MODE_AUTO = 0,    // Dual Sync / Ready Station
    STATION_MODE_GLASS1 = 1,  // Station 1 (Glass 1, Right)
    STATION_MODE_GLASS2 = 2   // Station 2 (Glass 2, Left)
} station_mode_t;

void panel_keys_init(void);
void panel_keys_poll(void);
void panel_keys_emergency_stop_all(void);
station_mode_t panel_keys_get_mode(void);
void panel_keys_set_mode(station_mode_t mode);
bool is_f5_boot_active(void);
bool panel_keys_is_start_pressed(void);
void panel_keys_trigger_key(panel_key_id_t key);

#ifdef __cplusplus
}
#endif

#endif // PANEL_KEYS_H
