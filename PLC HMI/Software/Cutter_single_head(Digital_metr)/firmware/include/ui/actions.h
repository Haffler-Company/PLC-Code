#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_stopstate(lv_event_t * e);
extern void action_printertest(lv_event_t * e);
extern void action_lang_farsi(lv_event_t * e);
extern void action_lang_english(lv_event_t * e);
extern void action_toggle_dropdown(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/