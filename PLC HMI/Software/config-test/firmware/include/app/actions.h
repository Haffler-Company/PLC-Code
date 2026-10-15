#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_btn_wifi_connect_event(lv_event_t * e);
extern void action_btn_wifi_search_event(lv_event_t * e);
extern void action_btn_connect(lv_event_t * e);
extern void action_admin_password_redy(lv_event_t * e);
extern void action_card_install(lv_event_t * e);
extern void action_install_card_by_index(int idx);
extern void action_confirm_yes(lv_event_t * e);
extern void action_confirm_no(lv_event_t * e);
extern void action_btn_back_to_password(lv_event_t * e);
extern void action_link_entry_open(lv_event_t * e);
extern void action_link_cancel(lv_event_t * e);
extern void action_link_confirm(lv_event_t * e);
extern void action_link_check(lv_event_t * e);
extern const char* link_check_get_result(void);
extern int link_check_get_status(void);
extern bool link_check_is_done(void);
extern void link_check_clear_done(void);
extern void action_version_open(lv_event_t * e);
extern void action_version_close(lv_event_t * e);
extern void action_download_cancel(lv_event_t * e);


enum LinkCheckStatus {
    LINK_CHECK_IDLE = 0,
    LINK_CHECK_BUSY = 1,
    LINK_CHECK_OK_INFO = 2,
    LINK_CHECK_OK_BIN = 3,
    LINK_CHECK_ERR_NOTFOUND = 4,
    LINK_CHECK_ERR_WIFI = 5
};

extern void link_entry_show_message(const char *msg, uint32_t bg_hex, uint32_t text_hex, uint32_t border_hex, uint32_t duration_ms);
extern void link_entry_hide_message(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/