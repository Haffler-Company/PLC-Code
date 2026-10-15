#ifndef LVGL_FUNCTION_H
#define LVGL_FUNCTION_H

#include <lvgl.h>
#include "ui.h"
#include <Arduino.h>
#include "general.h"
#include "ui_HardwareDiagnostics.h"
#include "ui_notification.h"
#include "lang.h"
#include "day_lock.h"
#include "web_portal.h"
#include "ota_manager.h"
#include "ui_WiFiScreen.h"
#include "ui_ProjectInfoScreen.h"
#include "ui_HelpScreen.h"
#include "panel_nav.h"

extern ScreenMap currentScreen;
extern String tempKeyboardText;
extern Storage storage;
extern bool processActive;
extern bool isAutoScreenActive;
extern bool cycleStarted;
extern bool shouldBeep;
extern TaskHandle_t buzzerTaskHandle;
extern SemaphoreHandle_t lv_mutex;

static inline bool lv_lock(TickType_t timeout = pdMS_TO_TICKS(50))
{
    if (lv_mutex == NULL) return true;
    return (xSemaphoreTakeRecursive(lv_mutex, timeout) == pdTRUE);
}

static inline void lv_unlock()
{
    if (lv_mutex != NULL)
    {
        xSemaphoreGiveRecursive(lv_mutex);
    }
}

extern bool isFunneleLeft;
extern bool isFunneleRight;

extern bool isPistonUp;
extern bool isPistonDown;

String keypadMode = ""; // S  //R

//! ------- DECLARE MISSING PROTOTYPES -------
void go_to_menu(lv_obj_t *from);
void go_to_automaticMode(lv_obj_t *from);
void go_to_manualMode(lv_obj_t *from);
void go_to_setting(lv_obj_t *from);
void go_to_menu_from_splash(lv_obj_t *from);

void menu_screen_events_init();
void automaticMode_screen_events_init();
void manual_screen_events_init();
void warning_next_btn(lv_event_t *e);
void menue_automatic_btn(lv_event_t *e);
void menue_manual_btn(lv_event_t *e);
void menue_settingImage_btn(lv_event_t *e);
void menue_lang_btn(lv_event_t *e);
void automatic_back_btn(lv_event_t *e);
void manoal_back_btn(lv_event_t *e);
void menu_keyboard_valueChange(lv_event_t *e);
void setting_screen_events_init();
void setting_back_btn(lv_event_t *e);
void automatic_WeightButtonAdd_btn(lv_event_t *e);
void automatic_WeightButtonRemove_btn(lv_event_t *e);
void automatic_Speed_plus_btn(lv_event_t *e);
void automatic_Speed_remove_btn(lv_event_t *e);
void automatic_keyboard_valueChange(lv_event_t *e);
void automatic_weight_holder(lv_event_t *e);
void automatic_speed_holder(lv_event_t *e);
void automatic_CounterResetButton_btn(lv_event_t *e);
void setting_keyboard_valueChange(lv_event_t *e);
void setting_pistonValue_btn(lv_event_t *e);
void setting_FunnleValue_btn(lv_event_t *e);
void setting_PistonButtonAdd_btn(lv_event_t *e);
void setting_PistonButtonRemove_btn(lv_event_t *e);
void setting_FunnleButtonAdd_btn(lv_event_t *e);
void setting_FunnleButtonRremove_btn(lv_event_t *e);
void automatic_StartStopButton_btn(lv_event_t *e);
void manoal_PistonUp_btn(lv_event_t *e);
void manoal_PistonDown_btn(lv_event_t *e);
void manoal_FunnleBack_btn(lv_event_t *e);
void manoal_FunnleForward_btn(lv_event_t *e);
void automatic_SilentKey_btn(lv_event_t *e);
void go_to_diagnostics(lv_obj_t *from);
void setting_diag_btn(lv_event_t *e);
void setting_flag_ir_btn(lv_event_t *e);
void setting_flag_gb_btn(lv_event_t *e);
void setting_flag_tr_btn(lv_event_t *e);
void setting_flag_ru_btn(lv_event_t *e);
void setting_wifi_switch_changed(lv_event_t *e);
void diagnostics_screen_events_init();
void diagnostics_back_btn(lv_event_t *e);
void diag_funnel_btn(lv_event_t *e);
void diag_piston_btn(lv_event_t *e);
void diag_inv_fwd_cb(lv_event_t *e);
void diag_inv_rev_cb(lv_event_t *e);
void diag_buzzer_btn(lv_event_t *e);
void setting_info_btn(lv_event_t *e);
void setting_help_btn(lv_event_t *e);
void setting_wifi_holder_btn(lv_event_t *e);
void menu_model_btn(lv_event_t *e);

//! ------- CUSTOM UI -------
void custom_ui_init(void)
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                              false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    ui____initial_actions0 = lv_obj_create(NULL);

    ScreenMap resume_target = MAIN;
    if (ui_resume_boot_get_latched(&resume_target))
    {
        //? Seamless crash/hang recovery: Skip splash and warning!
        Serial.printf("[UIRESUME] Hang recovery detected! Resuming screen %d\n", (int)resume_target);
        pistonStop();
        funneleStop();
        processActive = false;
        cycleStarted = false;

        ui_Menu_screen_init();
        menu_screen_events_init();

        switch (resume_target)
        {
            case AUTOMATIC:
                go_to_automaticMode(NULL);
                break;
            case MANUAL:
                go_to_manualMode(NULL);
                break;
            case SETTING:
                go_to_setting(NULL);
                break;
            case DIAGNOSTICS:
                go_to_diagnostics(NULL);
                break;
            case MAIN:
            default:
                currentScreen = MAIN;
                ui_resume_mark_current(MAIN);
                lv_scr_load(ui_Menu);
                lv_label_set_text(ui_Menu_Counter_Lable, String(storage.getChoneCounter()).c_str());
                break;
        }
        return;
    }

    //? Normal cold boot sequence:
    ui_SplashScreen_screen_init();
    ui_WarningPage_screen_init();
    ui_Menu_screen_init();
    menu_screen_events_init();
    lv_disp_load_scr(ui_SplashScreen);
    ui_resume_mark_current(WARNING);
}

