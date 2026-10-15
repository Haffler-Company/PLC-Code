#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *screen_splash;
    lv_obj_t *screen_password;
    lv_obj_t *screen_wifi;
    lv_obj_t *screen_framware;
    lv_obj_t *panle_wifi_password_1;
    lv_obj_t *frameware_password_keybord;
    lv_obj_t *text_area_framware_password;
    lv_obj_t *obj0;
    lv_obj_t *drop_down_wifi_list;
    lv_obj_t *btn_wifi_connect;
    lv_obj_t *btn_wifi_redresh;
    lv_obj_t *panle_wifi_password;
    lv_obj_t *obj1;
    lv_obj_t *keybord;
    lv_obj_t *text_area_wifi_password;
    lv_obj_t *language_panle;
    lv_obj_t *obj2;
    lv_obj_t *btn_single_dubble_install;
    lv_obj_t *btn_single_head_install;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *waiting_panle_2;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *waiting_panle;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *info_panle;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *seriall_number;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *obj24;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
    lv_obj_t *chane_gir;
    lv_obj_t *dubble_head_button;
    lv_obj_t *corner_cleaner_button;
    lv_obj_t *single_head_button;
    lv_obj_t *waiting_panle_1;
    lv_obj_t *obj28;
    lv_obj_t *obj29;
    lv_obj_t *download_metrics_label;
    lv_obj_t *download_cancel_btn;
    lv_obj_t *download_cancel_label;
    // ==== 4 برنامه‌ی جدید: دکمه‌ی نصب + لیبل ورژن زیر آن ====
    lv_obj_t *cutter_single_button;
    lv_obj_t *btn_cutter_single_install;
    lv_obj_t *ver_cutter_single;
    lv_obj_t *cutter_double_button;
    lv_obj_t *btn_cutter_double_install;
    lv_obj_t *ver_cutter_double;
    lv_obj_t *welder_double_button;
    lv_obj_t *btn_welder_double_install;
    lv_obj_t *ver_welder_double;
    lv_obj_t *gas_button;
    lv_obj_t *btn_gas_install;
    lv_obj_t *ver_gas;
    // لیبلِ نسخه برای ۴ کارتِ قدیمی (چانه‌گیر، جوش‌تک‌سر، تست، نرم‌افزار۱)
    lv_obj_t *ver_dough;
    lv_obj_t *ver_welder_single;
    lv_obj_t *ver_test;
    lv_obj_t *ver_sw1;
    // لیبلِ «ناسازگار» بالای دکمهٔ نصبِ کارت‌های وابسته به سخت‌افزار
    lv_obj_t *incompat_welder_single;
    lv_obj_t *incompat_welder_double;
    lv_obj_t *incompat_cutter_double;
    // ===== چیدمان جدید: ردیف افقیِ ۸ کارت + پنل تأیید نصب =====
    lv_obj_t *cards_row;
    lv_obj_t *card_btn[9];        // دکمه‌ی نصب هر کارت
    lv_obj_t *card_ver[9];        // لیبل نسخه/ناموجود
    lv_obj_t *card_incompat[9];   // لیبل «نرم‌افزار سازگار نیست»
    lv_obj_t *confirm_panel;
    lv_obj_t *confirm_label;
    lv_obj_t *confirm_yes;
    lv_obj_t *confirm_no;
    // ===== ورود لینک: دکمه در کارت HAFFLER + پنل مودال دانلود از URL دلخواه =====
    lv_obj_t *link_entry_btn;
    lv_obj_t *link_entry_panel;
    lv_obj_t *link_entry_textarea;
    lv_obj_t *link_entry_keyboard;
    lv_obj_t *link_entry_confirm;
    lv_obj_t *link_entry_cancel;
    lv_obj_t *link_entry_error;
    // ===== پاپ‌آپ اطلاعات ورژن (کلیک روی لیبل نسخه‌ی کارتِ Config) =====
    lv_obj_t *version_panel;
    lv_obj_t *version_upload;
    lv_obj_t *version_scroll;
    lv_obj_t *version_close;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_SCREEN_SPLASH = 1,
    SCREEN_ID_SCREEN_PASSWORD = 2,
    SCREEN_ID_SCREEN_WIFI = 3,
    SCREEN_ID_SCREEN_FRAMWARE = 4,
};

void create_screen_screen_splash();
void delete_screen_screen_splash();
void tick_screen_screen_splash();

void create_screen_screen_password();
void delete_screen_screen_password();
void tick_screen_screen_password();

void create_screen_screen_wifi();
void delete_screen_screen_wifi();
void tick_screen_screen_wifi();

void create_screen_screen_framware();
void delete_screen_screen_framware();
void tick_screen_screen_framware();

enum Themes {
    THEME_ID_DARK,
    THEME_ID_LIGHT,
};
enum Colors {
    COLOR_ID_BUTTON_GREY_1,
    COLOR_ID_B1,
    COLOR_ID_G1,
};
void change_color_theme(uint32_t themeIndex);
extern uint32_t theme_colors[2][3];

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/