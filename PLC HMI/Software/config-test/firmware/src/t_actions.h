#ifndef T_EEZ_LVGL_UI_EVENTS_H
#define T_EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_switchtomain(lv_event_t * e);
extern void action_switchtosetup(lv_event_t * e);
extern void action_counterplusing(lv_event_t * e);
extern void action_gotors485(lv_event_t * e);
extern void action_gotogsm(lv_event_t * e);
extern void action_gotobluetooth(lv_event_t * e);
extern void action_gotowifi(lv_event_t * e);
extern void action_senddata(lv_event_t * e);
extern void action_datareceive(lv_event_t * e);
extern void action_anablblutooth(lv_event_t * e);
extern void action_scanwifi(lv_event_t * e);
extern void action_test_wifi_connect(lv_event_t * e);
extern void action_gotodisw(lv_event_t * e);
extern void action_gotodisb(lv_event_t * e);
extern void action_gotodisr(lv_event_t * e);
extern void action_gotodisg(lv_event_t * e);
extern void action_gotodisblue(lv_event_t * e);
extern void action_gotosysinfo(lv_event_t * e);
extern void action_drawline(lv_event_t * e);
extern void action_gototouchtest(lv_event_t * e);
extern void action_settouch(lv_event_t * e);
extern void action_backfromtouch(lv_event_t * e);
extern void action_callrtc(lv_event_t * e);
extern void action_gotortcsetup(lv_event_t * e);
extern void action_gotossr(lv_event_t * e);
extern void action_pwm_start(lv_event_t * e);
extern void action_pwm_stop(lv_event_t * e);
extern void action_ifsetuploaded(lv_event_t * e);
extern void action_ifssrloaded(lv_event_t * e);
extern void action_clear_touch(lv_event_t * e);
extern void action_ble_scan(lv_event_t * e);
extern void action_ble_connect(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*T_EEZ_LVGL_UI_EVENTS_H*/