//! ------- UI FUNCTIONS -------
void go_to_menu_from_splash(lv_obj_t *from)
{
    panel_nav_exit_mode();
    currentScreen = MAIN;
    ui_resume_mark_current(MAIN);

    menu_screen_events_init();
    lv_scr_load(ui_Menu);
    if (ui_SplashScreen != NULL)
    {
        lv_obj_del(ui_SplashScreen);
        ui_SplashScreen = NULL;
    }
    if (from != NULL)
    {
        if (from == ui_WarningPage) ui_WarningPage = NULL;
        lv_obj_del(from);
    }

    lv_label_set_text(ui_Menu_Counter_Lable, String(storage.getChoneCounter()).c_str());
}
void go_to_menu(lv_obj_t *from)
{
    panel_nav_exit_mode();
    currentScreen = MAIN;
    ui_resume_mark_current(MAIN);
    isAutoScreenActive = false; // Set inactive before deleting screen!

    menu_screen_events_init();

    i18n_reset();
    i18n_register(ui_Menu_AutomaticBTT, STR_AUTO_MODE);
    i18n_register(ui_Menu_ManualBTT, STR_MANUAL_MODE);
    i18n_apply();

    lv_scr_load(ui_Menu);
    if (from != NULL && from != ui_Menu)
    {
        if (from == ui_SettingPage) ui_SettingPage = NULL;
        else if (from == ui_AutomaticMode) ui_AutomaticMode = NULL;
        else if (from == ui_Manual_Mode) ui_Manual_Mode = NULL;
        else if (from == ui_HardwareDiagnostics) ui_HardwareDiagnostics = NULL;
        else if (from == ui_WarningPage) ui_WarningPage = NULL;
        lv_obj_del(from);
    }

    lv_label_set_text(ui_Menu_Counter_Lable, String(storage.getChoneCounter()).c_str());
}
void go_to_automaticMode(lv_obj_t *from)
{
    panel_nav_exit_mode();
    currentScreen = AUTOMATIC;
    ui_resume_mark_current(AUTOMATIC);

    if (ui_AutomaticMode != NULL)
    {
        lv_obj_del(ui_AutomaticMode);
        ui_AutomaticMode = NULL;
    }

    ui_AutomaticMode_screen_init();
    automaticMode_screen_events_init();

    i18n_reset();
    i18n_register(ui_AutomaticMode_WeightLable, STR_WEIGHT);
    i18n_register(ui_AutomaticMode_SpeedLable, STR_SPEED);
    i18n_register(ui_AutomaticMode_Counter_H1TextLable, STR_COUNT);
    i18n_register(ui_AutomaticMode_CounterResetBTT, STR_RESET_COUNT);
    i18n_register(ui_AutomaticMode_Label13, STR_BACK);
    i18n_apply();

    lv_scr_load(ui_AutomaticMode);

    lv_label_set_text(ui_AutomaticMode_WeightValue, String(storage.getAutoWeight()).c_str());
    lv_label_set_text(ui_AutomaticMode_SpeedValue, String(storage.getAutoSpeed()).c_str());
    lv_label_set_text(ui_AutomaticMode_SpeedValue1, String(storage.getChoneCounter()).c_str());

    if (storage.get_activeCounterSound())
    {
        lv_obj_clear_state(ui_AutomaticMode_Silent_key, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_add_state(ui_AutomaticMode_Silent_key, LV_STATE_CHECKED);
    }

    isAutoScreenActive = true;

    if (from != NULL && from != ui_Menu && from != ui_AutomaticMode)
    {
        if (from == ui_SettingPage) ui_SettingPage = NULL;
        else if (from == ui_Manual_Mode) ui_Manual_Mode = NULL;
        else if (from == ui_HardwareDiagnostics) ui_HardwareDiagnostics = NULL;
        lv_obj_del(from);
    }
}
void go_to_manualMode(lv_obj_t *from)
{
    panel_nav_exit_mode();
    currentScreen = MANUAL;
    ui_resume_mark_current(MANUAL);

    if (ui_Manual_Mode != NULL)
    {
        lv_obj_del(ui_Manual_Mode);
        ui_Manual_Mode = NULL;
    }

    ui_Manual_Mode_screen_init();
    manual_screen_events_init();

    i18n_reset();
    i18n_register(ui_Manual_Mode_Label9, STR_BACK);
    i18n_register(ui_Manual_Mode_Label10, STR_FUNNEL_FWD);
    i18n_register(ui_Manual_Mode_Label11, STR_FUNNEL_REV);
    i18n_register(ui_Manual_Mode_Label12, STR_PISTON_UP);
    i18n_register(ui_Manual_Mode_Label14, STR_PISTON_DOWN);
    i18n_apply();

    lv_scr_load(ui_Manual_Mode);

    if (from != NULL && from != ui_Menu && from != ui_Manual_Mode)
    {
        if (from == ui_SettingPage) ui_SettingPage = NULL;
        else if (from == ui_AutomaticMode) ui_AutomaticMode = NULL;
        else if (from == ui_HardwareDiagnostics) ui_HardwareDiagnostics = NULL;
        lv_obj_del(from);
    }
}
void go_to_setting(lv_obj_t *from)
{
    panel_nav_exit_mode();
    currentScreen = SETTING;
    ui_resume_mark_current(SETTING);

    if (ui_SettingPage != NULL)
    {
        lv_obj_del(ui_SettingPage);
        ui_SettingPage = NULL;
    }

    ui_SettingPage_screen_init();
    setting_screen_events_init();

    i18n_reset();
    i18n_register(ui_SettingPage_Label15, STR_BACK);
    i18n_register(ui_SettingPage_PistonLable, STR_MAX_PISTON);
    i18n_register(ui_SettingPage_Funnle_Lable, STR_MAX_FUNNEL);
    i18n_register(ui_SettingPage_AutomaticLable3, STR_SETTINGS);
    i18n_register(ui_SettingPage_WifiTitle, STR_WIFI_TITLE);
    i18n_register(ui_SettingPage_LangTitle, STR_LANG_SELECT);
    i18n_register(ui_SettingPage_DiagTitle, STR_DIAGNOSTICS);
    i18n_register(ui_SettingPage_DiagBtnLabel, STR_IO_TEST);
    if (ui_SettingPage_InfoBtnLabel) {
        i18n_register(ui_SettingPage_InfoBtnLabel, STR_PROJECT_INFO);
    }
    if (ui_SettingPage_HelpBtnLabel) {
        i18n_register(ui_SettingPage_HelpBtnLabel, STR_HELP_MANUAL);
    }
    i18n_apply();

    lv_scr_load(ui_SettingPage);

    lv_label_set_text(ui_SettingPage_FunnleValue, String(storage.get_setting_max_funnle()).c_str());
    lv_label_set_text(ui_SettingPage_PistonValue, String(storage.getMaxPiston()).c_str());

    if (storage.getWifiEnabled()) {
        lv_obj_add_state(ui_SettingPage_WifiSwitch, LV_STATE_CHECKED);
        lv_label_set_text(ui_SettingPage_WifiLabel, "192.168.4.1");
        lv_obj_set_style_text_color(ui_SettingPage_WifiLabel, lv_color_hex(0x0288D1), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_clear_state(ui_SettingPage_WifiSwitch, LV_STATE_CHECKED);
        lv_label_set_text(ui_SettingPage_WifiLabel, (g_lang == LANG_FA) ? "خاموش" : "OFF");
        lv_obj_set_style_text_color(ui_SettingPage_WifiLabel, lv_color_hex(0x9E9E9E), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (from != NULL && from != ui_Menu && from != ui_SettingPage)
    {
        if (from == ui_HardwareDiagnostics) ui_HardwareDiagnostics = NULL;
        else if (from == ui_AutomaticMode) ui_AutomaticMode = NULL;
        else if (from == ui_Manual_Mode) ui_Manual_Mode = NULL;
        lv_obj_del(from);
    }
}
void go_to_diagnostics(lv_obj_t *from)
{
    panel_nav_exit_mode();
    currentScreen = DIAGNOSTICS;
    ui_resume_mark_current(DIAGNOSTICS);

    ui_HardwareDiagnostics_screen_init();
    diagnostics_screen_events_init();
    lv_scr_load(ui_HardwareDiagnostics);

    if (from != NULL && from != ui_Menu && from != ui_HardwareDiagnostics)
    {
        if (from == ui_SettingPage) ui_SettingPage = NULL;
        lv_obj_del(from);
    }
}
//! ------- INITIALIZE EVENTS -------
void warning_screen_events_init()
{
    if (ui_WarningPage_Button1 != NULL && lv_obj_is_valid(ui_WarningPage_Button1))
    {
        lv_obj_add_event_cb(ui_WarningPage_Button1, warning_next_btn, LV_EVENT_CLICKED, NULL);
    }
}

// ================= CUSTOM 3x4 INDUSTRIAL NUMERIC KEYPAD =================
static const char * custom_numeric_kb_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};

static const lv_btnmatrix_ctrl_t custom_numeric_kb_ctrl[] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
    1, 1, 1
};

static void custom_keyboard_draw_part_event_cb(lv_event_t * e)
{
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc && dsc->part == LV_PART_ITEMS) {
        lv_obj_t * kb = lv_event_get_target(e);
        const char * txt = lv_btnmatrix_get_btn_text(kb, dsc->id);
        if(txt) {
            uint16_t sel = lv_btnmatrix_get_selected_btn(kb);
            bool is_pressed = (sel == dsc->id) && (lv_obj_get_state(kb) & LV_STATE_PRESSED);

            if(strcmp(txt, LV_SYMBOL_OK) == 0) {
                // Confirm button: Vivid Green background, white symbol
                dsc->rect_dsc->bg_color = is_pressed ? lv_color_hex(0x059669) : lv_color_hex(0x10B981);
                dsc->rect_dsc->bg_opa = LV_OPA_COVER;
                dsc->label_dsc->color = lv_color_hex(0xFFFFFF);
            }
            else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
                // Backspace button: Vivid Red background, white symbol
                dsc->rect_dsc->bg_color = is_pressed ? lv_color_hex(0xDC2626) : lv_color_hex(0xEF4444);
                dsc->rect_dsc->bg_opa = LV_OPA_COVER;
                dsc->label_dsc->color = lv_color_hex(0xFFFFFF);
            }
            else if(is_pressed) {
                dsc->rect_dsc->bg_color = lv_color_hex(0x3F3F46);
            }
        }
    }
}

