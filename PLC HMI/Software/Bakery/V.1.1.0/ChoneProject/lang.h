#ifndef LANG_H
#define LANG_H

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LANG_FA = 0,    // فارسی (پیش فرض)
    LANG_EN = 1,    // English
    LANG_TR = 2,    // Türkçe
    LANG_RU = 3,    // Русский
    LANG_COUNT = 4
} Lang;

typedef enum {
    STR_APP_TITLE = 0,
    STR_AUTO_MODE,
    STR_MANUAL_MODE,
    STR_SETTINGS,
    STR_WEIGHT,
    STR_SPEED,
    STR_COUNT,
    STR_RESET_COUNT,
    STR_START,
    STR_STOP,
    STR_BACK,
    STR_SAVE,
    STR_MAX_PISTON,
    STR_MAX_FUNNEL,
    STR_PISTON_UP,
    STR_PISTON_DOWN,
    STR_FUNNEL_FWD,
    STR_FUNNEL_REV,
    STR_DIAGNOSTICS,
    STR_SYS_READY,
    STR_RUNNING,
    STR_SAVED_OK,
    STR_PIN_ERROR,
    STR_WARNING,
    STR_ERROR,
    STR_CONFIRM_CONTINUE,
    STR_WARN_RULE_1,
    STR_WARN_RULE_2,
    STR_WARN_RULE_3,
    STR_WARN_RULE_4,
    STR_WIFI_TITLE,
    STR_WIFI_ON,
    STR_WIFI_OFF,
    STR_LANG_SELECT,
    STR_IO_TEST,
    STR_SET_VALUE,
    STR_TIME_MS,
    STR_CLOSE,
    STR_LANG_CHANGED,
    STR_PROJECT_INFO,
    STR_OTA_UPDATE,
    STR_WIFI_NETWORKS,
    STR_WIFI_SCAN,
    STR_WIFI_SCANNING,
    STR_WIFI_CONNECT,
    STR_WIFI_CONNECTING,
    STR_WIFI_CONNECTED,
    STR_WIFI_DISCONNECTED,
    STR_WIFI_WRONG_PASS,
    STR_WIFI_PASS_PROMPT,
    STR_FETCH_FROM_SERVER,
    STR_FETCHING_SERVER,
    STR_SERVER_UNAVAILABLE,
    STR_WIFI_NOT_CONNECTED_WARN,
    STR_INSTALL_UPDATE,
    STR_INSTALL_CONFIRM,
    STR_INSTALLING,
    STR_PINMAP_TITLE,
    STR_INPUTS_TITLE,
    STR_OUTPUTS_TITLE,
    STR_CHANGELOG_TITLE,
    STR_UP_TO_DATE,
    STR_HELP_MANUAL,
    STR_LINK_ENTRY,
    STR_DEFAULT_LINK,
    STR_DOWNLOAD_AND_INSTALL,
    STR_SPECS_TITLE,
    STR_CONFIG_TEST_TITLE,
    STR_CHONE_GIR_TITLE,
    STR_ID_COUNT
} StrId;

extern Lang g_lang;

const char *lang_native_name(Lang l);
const char *tr(StrId id);
void lang_cycle(void);
void lang_set(Lang l);

void i18n_reset(void);
void i18n_register(lv_obj_t *label, StrId id);
void i18n_apply(void);

#ifdef __cplusplus
}
#endif

#endif /* LANG_H */
