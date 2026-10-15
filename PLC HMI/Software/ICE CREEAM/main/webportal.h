#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

void webportal_init(void);
void webportal_stop(void);
bool webportal_is_running(void);

#ifdef __cplusplus
}
#endif
