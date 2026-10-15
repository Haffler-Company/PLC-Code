#pragma once
#ifndef APP_SOUND_H
#define APP_SOUND_H

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_sound_init(void);
void app_sound_tick(void);
void app_sound_stop(void);

#ifdef __cplusplus
}
#endif

#endif // APP_SOUND_H
