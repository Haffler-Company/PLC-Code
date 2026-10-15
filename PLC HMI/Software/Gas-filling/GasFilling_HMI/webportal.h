#ifndef GASFILLING_WEBPORTAL_H
#define GASFILLING_WEBPORTAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void webportal_init(void);
void webportal_poll(void);
bool webportal_is_running(void);
void webportal_set_enabled(bool enable);
bool webportal_is_enabled(void);
const char* webportal_get_ip(void);
const char* webportal_get_ssid(void);
bool webportal_is_sta_connected(void);
void stats_increment(uint32_t g1_delta, uint32_t g2_delta);
void stats_roll_day(void);

#ifdef __cplusplus
}
#endif

#endif /* GASFILLING_WEBPORTAL_H */