static void setup_custom_numeric_keyboard(lv_obj_t * kb)
{
    if(!kb) return;

    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_NUMBER, (const char **)custom_numeric_kb_map, custom_numeric_kb_ctrl);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);

    lv_obj_set_width(kb, 456);
    lv_obj_set_height(kb, 194);
    lv_obj_set_x(kb, 0);
    lv_obj_set_y(kb, 57);
    lv_obj_set_align(kb, LV_ALIGN_CENTER);

    lv_obj_set_style_text_font(kb, &lv_font_montserrat_22, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(kb, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(kb, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(kb, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(kb, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(kb, custom_keyboard_draw_part_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
}

// ================= PROGRESSIVE HOLD-TO-REPEAT ACCELERATION =================
static uint16_t s_hold_repeat_count = 0;

static int get_step_acceleration(lv_event_code_t code)
{
    if (code == LV_EVENT_SHORT_CLICKED)
    {
        s_hold_repeat_count = 0;
        return 1;
    }
    else if (code == LV_EVENT_LONG_PRESSED)
    {
        s_hold_repeat_count = 1;
        return 1;
    }
    else if (code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        s_hold_repeat_count++;
        if (s_hold_repeat_count < 6) return 1;
        if (s_hold_repeat_count < 16) return 5;
        if (s_hold_repeat_count < 30) return 20;
        return 50;
    }
    else if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return 0;
    }
    return 0;
}

void menu_screen_events_init()
{
    static bool s_menu_events_initialized = false;
    if (s_menu_events_initialized) return;
    s_menu_events_initialized = true;

    setup_custom_numeric_keyboard(ui_Menu_KeyboardSetting);

    lv_obj_add_event_cb(ui_Menu_AutomaticModeButton, menue_automatic_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Menu_ManualModeButton, menue_manual_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Menu_SettingImageButton, menue_settingImage_btn, LV_EVENT_CLICKED, NULL);
    if (ui_Menu_LangButton) {
        lv_obj_add_event_cb(ui_Menu_LangButton, menue_lang_btn, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_add_event_cb(ui_Menu_KeyboardSetting, menu_keyboard_valueChange, LV_EVENT_VALUE_CHANGED, NULL);
}
void automaticMode_screen_events_init()
{
    setup_custom_numeric_keyboard(ui_AutomaticMode_ModifyPanleKeybord);

    lv_obj_add_event_cb(ui_AutomaticMode_BackButton, automatic_back_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(ui_AutomaticMode_WeightButtonAdd, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_flag(ui_AutomaticMode_WeightValueRemove, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_flag(ui_AutomaticMode_SpeedButtonAdd, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_flag(ui_AutomaticMode_SpeedValueRemove, LV_OBJ_FLAG_PRESS_LOCK);

    lv_obj_add_event_cb(ui_AutomaticMode_WeightButtonAdd, automatic_WeightButtonAdd_btn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_AutomaticMode_WeightValueRemove, automatic_WeightButtonRemove_btn, LV_EVENT_ALL, NULL);

    lv_obj_add_event_cb(ui_AutomaticMode_SpeedButtonAdd, automatic_Speed_plus_btn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_AutomaticMode_SpeedValueRemove, automatic_Speed_remove_btn, LV_EVENT_ALL, NULL);

    lv_obj_add_event_cb(ui_AutomaticMode_CounterResetButton, automatic_CounterResetButton_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_AutomaticMode_WeightValue, automatic_weight_holder, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_AutomaticMode_SpeedValue, automatic_speed_holder, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_AutomaticMode_ModifyPanleKeybord, automatic_keyboard_valueChange, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_event_cb(ui_AutomaticMode_StartStopButton, automatic_StartStopButton_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_AutomaticMode_Silent_key, automatic_SilentKey_btn, LV_EVENT_VALUE_CHANGED, NULL);
}
void manual_screen_events_init()
{
    lv_obj_add_event_cb(ui_Manual_Mode_BackButton1, manoal_back_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_Manual_Mode_PistonUpButton, manoal_PistonUp_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Manual_Mode_PistonDownButton, manoal_PistonDown_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_Manual_Mode_Funnle_BackButton, manoal_FunnleBack_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Manual_Mode_Funnle_ForwardButton, manoal_FunnleForward_btn, LV_EVENT_CLICKED, NULL);
}
void setting_screen_events_init()
{
    setup_custom_numeric_keyboard(ui_SettingPage_SettingPanleKeybord);

    lv_obj_add_event_cb(ui_SettingPage_BackButton2, setting_back_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_SettingPage_SettingPanleKeybord, setting_keyboard_valueChange, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_event_cb(ui_SettingPage_PistonValue, setting_pistonValue_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_SettingPage_FunnleValue, setting_FunnleValue_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(ui_SettingPage_PistonButtonAdd, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_flag(ui_SettingPage_PistonButtonRemove, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_flag(ui_SettingPage_FunnleButtonAdd, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_flag(ui_SettingPage_FunnleButtonRremove, LV_OBJ_FLAG_PRESS_LOCK);

    lv_obj_add_event_cb(ui_SettingPage_PistonButtonAdd, setting_PistonButtonAdd_btn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SettingPage_PistonButtonRemove, setting_PistonButtonRemove_btn, LV_EVENT_ALL, NULL);

    lv_obj_add_event_cb(ui_SettingPage_FunnleButtonAdd, setting_FunnleButtonAdd_btn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SettingPage_FunnleButtonRremove, setting_FunnleButtonRremove_btn, LV_EVENT_ALL, NULL);

    lv_obj_add_event_cb(ui_SettingPage_DiagButton, setting_diag_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_SettingPage_FlagIR, setting_flag_ir_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_SettingPage_FlagGB, setting_flag_gb_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_SettingPage_FlagTR, setting_flag_tr_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_SettingPage_FlagRU, setting_flag_ru_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_SettingPage_WifiSwitch, setting_wifi_switch_changed, LV_EVENT_VALUE_CHANGED, NULL);
    if (ui_SettingPage_WifiHolder) {
        lv_obj_add_event_cb(ui_SettingPage_WifiHolder, setting_wifi_holder_btn, LV_EVENT_CLICKED, NULL);
    }
    if (ui_SettingPage_InfoButton) {
        lv_obj_add_event_cb(ui_SettingPage_InfoButton, setting_info_btn, LV_EVENT_CLICKED, NULL);
    }
    if (ui_SettingPage_HelpButton) {
        lv_obj_add_event_cb(ui_SettingPage_HelpButton, setting_help_btn, LV_EVENT_CLICKED, NULL);
    }
}
void diagnostics_screen_events_init()
{
    lv_obj_add_event_cb(ui_diag_btn_back, diagnostics_back_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_diag_btn_funnel, diag_funnel_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_diag_btn_piston, diag_piston_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_diag_btn_inv_fwd, diag_inv_fwd_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_diag_btn_inv_rev, diag_inv_rev_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_diag_btn_buzzer, diag_buzzer_btn, LV_EVENT_CLICKED, NULL);
}
//! ------- UI EVENT CALLBACK -------
void warning_next_btn(lv_event_t *e)
{
    go_to_menu_from_splash(ui_WarningPage);
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);
    ui_show_notification(NOTIF_SUCCESS, "ورود به سیستم", "خوش آمدید - دستگاه چانه گیر آماده به کار است");
}
void menue_automatic_btn(lv_event_t *e)
{
    if (day_lock_is_locked())
    {
        sound_play(SOUND_ALARM_FAULT);
        ui_show_notification(NOTIF_ERROR, "خطای قفل دستگاه", "مهلت کارکرد به پایان رسیده است");
        dev_lock_ui_show();
        return;
    }
    go_to_automaticMode(ui_Menu);
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);
    ui_show_notification(NOTIF_INFO, "حالت اتوماتیک", "صفحه کنترل فرآیند اتوماتیک آماده به کار");
}
void menue_manual_btn(lv_event_t *e)
{
    if (day_lock_is_locked())
    {
        sound_play(SOUND_ALARM_FAULT);
        ui_show_notification(NOTIF_ERROR, "خطای قفل دستگاه", "مهلت کارکرد به پایان رسیده است");
        dev_lock_ui_show();
        return;
    }
    go_to_manualMode(ui_Menu);
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);
    ui_show_notification(NOTIF_INFO, "حالت دستی", "کنترل دستی محرک ها و سنسورها فعال گردید");
}
void menu_keyboard_valueChange(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
        const char *btn_text = lv_btnmatrix_get_btn_text(kb, btn_id);

        if (btn_text)
        {
            if (strcmp(btn_text, LV_SYMBOL_OK) == 0)
            {
                if (tempKeyboardText == "123456")
                {
                    lv_obj_add_flag(ui_Menu_SettingPasswordPanle, LV_OBJ_FLAG_HIDDEN);
                    tempKeyboardText = "";
                    lv_label_set_text(ui_Menu_PaswordLable, tempKeyboardText.c_str());

                    go_to_setting(ui_Menu);
                    ui_show_notification(NOTIF_SUCCESS, "ورود موفق", "دسترسی به صفحه تنظیمات تایید شد");
                }
                else
                {
                    // Wrong password feedback (Apple HIG clear feedback)
                    tempKeyboardText = "";
                    lv_label_set_text(ui_Menu_PaswordLable, "رمز اشتباه است");
                    ui_show_notification(NOTIF_ERROR, "خطای رمز عبور", "رمز ورود به تنظیمات نادرست است");
                }
            }

            else if (strcmp(btn_text, LV_SYMBOL_BACKSPACE) == 0)
            {
                if (tempKeyboardText == "رمز اشتباه است") tempKeyboardText = "";
                if (tempKeyboardText.length() > 0)
                {
                    tempKeyboardText = tempKeyboardText.substring(0, tempKeyboardText.length() - 1);
                    lv_label_set_text(ui_Menu_PaswordLable, tempKeyboardText.c_str());
                }
                else
                {
                    lv_label_set_text(ui_Menu_PaswordLable, "");
                }
            }
            else
            {
                String txt = btn_text;
                if ((btn_text == "0" || btn_text == "1" || btn_text == "2" || btn_text == "3" || btn_text == "4" || btn_text == "5" || btn_text == "6" || btn_text == "7" || btn_text == "8" || btn_text == "9") && tempKeyboardText.length() < 12)
                {
                    if (tempKeyboardText == "رمز اشتباه است") tempKeyboardText = "";
                    tempKeyboardText += btn_text;
                    lv_label_set_text(ui_Menu_PaswordLable, tempKeyboardText.c_str());
                }
            }
        }
    }
}
void manoal_back_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    pistonStop();
    funneleStop();
    setInverterDirection(0);
    isPistonDown = false;
    isFunneleLeft = false;
    isFunneleRight = false;
    isPistonUp = false;

    go_to_menu(ui_Manual_Mode);
    ui_show_notification(NOTIF_INFO, "خروج از حالت دستی", "کلیه موتورها خاموش و به منو بازگشت");
}
void menue_settingImage_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    go_to_setting(ui_Menu);
}

void menue_lang_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    lang_cycle();
    storage.setLanguage((uint8_t)g_lang);

    if (ui_Menu_LangLabel && lv_obj_is_valid(ui_Menu_LangLabel)) {
        const char *code = (g_lang == LANG_FA) ? "FA" : ((g_lang == LANG_EN) ? "EN" : ((g_lang == LANG_TR) ? "TR" : "RU"));
        lv_label_set_text(ui_Menu_LangLabel, code);
    }

    sound_play(SOUND_CLICK);
    ui_show_notification(NOTIF_INFO, "زبان انتخاب شده", lang_native_name(g_lang));
}

void manoal_PistonUp_btn(lv_event_t *e)
{
    if (digitalRead(PISTON_SENSOR) != LOW)
    {
        ui_show_notification(NOTIF_WARNING, "محدوده مجاز حرکت", "پیستون هم اکنون در بالاترین حد هوم قرار دارد");
        sound_play(SOUND_WARNING);
        return;
    }
    xTaskNotifyGive(buzzerTaskHandle);

    isPistonDown = false;
    isFunneleLeft = false;
    isFunneleRight = false;
    isPistonUp = true;
    ui_show_notification(NOTIF_INFO, "کنترل دستی پیستون", "فرمان حرکت پیستون به سمت بالا صادر شد");
}
void manoal_PistonDown_btn(lv_event_t *e)
{
    xTaskNotifyGive(buzzerTaskHandle);

    isPistonUp = false;
    isFunneleLeft = false;
    isFunneleRight = false;
    isPistonDown = true;
    ui_show_notification(NOTIF_INFO, "کنترل دستی پیستون", "فرمان حرکت پیستون به سمت پایین صادر شد");
}
void manoal_FunnleBack_btn(lv_event_t *e)
{
    if (digitalRead(FUNNELE_SENSOR) != LOW)
    {
        ui_show_notification(NOTIF_WARNING, "محدوده مجاز حرکت", "قیف هم اکنون در موقعیت اولیه هوم قرار دارد");
        sound_play(SOUND_WARNING);
        return;
    }
    xTaskNotifyGive(buzzerTaskHandle);

    isPistonUp = false;
    isFunneleRight = false;
    isPistonDown = false;
    isFunneleLeft = true;
    ui_show_notification(NOTIF_INFO, "کنترل دستی قیف", "فرمان بازگشت قیف به موقعیت اولیه صادر شد");
}
void manoal_FunnleForward_btn(lv_event_t *e)
{
    xTaskNotifyGive(buzzerTaskHandle);

    isPistonUp = false;
    isPistonDown = false;
    isFunneleLeft = false;
    isFunneleRight = true;
    ui_show_notification(NOTIF_INFO, "کنترل دستی قیف", "فرمان حرکت قیف به سمت جلو صادر شد");
}

void setting_back_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    ui_show_notification(NOTIF_SUCCESS, "تنظیمات ذخیره شد", "کلیه تغییرات در حافظه پایدار NVS ذخیره شد");
    go_to_menu(ui_SettingPage);
}

void automatic_WeightButtonAdd_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int weight = storage.getAutoWeight();
    if (weight < 2500)
    {
        weight += step;
        if (weight > 2500) weight = 2500;
        storage.setAutoWeight(weight);
        lv_label_set_text(ui_AutomaticMode_WeightValue, String(weight).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "محدوده وزن", "به حداکثر وزن مجاز ۲۵۰۰ میلی ثانیه رسیدید");
    }
}
void automatic_WeightButtonRemove_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int weight = storage.getAutoWeight();
    if (weight > 50)
    {
        weight -= step;
        if (weight < 50) weight = 50;
        storage.setAutoWeight(weight);
        lv_label_set_text(ui_AutomaticMode_WeightValue, String(weight).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "محدوده وزن", "به حداقل وزن مجاز ۵۰ میلی ثانیه رسیدید");
    }
}
void automatic_Speed_plus_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int speed = storage.getAutoSpeed();
    if (speed < 3000)
    {
        speed += step;
        if (speed > 3000) speed = 3000;
        storage.setAutoSpeed(speed);
        lv_label_set_text(ui_AutomaticMode_SpeedValue, String(speed).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "محدوده وقفه", "به حداکثر وقفه مجاز ۳۰۰۰ میلی ثانیه رسیدید");
    }
}
void automatic_Speed_remove_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int speed = storage.getAutoSpeed();
    if (speed > 50)
    {
        speed -= step;
        if (speed < 50) speed = 50;
        storage.setAutoSpeed(speed);
        lv_label_set_text(ui_AutomaticMode_SpeedValue, String(speed).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "محدوده وقفه", "به حداقل وقفه مجاز ۵۰ میلی ثانیه رسیدید");
    }
}
void automatic_keyboard_valueChange(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
        const char *btn_text = lv_btnmatrix_get_btn_text(kb, btn_id);

        if (btn_text)
        {
            if (strcmp(btn_text, LV_SYMBOL_OK) == 0)
            {
                if (keypadMode == "W")
                {
                    int val = tempKeyboardText.toInt();
                    if (val >= 50 && val <= 2500)
                    {
                        storage.setAutoWeight(val);
                        lv_obj_add_flag(ui_AutomaticMode_ModifyValuePanle, LV_OBJ_FLAG_HIDDEN);
                        lv_label_set_text(ui_AutomaticMode_WeightValue, tempKeyboardText.c_str());
                        char msgBuf[64];
                        snprintf(msgBuf, sizeof(msgBuf), "زمان وزن: %d میلی ثانیه ذخیره شد", val);
                        ui_show_notification(NOTIF_SUCCESS, "تنظیمات ذخیره شد", msgBuf);
                        sound_play(SOUND_CLICK);
                    }
                    else
                    {
                        ui_show_notification(NOTIF_ERROR, "خطای مقدار ورودی", "مقدار وزن باید بین ۵۰ تا ۲۵۰۰ میلی ثانیه باشد");
                        sound_play(SOUND_ALARM_FAULT);
                    }
                }
                else if (keypadMode == "S")
                {
                    int val = tempKeyboardText.toInt();
                    if (val >= 50 && val <= 3000)
                    {
                        storage.setAutoSpeed(val);
                        lv_obj_add_flag(ui_AutomaticMode_ModifyValuePanle, LV_OBJ_FLAG_HIDDEN);
                        lv_label_set_text(ui_AutomaticMode_SpeedValue, tempKeyboardText.c_str());
                        char msgBuf[64];
                        snprintf(msgBuf, sizeof(msgBuf), "وقفه پیستون: %d میلی ثانیه ذخیره شد", val);
                        ui_show_notification(NOTIF_SUCCESS, "تنظیمات ذخیره شد", msgBuf);
                        sound_play(SOUND_CLICK);
                    }
                    else
                    {
                        ui_show_notification(NOTIF_ERROR, "خطای مقدار ورودی", "وقفه پیستون باید بین ۵۰ تا ۳۰۰۰ میلی ثانیه باشد");
                        sound_play(SOUND_ALARM_FAULT);
                    }
                }
                else // keypadMode == "R" (Reset Counter)
                {
                    if (tempKeyboardText == "654321")
                    {
                        storage.setChoneCounter(0);
                        lv_label_set_text(ui_AutomaticMode_SpeedValue1, "0");
                        lv_label_set_text(ui_Menu_Counter_Lable, "0");
                        lv_obj_add_flag(ui_AutomaticMode_ModifyValuePanle, LV_OBJ_FLAG_HIDDEN);
                        ui_show_notification(NOTIF_SUCCESS, "ریست شمارنده", "شمارشگر کل چانه ها با موفقیت صفر گردید");
                        sound_play(SOUND_CLICK);
                    }
                    else
                    {
                        ui_show_notification(NOTIF_ERROR, "کد تایید نامعتبر", "کد ۶ رقمی ریست شمارنده نادرست است");
                        sound_play(SOUND_ALARM_FAULT);
                        tempKeyboardText = "";
                        lv_label_set_text(ui_AutomaticMode_Valuelable, "");
                    }
                }
            }

            else if (strcmp(btn_text, LV_SYMBOL_BACKSPACE) == 0)
            {
                if (tempKeyboardText.length() > 0)
                {
                    tempKeyboardText = tempKeyboardText.substring(0, tempKeyboardText.length() - 1);
                    lv_label_set_text(ui_AutomaticMode_Valuelable, tempKeyboardText.c_str());
                }
            }
            else
            {
                String txt = btn_text;
                if ((btn_text == "0" || btn_text == "1" || btn_text == "2" || btn_text == "3" || btn_text == "4" || btn_text == "5" || btn_text == "6" || btn_text == "7" || btn_text == "8" || btn_text == "9") && tempKeyboardText.length() < 10)
                {
                    tempKeyboardText += btn_text;
                    lv_label_set_text(ui_AutomaticMode_Valuelable, tempKeyboardText.c_str());
                }
            }
        }
    }
}
void automatic_weight_holder(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    lv_label_set_text(ui_AutomaticMode_ModifyPanleH1Lable, String("وزن").c_str());
    lv_label_set_text(ui_AutomaticMode_Valuelable, String(storage.getAutoWeight()).c_str());

    lv_obj_clear_flag(ui_AutomaticMode_ModifyValuePanle, LV_OBJ_FLAG_HIDDEN);

    tempKeyboardText = String(storage.getAutoWeight());
    keypadMode = "W";
    ui_show_notification(NOTIF_INFO, "تنظیم زمان وزن", "مقدار وزن را بین ۵۰ تا ۲۵۰۰ میلی ثانیه وارد کنید");
}
void automatic_speed_holder(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    lv_label_set_text(ui_AutomaticMode_ModifyPanleH1Lable, String("وقفه پیستون").c_str());
    lv_label_set_text(ui_AutomaticMode_Valuelable, String(storage.getAutoSpeed()).c_str());

    lv_obj_clear_flag(ui_AutomaticMode_ModifyValuePanle, LV_OBJ_FLAG_HIDDEN);

    tempKeyboardText = String(storage.getAutoSpeed());
    keypadMode = "S";
    ui_show_notification(NOTIF_INFO, "تنظیم وقفه پیستون", "مقدار وقفه را بین ۵۰ تا ۳۰۰۰ میلی ثانیه وارد کنید");
}
void automatic_back_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    if (processActive)
    {
        ui_show_notification(NOTIF_WARNING, "فرآیند در حال اجرا", "جهت بازگشت، ابتدا دکمه توقف را فشار دهید");
        return;
    }

    ui_show_notification(NOTIF_INFO, "بازگشت به منو", "خروج از صفحه کنترل اتوماتیک");
    go_to_menu(ui_AutomaticMode);
}
void automatic_CounterResetButton_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    lv_label_set_text(ui_AutomaticMode_ModifyPanleH1Lable, String("ریست شمارشگر").c_str());
    lv_label_set_text(ui_AutomaticMode_Valuelable, String("").c_str());

    lv_obj_clear_flag(ui_AutomaticMode_ModifyValuePanle, LV_OBJ_FLAG_HIDDEN);

    tempKeyboardText = "";
    keypadMode = "R";
    ui_show_notification(NOTIF_INFO, "ریست شمارنده", "کد امنیتی ۶ رقمی را جهت صفر کردن وارد نمایید");
}
void automatic_StartStopButton_btn(lv_event_t *e)
{
    if (processActive)
    {
        processActive = false;
        cycleStarted = false;
        sound_play(SOUND_STOP_CYCLE);

        lv_label_set_text(ui_AutomaticMode_StartStopBTT, "شروع");
        lv_obj_clear_state(ui_AutomaticMode_StartStopButton, LV_STATE_USER_1);
        lv_obj_clear_state(ui_AutomaticMode_BackButton, LV_STATE_DISABLED);
        lv_obj_clear_state(ui_AutomaticMode_SetupHolder, LV_STATE_USER_1);
        ui_show_notification(NOTIF_INFO, "توقف فرآیند", "تولید اتوماتیک چانه توسط اپراتور متوقف گردید");
    }
    else
    {
        if (day_lock_is_locked())
        {
            sound_play(SOUND_ALARM_FAULT);
            ui_show_notification(NOTIF_ERROR, "خطای قفل دستگاه", "مهلت کارکرد به پایان رسیده است");
            dev_lock_ui_show();
            return;
        }

        processActive = true;
        cycleStarted = true;
        sound_play(SOUND_START_CYCLE);

        lv_label_set_text(ui_AutomaticMode_StartStopBTT, "توقف");
        lv_obj_add_state(ui_AutomaticMode_StartStopButton, LV_STATE_USER_1);
        lv_obj_add_state(ui_AutomaticMode_BackButton, LV_STATE_DISABLED);
        lv_obj_add_state(ui_AutomaticMode_SetupHolder, LV_STATE_USER_1);
        ui_show_notification(NOTIF_SUCCESS, "شروع فرآیند اتوماتیک", "چرخه تولید چانه با موفقیت آغاز شد");
    }
}
void automatic_SilentKey_btn(lv_event_t *e)
{
    xTaskNotifyGive(buzzerTaskHandle);

    bool new_state = !storage.get_activeCounterSound();
    storage.set_ActiveCounterSound(new_state);

    if (new_state)
    {
        ui_show_notification(NOTIF_SUCCESS, "بلندگو فعال شد", "صدای بیپ شمارنده فعال است");
    }
    else
    {
        ui_show_notification(NOTIF_INFO, "بلندگو قطع شد", "صدای بیپ شمارنده غیرفعال شد");
    }
}

void setting_keyboard_valueChange(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
        const char *btn_text = lv_btnmatrix_get_btn_text(kb, btn_id);

        if (btn_text)
        {
            if (strcmp(btn_text, LV_SYMBOL_OK) == 0)
            {
                if (keypadMode == "MP")
                {
                    int val = tempKeyboardText.toInt();
                    if (val >= 50 && val <= 3000)
                    {
                        storage.set_setting_max_piston(val);
                        lv_obj_add_flag(ui_SettingPage_SettingModifyValuePanle, LV_OBJ_FLAG_HIDDEN);
                        lv_label_set_text(ui_SettingPage_PistonValue, tempKeyboardText.c_str());
                        char msgBuf[64];
                        snprintf(msgBuf, sizeof(msgBuf), "حد پیستون: %d میلی ثانیه ذخیره شد", val);
                        ui_show_notification(NOTIF_SUCCESS, "تنظیمات ذخیره شد", msgBuf);
                        sound_play(SOUND_CLICK);
                    }
                    else
                    {
                        ui_show_notification(NOTIF_ERROR, "خطای مقدار ورودی", "حد پیستون باید بین ۵۰ تا ۳۰۰۰ میلی ثانیه باشد");
                        sound_play(SOUND_ALARM_FAULT);
                    }
                }
                else // keypadMode == "MF"
                {
                    int val = tempKeyboardText.toInt();
                    if (val >= 50 && val <= 3000)
                    {
                        storage.set_setting_max_funnle(val);
                        lv_obj_add_flag(ui_SettingPage_SettingModifyValuePanle, LV_OBJ_FLAG_HIDDEN);
                        lv_label_set_text(ui_SettingPage_FunnleValue, tempKeyboardText.c_str());
                        char msgBuf[64];
                        snprintf(msgBuf, sizeof(msgBuf), "حد قیف: %d میلی ثانیه ذخیره شد", val);
                        ui_show_notification(NOTIF_SUCCESS, "تنظیمات ذخیره شد", msgBuf);
                        sound_play(SOUND_CLICK);
                    }
                    else
                    {
                        ui_show_notification(NOTIF_ERROR, "خطای مقدار ورودی", "حد قیف باید بین ۵۰ تا ۳۰۰۰ میلی ثانیه باشد");
                        sound_play(SOUND_ALARM_FAULT);
                    }
                }
            }

            else if (strcmp(btn_text, LV_SYMBOL_BACKSPACE) == 0)
            {
                if (tempKeyboardText.length() > 0)
                {
                    tempKeyboardText = tempKeyboardText.substring(0, tempKeyboardText.length() - 1);
                    lv_label_set_text(ui_SettingPage_SettingValuelable, tempKeyboardText.c_str());
                }
            }
            else
            {
                String txt = btn_text;
                if ((btn_text == "0" || btn_text == "1" || btn_text == "2" || btn_text == "3" || btn_text == "4" || btn_text == "5" || btn_text == "6" || btn_text == "7" || btn_text == "8" || btn_text == "9") && tempKeyboardText.length() < 10)
                {
                    tempKeyboardText += btn_text;
                    lv_label_set_text(ui_SettingPage_SettingValuelable, tempKeyboardText.c_str());
                }
            }
        }
    }
}
void setting_pistonValue_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    keypadMode = "MP";

    lv_label_set_text(ui_SettingPage_SettingModifyPanleH1Lable, String("حد پیستون").c_str());
    lv_label_set_text(ui_SettingPage_SettingValuelable, String(storage.getMaxPiston()).c_str());

    lv_obj_clear_flag(ui_SettingPage_SettingModifyValuePanle, LV_OBJ_FLAG_HIDDEN);

    tempKeyboardText = String(storage.getMaxPiston());
    ui_show_notification(NOTIF_INFO, "تنظیم حد پیستون", "حد مجاز پیستون را بین ۵۰ تا ۳۰۰۰ میلی ثانیه وارد کنید");
}
void setting_FunnleValue_btn(lv_event_t *e)
{
    shouldBeep = true;
    xTaskNotifyGive(buzzerTaskHandle);

    keypadMode = "MF";

    lv_label_set_text(ui_SettingPage_SettingModifyPanleH1Lable, String("حد قیف").c_str());
    lv_label_set_text(ui_SettingPage_SettingValuelable, String(storage.get_setting_max_funnle()).c_str());

    lv_obj_clear_flag(ui_SettingPage_SettingModifyValuePanle, LV_OBJ_FLAG_HIDDEN);

    tempKeyboardText = String(storage.get_setting_max_funnle());
    ui_show_notification(NOTIF_INFO, "تنظیم حد قیف", "حد مجاز قیف را بین ۵۰ تا ۳۰۰۰ میلی ثانیه وارد کنید");
}

