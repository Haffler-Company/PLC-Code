#ifndef LANG_H
#define LANG_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// ترتیب زبان‌ها: انگلیسی، ترکی، فارسی (به ترتیب الفبای فارسی)
typedef enum {
    LANG_EN = 0,
    LANG_TR = 1,
    LANG_FA = 2,
    LANG_DE = 3,
    LANG_RU = 4,
    LANG_COUNT = 5
} Lang;

// شناسه‌ی رشته‌های قابل ترجمه
typedef enum {
    STR_ADMIN_ENTER = 0,
    STR_SELECT_WIFI,
    STR_CONNECT,
    STR_DEVICE_MODEL,
    STR_SERIAL,
    STR_WIRING_GUIDE,
    STR_WIRE_I6,
    STR_WIRE_I7,
    STR_WIRE_Q1,
    STR_WIRE_Q0,
    STR_WIRE_Q2,
    STR_WIRE_Q3,
    STR_PROD_CHANEGIR,
    STR_INSTALL,
    STR_PROD_SINGLE,
    STR_PROD_TEST,
    STR_PROD_SW1,
    STR_SCANNING,
    STR_WIFI_FAIL,
    STR_WIFI_WRONGSAVED,
    STR_ADMIN_WRONG,
    STR_DL_PREFIX,
    STR_UP_TO_DATE,
    STR_NOT_ON_SERVER,
    STR_NOT_AVAILABLE,
    STR_INCOMPATIBLE,
    STR_PROD_CUTTER_SINGLE,
    STR_PROD_CUTTER_DOUBLE,
    STR_PROD_WELDER_DOUBLE,
    STR_PROD_GAS,
    STR_PROD_CORNER,
    STR_PROD_COPY,
    STR_PROD_PUNCH,
    STR_PROD_CONFIG,
    STR_CONFIRM_Q,
    STR_YES,
    STR_NO,
    STR_BACK_TO_ADMIN,
    STR_CANCEL_DL,
    STR_DL_CANCELLED,
    STR_DL_CANCELLING,
    STR_DL_STALLED,
    STR_DL_FINALIZING,
    STR_LINK_ENTRY,
    STR_INFO,
    STR_BACK,
    STR_INSTALL_THIS,
    STR_INPUTS_PINS,
    STR_OUTPUTS_PINS,
    STR_FW_DETAILS,
    STR_FEATURES_SPECS,
    STR_STATUS_READY,
    STR_STATUS_OFFLINE,
    STR_HW_BASE,
    STR_HW_REQ1,
    STR_HW_REQ2,
    STR_HW_REQ3,
    STR_PROD_ICE_CREAM,
    STR_PROD_BAKERY_CHANGIR,
    STR_PROD_BAKERY_MIXER,
    STR_SUITE_UPVC,
    STR_SUITE_OTHER,
    STR_CONNECTING_TO_SERVER,
    STR_CHECKING,
    STR_UNAVAILABLE_BTN,
    STR_NEW_VERSION,
    STR_PROGRAM_UP_TO_DATE,
    STR_REINSTALL,
    STR_NOT_DEFINED_ON_SERVER,
    STR_NOT_RECEIVED_FROM_SERVER,
    STR_DETAILS_NOT_RECEIVED,
    STR_DEFAULT_LINK,
    STR_DOWNLOAD_AND_INSTALL,
    STR_ENTER_LINK_OR_FILE,
    STR_LINK_CHECKING,
    STR_ENTER_FILENAME_ERR,
    STR_WIFI_DISCONNECTED_ERR,
    STR_LINK_VALID_BIN,
    STR_LINK_NOT_FOUND,
    STR_LINK_VALID_INFO,
    STR_VERSION_INFO,
    STR_ID_COUNT
} StrId;

// زبان جاری (پیش‌فرض فارسی)
extern Lang g_lang;

// نام بومی هر زبان (همیشه به زبان خودش نمایش داده می‌شود)
extern const char *lang_native_name(Lang l);

// ترجمه‌ی یک شناسه به زبان جاری
const char *tr(StrId id);

// ---- رجیستری برای به‌روزرسانی زنده‌ی لیبل‌های صفحه‌ی فعلی ----
void i18n_reset(void);                          // ابتدای هر create_screen
void i18n_register(lv_obj_t *label, StrId id);  // ثبت لیبل ایستا
void i18n_apply(void);                          // اعمال زبان روی لیبل‌های ثبت‌شده

#ifdef __cplusplus
}
#endif

#endif // LANG_H
