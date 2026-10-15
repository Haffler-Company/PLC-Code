#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_LANG_FA = 0, /* فارسی */
    UI_LANG_EN = 1, /* English */
    UI_LANG_TR = 2  /* Türkçe */
} ui_lang_t;

typedef enum {
    /* Main Screen */
    STR_MAIN_TITLE = 0,
    STR_MODE_WASH,
    STR_MODE_PROD,
    STR_MODE_HOLD,
    STR_MODE_STOP,
    STR_EDGE_INJECT,
    STR_RAINBOW,
    STR_TODAY,
    STR_TOTAL,

    /* Settings Tabs Headers */
    STR_TAB_F1_HDR,
    STR_TAB_F2_HDR,
    STR_TAB_F3_HDR,
    STR_TAB_F4_HDR,
    STR_TAB_F5_HDR,
    STR_TAB_F6_HDR,
    STR_TAB_F7_HDR,

    /* F1 Timers */
    STR_TMR_PADDLE,
    STR_TMR_COMP_RST,
    STR_TMR_EYE,
    STR_T1_FALLBACK_ON,
    STR_T1_FALLBACK_OFF,
    STR_COMP_FALLBACK_ON,
    STR_COMP_FALLBACK_OFF,
    STR_COUNTER_TITLE,
    STR_COUNTER_RESET,
    STR_STANDBY_PREFIX,
    STR_STANDBY_OFF,
    STR_MINUTES,
    STR_SECONDS,
    STR_HOURS,

    /* F2 Clock & NTP */
    STR_YEAR,
    STR_MONTH,
    STR_DAY,
    STR_HOUR,
    STR_MINUTE,
    STR_NTP_SYNC_BTN,
    STR_NTP_SYNCING,
    STR_NTP_SUCCESS,
    STR_NTP_NO_WIFI,

    /* F3 Inverter / Hold */
    STR_MODBUS_DELAY,
    STR_COMP_CYCLE,
    STR_HOLD_DELAY,
    STR_CURRENT_SET1,
    STR_CURRENT_SET2,
    STR_TEMP_T1_ON,
    STR_TEMP_T1_OFF,

    /* F4 WiFi */
    STR_WIFI_SCAN,
    STR_WIFI_CONNECT,
    STR_OTA_UPDATE_BTN,

    /* F5 Schedules */
    STR_F5_HOLD_TIME,
    STR_F5_PROD_TIME,
    STR_F5_PADDLE_TIME,
    STR_F5_MAIN_COMP,
    STR_F5_HOLD_COMP,

    /* F6 Diagnostics */
    STR_F6_INPUTS_HDR,
    STR_F6_OUTPUTS_HDR,
    STR_F6_IN_FRONT_SW,
    STR_F6_IN_REAR_SW,
    STR_F6_IN_EYE,
    STR_F6_IN_STOP,
    STR_F6_IN_START,
    STR_F6_IN_MTR_OVL,
    STR_F6_IN_COMP_OVL,
    STR_F6_OUT_COMP,
    STR_F6_OUT_PROD,
    STR_F6_OUT_MIXER,
    STR_F6_OUT_EDGE,
    STR_F6_OUT_RAINBOW,
    STR_F6_OUT_AIR,
    STR_F6_OUT_RESET,
    STR_F6_OUT_WASH,
    STR_F6_TEMP_T1,
    STR_F6_TEMP_T2,

    /* F7 Dispense & Safety */
    STR_TMR_DSP_RUN,
    STR_TMR_FRT_HOLD,
    STR_CNT_SCP_COMP,
    STR_TMR_FRZ_TO,
    STR_TMR_LEV_MAX,
    STR_LOCKED_TIMERS_HDR,
    STR_LOCKED_BADGE,
    STR_FIXED_COOLDOWN,
    STR_FIXED_RESET_PULSE,
    STR_FIXED_DEBOUNCE,
    STR_FIXED_CYL_LIMIT,
    STR_FIXED_PULLEY_TIMEOUT,

    /* Bottom Toolbar */
    STR_BTN_HOME,
    STR_BTN_SAVE,
    STR_BTN_HELP,
    STR_BTN_FAULTS,

    /* Help Dialogs */
    STR_HELP_GOT_IT,
    STR_HELP_F1_TITLE,
    STR_HELP_F1_BODY,
    STR_HELP_F2_TITLE,
    STR_HELP_F2_BODY,
    STR_HELP_F3_TITLE,
    STR_HELP_F3_BODY,
    STR_HELP_F4_TITLE,
    STR_HELP_F4_BODY,
    STR_HELP_F5_TITLE,
    STR_HELP_F5_BODY,
    STR_HELP_F6_TITLE,
    STR_HELP_F6_BODY,
    STR_HELP_F7_TITLE,
    STR_HELP_F7_BODY,

    /* Dashboard Extras & Notifications */
    STR_HOPPER_LABEL,
    STR_CYL_LABEL,
    STR_SENSOR_NC,
    STR_HOPPER_SENSOR_EN,
    STR_HOPPER_SENSOR_DIS,
    STR_MODE_PADDLE_BEATER,
    STR_READY_STANDBY,
    STR_READY_DISPENSE,
    STR_FREEZING_PCT,
    STR_COMP_COOLDOWN_PREFIX,
    STR_WASH_DRAIN_PREFIX,
    STR_WASH_AUTO_PREFIX,

    /* Status Badges */
    STR_STATUS_ON,
    STR_STATUS_OFF,
    STR_STATUS_READY,

    /* F2 Day-Lock & Leasing */
    STR_DL_TITLE,
    STR_DL_STATUS_LOCKED,
    STR_DL_STATUS_DAYS,
    STR_DL_STATUS_UNLIMITED,
    STR_DL_CHALLENGE,
    STR_DL_EXTEND_BTN,
    STR_DL_5TIMERS_BTN,
    STR_DL_SUPPORT,
    STR_DL_EMERGENCY,
    STR_DL_TAMPER_ALERT,
    STR_DL_LOCKED_TITLE,
    STR_DL_HELP_TITLE,
    STR_DL_HELP_MSG,
    STR_DL_HELP_SITE,
    STR_DL_SYNCING_TIME,
    STR_DL_NO_WIFI,
    STR_DL_GRACE_USED,
    STR_DL_INVALID_PIN,

    /* F4 WiFi Extras */
    STR_WIFI_NO_NETS,
    STR_WIFI_SCANNING,
    STR_WIFI_PASS_TITLE,
    STR_WIFI_PASS_PLACEHOLDER,

    /* Fault History Modal */
    STR_FAULT_HIST_TITLE,
    STR_FAULT_HIST_EMPTY,
    STR_FAULT_HIST_CLEAR,
    STR_FAULT_HIST_CLOSE,

    /* Screensaver */
    STR_SS_STANDBY,
    STR_SS_READY_DISPENSE,
    STR_SS_DISPENSING,
    STR_SS_PROD_CYCLE,
    STR_SS_PADDLE_ACTIVE,
    STR_SS_WAKE_HINT,

    /* Manager PIN Dialog */
    STR_PIN_TITLE,
    STR_PIN_PROMPT,
    STR_PIN_CLEAR,
    STR_PIN_ENTER,

    /* Counter Details Dialog */
    STR_CNT_STATS_TITLE,
    STR_CNT_SHIFT_CONE,
    STR_CNT_SHIFT_REV,
    STR_CNT_SHIFT_RST,
    STR_CNT_TOTAL_CONE,
    STR_CNT_TOTAL_REV,
    STR_CNT_ODOMETER,
    STR_CNT_PRICE_NOTE,
    STR_CNT_DRAIN_START,
    STR_CNT_DRAIN_STOP,
    STR_CLOSE_DIALOG,

    /* Interactive Fault Dialog */
    STR_FAULT_CAUSE_HDR,
    STR_FAULT_SOL_HDR,
    STR_FAULT_RETRY,
    STR_FAULT_DISMISS,

    /* Weekday Names */
    STR_WDAY_SUN,
    STR_WDAY_MON,
    STR_WDAY_TUE,
    STR_WDAY_WED,
    STR_WDAY_THU,
    STR_WDAY_FRI,
    STR_WDAY_SAT,

    /* Unsaved Changes Confirmation Dialog */
    STR_UNSAVED_TITLE,
    STR_UNSAVED_MSG,
    STR_UNSAVED_SAVE_EXIT,
    STR_UNSAVED_DISCARD_EXIT,
    STR_UNSAVED_CANCEL,

    /* Live Process Monitoring Card */
    STR_MON_IDLE,
    STR_MON_NEXT_SELECT,
    STR_MON_PROD_ACTIVE,
    STR_MON_PROD_NEXT,
    STR_MON_HOLD_ACTIVE,
    STR_MON_HOLD_NEXT,
    STR_MON_WASH_ACTIVE,
    STR_MON_WASH_NEXT,
    STR_MON_DRAIN_ACTIVE,
    STR_MON_DRAIN_NEXT,
    STR_MON_DISPENSE_ACTIVE,
    STR_MON_DISPENSE_NEXT,
    STR_MON_COOLDOWN_ACTIVE,
    STR_MON_COOLDOWN_NEXT,

    /* Splash & Hardware POST Alert */
    STR_SPLASH_INIT,
    STR_BOOT_ALERT_TITLE,
    STR_BOOT_ALERT_SUB,
    STR_BOOT_ALERT_VIEW,
    STR_BOOT_ALERT_DISMISS,
    STR_BOOT_ALERT_AUTO,

    /* Counter Reset Confirmation Dialog */
    STR_RST_CNT_TITLE,
    STR_RST_CNT_MSG,
    STR_RST_CNT_CONFIRM,

    /* Boot WiFi Prompt Dialog */
    STR_BOOT_WIFI_PROMPT_TITLE,
    STR_BOOT_WIFI_PROMPT_MSG,
    STR_BOOT_WIFI_BTN_LATER,
    STR_BOOT_WIFI_BTN_GOTO,

    /* OTA & Firmware Update */
    STR_OTA_UP_TO_DATE,
    STR_OTA_CARD_UPDATE_BTN,
    STR_OTA_APP_IS_UPDATED,
    STR_BOOT_OTA_PROMPT_TITLE,
    STR_BOOT_OTA_PROMPT_MSG,
    STR_BOOT_OTA_BTN_INSTALL,
    STR_BOOT_OTA_BTN_LATER,
    STR_SETTINGS_SAVED,
    STR_WIFI_CONNECT_FIRST,

    /* New Features: Anti-cycle, Pulley toggle, Overload polarity, Temp Calibration */
    STR_FIXED_ANTI_CYCLE,
    STR_PULLEY_SENSOR,
    STR_PULLEY_ENABLED,
    STR_PULLEY_DISABLED,
    STR_CALIBRATE_TEMP,
    STR_RAW_TEMP,
    STR_CAL_OFFSET,
    STR_FINAL_TEMP,
    STR_OVERLOAD_POLARITY,
    STR_POLARITY_NO,
    STR_POLARITY_NC,
    STR_DISABLE_PULLEY_BTN,
    STR_CAL_RESET_ZERO,
    STR_CAL_SAVE,

    /* Factory Reset Confirmation Dialog */
    STR_FACTORY_RST_TITLE,
    STR_FACTORY_RST_DESC,
    STR_FACTORY_RST_CONFIRM,
    STR_FACTORY_RST_CANCEL,
    STR_FACTORY_RST_AUTO_CLOSE,
    STR_FACTORY_RST_AUTO_PAUSED,
    STR_FACTORY_RST_DONE,
    STR_FACTORY_RST_BTN,

    /* Warranty Activation Subsystem */
    STR_WARR_ACTIVATE_BTN,
    STR_WARR_ACTIVE_TITLE,
    STR_WARR_TITLE,
    STR_WARR_BUYER_NAME,
    STR_WARR_CITY,
    STR_WARR_PHONE,
    STR_WARR_SUBMIT,
    STR_WARR_DATE_LBL,
    STR_WARR_CODE_LBL,
    STR_WARR_EXPIRED_TITLE,
    STR_WARR_EXPIRED_MSG,
    STR_WARR_DISMISS_BTN,
    STR_WARR_NO_NET_TITLE,
    STR_WARR_NO_NET_MSG,
    STR_WARR_GOTO_WIFI,
    STR_WARR_ADMIN_OVERRIDE,
    STR_WARR_INVALID_INPUT,
    STR_WARR_ACTIVATED_ONLINE,
    STR_WARR_ACTIVATED_OFFLINE,
    STR_WARR_PH_NAME,
    STR_WARR_PH_CITY,
    STR_WARR_PH_PHONE,

    STR_COUNT
} ui_str_key_t;

/* Core API */
void ui_lang_init(void);
ui_lang_t ui_lang_get(void);
void ui_lang_set(ui_lang_t lang);
const char *ui_lang_str(ui_str_key_t key);
const lv_font_t *ui_lang_font_14(void);
const lv_font_t *ui_lang_font_16(void);
const lv_font_t *ui_lang_font_20(void);
const lv_font_t *ui_lang_font_24(void);
lv_base_dir_t ui_lang_base_dir(void);

/* Registration for live refresh */
typedef void (*ui_lang_refresh_cb_t)(void);
void ui_lang_register_refresh_cb(ui_lang_refresh_cb_t cb);
void ui_lang_trigger_refresh(void);

#ifdef __cplusplus
}
#endif