void setting_PistonButtonAdd_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int piston = storage.getMaxPiston();
    if (piston < 3000)
    {
        piston += step;
        if (piston > 3000) piston = 3000;
        storage.set_setting_max_piston(piston);
        lv_label_set_text(ui_SettingPage_PistonValue, String(piston).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "حد پیستون", "به حداکثر حد مجاز پیستون (۳۰۰۰ میلی ثانیه) رسیدید");
    }
}
void setting_PistonButtonRemove_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int piston = storage.getMaxPiston();
    if (piston > 50)
    {
        piston -= step;
        if (piston < 50) piston = 50;
        storage.set_setting_max_piston(piston);
        lv_label_set_text(ui_SettingPage_PistonValue, String(piston).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "حد پیستون", "به حداقل حد مجاز پیستون (۵۰ میلی ثانیه) رسیدید");
    }
}

void setting_FunnleButtonAdd_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int funnel = storage.get_setting_max_funnle();
    if (funnel < 3000)
    {
        funnel += step;
        if (funnel > 3000) funnel = 3000;
        storage.set_setting_max_funnle(funnel);
        lv_label_set_text(ui_SettingPage_FunnleValue, String(funnel).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "حد قیف", "به حداکثر حد مجاز قیف (۳۰۰۰ میلی ثانیه) رسیدید");
    }
}
void setting_FunnleButtonRremove_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED)
    {
        s_hold_repeat_count = 0;
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT)
    {
        return;
    }

    int step = get_step_acceleration(code);
    if (step <= 0) return;

    if (code == LV_EVENT_SHORT_CLICKED || (s_hold_repeat_count % 3 == 0))
    {
        shouldBeep = true;
        xTaskNotifyGive(buzzerTaskHandle);
    }

    int funnel = storage.get_setting_max_funnle();
    if (funnel > 50)
    {
        funnel -= step;
        if (funnel < 50) funnel = 50;
        storage.set_setting_max_funnle(funnel);
        lv_label_set_text(ui_SettingPage_FunnleValue, String(funnel).c_str());
    }
    else if (code == LV_EVENT_SHORT_CLICKED)
    {
        ui_show_notification(NOTIF_WARNING, "حد قیف", "به حداقل حد مجاز قیف (۵۰ میلی ثانیه) رسیدید");
    }
}

static void update_setting_flag_borders()
{
    if (ui_SettingPage_FlagIR && lv_obj_is_valid(ui_SettingPage_FlagIR)) {
        bool active = (g_lang == LANG_FA);
        lv_obj_set_style_border_width(ui_SettingPage_FlagIR, active ? 4 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_SettingPage_FlagIR, active ? lv_color_hex(0x0288D1) : lv_color_hex(0xD1D5DB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_SettingPage_FlagIR, active ? LV_OPA_COVER : 120, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_SettingPage_FlagGB && lv_obj_is_valid(ui_SettingPage_FlagGB)) {
        bool active = (g_lang == LANG_EN);
        lv_obj_set_style_border_width(ui_SettingPage_FlagGB, active ? 4 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_SettingPage_FlagGB, active ? lv_color_hex(0x0288D1) : lv_color_hex(0xD1D5DB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_SettingPage_FlagGB, active ? LV_OPA_COVER : 120, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_SettingPage_FlagTR && lv_obj_is_valid(ui_SettingPage_FlagTR)) {
        bool active = (g_lang == LANG_TR);
        lv_obj_set_style_border_width(ui_SettingPage_FlagTR, active ? 4 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_SettingPage_FlagTR, active ? lv_color_hex(0x0288D1) : lv_color_hex(0xD1D5DB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_SettingPage_FlagTR, active ? LV_OPA_COVER : 120, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_SettingPage_FlagRU && lv_obj_is_valid(ui_SettingPage_FlagRU)) {
        bool active = (g_lang == LANG_RU);
        lv_obj_set_style_border_width(ui_SettingPage_FlagRU, active ? 4 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_SettingPage_FlagRU, active ? lv_color_hex(0x0288D1) : lv_color_hex(0xD1D5DB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_SettingPage_FlagRU, active ? LV_OPA_COVER : 120, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void setting_flag_ir_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    lang_set(LANG_FA);
    storage.setLanguage((uint8_t)LANG_FA);
    update_setting_flag_borders();
    ui_show_notification(NOTIF_INFO, "تغییر زبان", "فارسی");
}

void setting_flag_gb_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    lang_set(LANG_EN);
    storage.setLanguage((uint8_t)LANG_EN);
    update_setting_flag_borders();
    ui_show_notification(NOTIF_INFO, "Language Changed", "English");
}

void setting_flag_tr_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    lang_set(LANG_TR);
    storage.setLanguage((uint8_t)LANG_TR);
    update_setting_flag_borders();
    ui_show_notification(NOTIF_INFO, "Dil Degistirildi", "Turkce");
}

void setting_flag_ru_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    lang_set(LANG_RU);
    storage.setLanguage((uint8_t)LANG_RU);
    update_setting_flag_borders();
    ui_show_notification(NOTIF_INFO, "Language Changed", "Russian");
}

void setting_wifi_switch_changed(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool en = lv_obj_has_state(sw, LV_STATE_CHECKED);
    sound_play(SOUND_CLICK);

    storage.setWifiEnabled(en);
    storage.save();

    if (en) {
        webPortal.begin();
        if (ui_SettingPage_WifiLabel && lv_obj_is_valid(ui_SettingPage_WifiLabel)) {
            lv_label_set_text(ui_SettingPage_WifiLabel, "192.168.4.1");
            lv_obj_set_style_text_color(ui_SettingPage_WifiLabel, lv_color_hex(0x0288D1), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        ui_show_notification(NOTIF_SUCCESS, (g_lang == LANG_FA) ? "وای فای" : "WiFi AP", (g_lang == LANG_FA) ? "وای فای روشن شد - 192.168.4.1" : "WiFi AP Started (192.168.4.1)");
    } else {
        webPortal.stop();
        if (ui_SettingPage_WifiLabel && lv_obj_is_valid(ui_SettingPage_WifiLabel)) {
            lv_label_set_text(ui_SettingPage_WifiLabel, (g_lang == LANG_FA) ? "خاموش" : "OFF");
            lv_obj_set_style_text_color(ui_SettingPage_WifiLabel, lv_color_hex(0x9E9E9E), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        ui_show_notification(NOTIF_WARNING, (g_lang == LANG_FA) ? "وای فای" : "WiFi AP", (g_lang == LANG_FA) ? "وای فای خاموش شد" : "WiFi AP Stopped");
    }
}

void setting_info_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    ui_ProjectInfoScreen_open();
}

void setting_help_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    ui_HelpScreen_open();
}

void setting_wifi_holder_btn(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == ui_SettingPage_WifiSwitch) {
        return;
    }
    sound_play(SOUND_CLICK);
    ui_WiFiScreen_open();
}

void menu_model_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    ui_ProjectInfoScreen_open();
}

//? ============================================================================
//? TECHNICIAN HARDWARE I/O DIAGNOSTICS EVENT HANDLERS (Siemens Standard)
//? ============================================================================
void setting_diag_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    ui_show_notification(NOTIF_WARNING, "تست سخت افزار", "ورود به صفحه عیب یابی و آزمایش مستقیم رله ها");
    go_to_diagnostics(ui_SettingPage);
}

static bool diag_funnel_motor_on = false;
static bool diag_piston_motor_on = false;

void diagnostics_back_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    // Unconditional Siemens fail-safe: Cut off all motor relays immediately!
    funneleStop();
    pistonStop();
    setInverterDirection(0);
    diag_funnel_motor_on = false;
    diag_piston_motor_on = false;
    ui_show_notification(NOTIF_INFO, "پایان تست قطعات", "کلیه محرک ها خاموش و به تنظیمات بازگشت");
    go_to_setting(ui_HardwareDiagnostics);
}

void diag_funnel_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    diag_funnel_motor_on = !diag_funnel_motor_on;
    if (diag_funnel_motor_on)
    {
        digitalWrite(FUNNELE_MOTOR, ON);
        lv_label_set_text(ui_diag_lbl_funnel, "موتور قیف Q1: روشن");
        lv_obj_set_style_bg_color(ui_diag_btn_funnel, lv_color_hex(0x2E7D32), LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_show_notification(NOTIF_WARNING, "تست موتور قیف Q1", "خروجی رله موتور قیف وصل شد");
    }
    else
    {
        digitalWrite(FUNNELE_MOTOR, OFF);
        lv_label_set_text(ui_diag_lbl_funnel, "موتور قیف Q1: خاموش");
        lv_obj_set_style_bg_color(ui_diag_btn_funnel, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_show_notification(NOTIF_INFO, "تست موتور قیف Q1", "موتور قیف خاموش شد");
    }
}

void diag_piston_btn(lv_event_t *e)
{
    sound_play(SOUND_CLICK);
    diag_piston_motor_on = !diag_piston_motor_on;
    if (diag_piston_motor_on)
    {
        digitalWrite(PISTON_MOTOR, ON);
        lv_label_set_text(ui_diag_lbl_piston, "موتور پیستون Q7: روشن");
        lv_obj_set_style_bg_color(ui_diag_btn_piston, lv_color_hex(0x2E7D32), LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_show_notification(NOTIF_WARNING, "تست موتور پیستون Q7", "خروجی رله موتور پیستون وصل شد");
    }
    else
    {
        digitalWrite(PISTON_MOTOR, OFF);
        lv_label_set_text(ui_diag_lbl_piston, "موتور پیستون Q7: خاموش");
        lv_obj_set_style_bg_color(ui_diag_btn_piston, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_show_notification(NOTIF_INFO, "تست موتور پیستون Q7", "موتور پیستون خاموش شد");
    }
}

void diag_inv_fwd_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED)
    {
        sound_play(SOUND_CLICK);
        setInverterDirection(1); // FWD
        ui_show_notification(NOTIF_WARNING, "تست اینورتر", "حرکت اینورتر رو به جلو (FWD) فعال است");
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        setInverterDirection(0); // STOP
        ui_show_notification(NOTIF_INFO, "تست اینورتر", "حرکت اینورتر متوقف شد");
    }
}

void diag_inv_rev_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED)
    {
        sound_play(SOUND_CLICK);
        setInverterDirection(2); // REV
        ui_show_notification(NOTIF_WARNING, "تست اینورتر", "حرکت اینورتر معکوس (REV) فعال است");
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        setInverterDirection(0); // STOP
        ui_show_notification(NOTIF_INFO, "تست اینورتر", "حرکت اینورتر متوقف شد");
    }
}

void diag_buzzer_btn(lv_event_t *e)
{
    sound_play(SOUND_ALARM_FAULT);
    ui_show_notification(NOTIF_INFO, "تست آژیر صوتی", "صدای آژیر با موفقیت آزمایش شد");
}

#endif
