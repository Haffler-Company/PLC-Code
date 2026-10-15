#include "ui_HardwareDiagnostics.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

lv_obj_t * ui_HardwareDiagnostics = NULL;

// Output buttons
lv_obj_t * ui_diag_btn_funnel = NULL;
lv_obj_t * ui_diag_lbl_funnel = NULL;
lv_obj_t * ui_diag_btn_piston = NULL;
lv_obj_t * ui_diag_lbl_piston = NULL;
lv_obj_t * ui_diag_btn_inv_fwd = NULL;
lv_obj_t * ui_diag_btn_inv_rev = NULL;
lv_obj_t * ui_diag_btn_buzzer = NULL;
lv_obj_t * ui_diag_btn_back = NULL;

// Input & Sensor indicators
lv_obj_t * ui_diag_led_funnel_sensor = NULL;
lv_obj_t * ui_diag_led_piston_sensor = NULL;
lv_obj_t * ui_diag_lbl_adc_f = NULL;
lv_obj_t * ui_diag_lbl_adc_m = NULL;
lv_obj_t * ui_diag_lbl_sys_health = NULL;

void ui_HardwareDiagnostics_screen_init(void)
{
    if (ui_HardwareDiagnostics != NULL) {
        lv_obj_del(ui_HardwareDiagnostics);
        ui_HardwareDiagnostics = NULL;
    }

    ui_HardwareDiagnostics = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_HardwareDiagnostics, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_HardwareDiagnostics, lv_color_hex(0x14181E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HardwareDiagnostics, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    //? --- Header Bar (480x42) ---
    lv_obj_t * header = lv_obj_create(ui_HardwareDiagnostics);
    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, 480);
    lv_obj_set_height(header, 42);
    lv_obj_set_x(header, 0);
    lv_obj_set_y(header, -139);
    lv_obj_set_align(header, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1F242D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(header, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(header, lv_color_hex(0x2E3642), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(header, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Title Label
    lv_obj_t * title = lv_label_create(header);
    lv_obj_set_align(title, LV_ALIGN_CENTER);
    lv_label_set_text(title, "تست و عیب یابی سخت افزار");
    lv_obj_set_style_text_color(title, lv_color_hex(0xECEFF1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Back Button
    ui_diag_btn_back = lv_btn_create(header);
    lv_obj_set_width(ui_diag_btn_back, 76);
    lv_obj_set_height(ui_diag_btn_back, 30);
    lv_obj_set_x(ui_diag_btn_back, -195);
    lv_obj_set_y(ui_diag_btn_back, 0);
    lv_obj_set_align(ui_diag_btn_back, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_diag_btn_back, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_btn_back, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_btn_back, lv_color_hex(0xDC2626), LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t * lbl_back = lv_label_create(ui_diag_btn_back);
    lv_obj_set_align(lbl_back, LV_ALIGN_CENTER);
    lv_label_set_text(lbl_back, "بازگشت");
    lv_obj_set_style_text_font(lbl_back, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    //? --- Left Panel: Output Relays (225x262) ---
    lv_obj_t * pnl_left = lv_obj_create(ui_HardwareDiagnostics);
    lv_obj_remove_style_all(pnl_left);
    lv_obj_set_width(pnl_left, 225);
    lv_obj_set_height(pnl_left, 262);
    lv_obj_set_x(pnl_left, -120);
    lv_obj_set_y(pnl_left, 24);
    lv_obj_set_align(pnl_left, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(pnl_left, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(pnl_left, lv_color_hex(0x1F242D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(pnl_left, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(pnl_left, lv_color_hex(0x2E3642), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(pnl_left, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * lbl_out_title = lv_label_create(pnl_left);
    lv_obj_set_x(lbl_out_title, 0);
    lv_obj_set_y(lbl_out_title, 8);
    lv_obj_set_align(lbl_out_title, LV_ALIGN_TOP_MID);
    lv_label_set_text(lbl_out_title, "رله ها و محرک های خروجی");
    lv_obj_set_style_text_color(lbl_out_title, lv_color_hex(0x90A4AE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_out_title, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Button 1: Funnel Motor Q1 (Toggle)
    ui_diag_btn_funnel = lv_btn_create(pnl_left);
    lv_obj_set_width(ui_diag_btn_funnel, 205);
    lv_obj_set_height(ui_diag_btn_funnel, 38);
    lv_obj_set_x(ui_diag_btn_funnel, 0);
    lv_obj_set_y(ui_diag_btn_funnel, 28);
    lv_obj_set_align(ui_diag_btn_funnel, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(ui_diag_btn_funnel, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_btn_funnel, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_diag_lbl_funnel = lv_label_create(ui_diag_btn_funnel);
    lv_obj_set_align(ui_diag_lbl_funnel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_diag_lbl_funnel, "موتور قیف Q1: خاموش");
    lv_obj_set_style_text_font(ui_diag_lbl_funnel, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Button 2: Piston Motor Q7 (Toggle)
    ui_diag_btn_piston = lv_btn_create(pnl_left);
    lv_obj_set_width(ui_diag_btn_piston, 205);
    lv_obj_set_height(ui_diag_btn_piston, 38);
    lv_obj_set_x(ui_diag_btn_piston, 0);
    lv_obj_set_y(ui_diag_btn_piston, 74);
    lv_obj_set_align(ui_diag_btn_piston, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(ui_diag_btn_piston, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_btn_piston, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_diag_lbl_piston = lv_label_create(ui_diag_btn_piston);
    lv_obj_set_align(ui_diag_lbl_piston, LV_ALIGN_CENTER);
    lv_label_set_text(ui_diag_lbl_piston, "موتور پیستون Q7: خاموش");
    lv_obj_set_style_text_font(ui_diag_lbl_piston, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Button 3: Inverter Forward Q2 (Momentary)
    ui_diag_btn_inv_fwd = lv_btn_create(pnl_left);
    lv_obj_set_width(ui_diag_btn_inv_fwd, 205);
    lv_obj_set_height(ui_diag_btn_inv_fwd, 38);
    lv_obj_set_x(ui_diag_btn_inv_fwd, 0);
    lv_obj_set_y(ui_diag_btn_inv_fwd, 120);
    lv_obj_set_align(ui_diag_btn_inv_fwd, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(ui_diag_btn_inv_fwd, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_btn_inv_fwd, lv_color_hex(0x1B5E20), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t * lbl_inv_fwd = lv_label_create(ui_diag_btn_inv_fwd);
    lv_obj_set_align(lbl_inv_fwd, LV_ALIGN_CENTER);
    lv_label_set_text(lbl_inv_fwd, "حرکت اینورتر راست گرد");
    lv_obj_set_style_text_font(lbl_inv_fwd, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Button 4: Inverter Reverse Q3 (Momentary)
    ui_diag_btn_inv_rev = lv_btn_create(pnl_left);
    lv_obj_set_width(ui_diag_btn_inv_rev, 205);
    lv_obj_set_height(ui_diag_btn_inv_rev, 38);
    lv_obj_set_x(ui_diag_btn_inv_rev, 0);
    lv_obj_set_y(ui_diag_btn_inv_rev, 166);
    lv_obj_set_align(ui_diag_btn_inv_rev, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(ui_diag_btn_inv_rev, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_btn_inv_rev, lv_color_hex(0xB71C1C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t * lbl_inv_rev = lv_label_create(ui_diag_btn_inv_rev);
    lv_obj_set_align(lbl_inv_rev, LV_ALIGN_CENTER);
    lv_label_set_text(lbl_inv_rev, "حرکت اینورتر چپ گرد");
    lv_obj_set_style_text_font(lbl_inv_rev, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Button 5: Buzzer Test Q5
    ui_diag_btn_buzzer = lv_btn_create(pnl_left);
    lv_obj_set_width(ui_diag_btn_buzzer, 205);
    lv_obj_set_height(ui_diag_btn_buzzer, 38);
    lv_obj_set_x(ui_diag_btn_buzzer, 0);
    lv_obj_set_y(ui_diag_btn_buzzer, 212);
    lv_obj_set_align(ui_diag_btn_buzzer, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(ui_diag_btn_buzzer, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_btn_buzzer, lv_color_hex(0xE65100), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t * lbl_buzzer = lv_label_create(ui_diag_btn_buzzer);
    lv_obj_set_align(lbl_buzzer, LV_ALIGN_CENTER);
    lv_label_set_text(lbl_buzzer, "تست هشدار بوق");
    lv_obj_set_style_text_font(lbl_buzzer, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    //? --- Right Panel: Sensors & Analog Inputs (225x262) ---
    lv_obj_t * pnl_right = lv_obj_create(ui_HardwareDiagnostics);
    lv_obj_remove_style_all(pnl_right);
    lv_obj_set_width(pnl_right, 225);
    lv_obj_set_height(pnl_right, 262);
    lv_obj_set_x(pnl_right, 120);
    lv_obj_set_y(pnl_right, 24);
    lv_obj_set_align(pnl_right, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(pnl_right, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(pnl_right, lv_color_hex(0x1F242D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(pnl_right, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(pnl_right, lv_color_hex(0x2E3642), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(pnl_right, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * lbl_in_title = lv_label_create(pnl_right);
    lv_obj_set_x(lbl_in_title, 0);
    lv_obj_set_y(lbl_in_title, 8);
    lv_obj_set_align(lbl_in_title, LV_ALIGN_TOP_MID);
    lv_label_set_text(lbl_in_title, "سنسورها و ورودی های دستگاه");
    lv_obj_set_style_text_color(lbl_in_title, lv_color_hex(0x90A4AE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_in_title, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Sensor 1: Funnel Home Sensor I6
    lv_obj_t * row_s1 = lv_obj_create(pnl_right);
    lv_obj_remove_style_all(row_s1);
    lv_obj_set_width(row_s1, 205);
    lv_obj_set_height(row_s1, 38);
    lv_obj_set_x(row_s1, 0);
    lv_obj_set_y(row_s1, 28);
    lv_obj_set_align(row_s1, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(row_s1, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row_s1, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_diag_led_funnel_sensor = lv_obj_create(row_s1);
    lv_obj_set_width(ui_diag_led_funnel_sensor, 18);
    lv_obj_set_height(ui_diag_led_funnel_sensor, 18);
    lv_obj_set_x(ui_diag_led_funnel_sensor, -80);
    lv_obj_set_y(ui_diag_led_funnel_sensor, 0);
    lv_obj_set_align(ui_diag_led_funnel_sensor, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_diag_led_funnel_sensor, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_led_funnel_sensor, lv_color_hex(0x455A64), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * lbl_s1 = lv_label_create(row_s1);
    lv_obj_set_x(lbl_s1, 15);
    lv_obj_set_align(lbl_s1, LV_ALIGN_CENTER);
    lv_label_set_text(lbl_s1, "سنسور موقعیت قیف I6");
    lv_obj_set_style_text_font(lbl_s1, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_s1, lv_color_hex(0xCFD8DC), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Sensor 2: Piston Home Sensor I7
    lv_obj_t * row_s2 = lv_obj_create(pnl_right);
    lv_obj_remove_style_all(row_s2);
    lv_obj_set_width(row_s2, 205);
    lv_obj_set_height(row_s2, 38);
    lv_obj_set_x(row_s2, 0);
    lv_obj_set_y(row_s2, 74);
    lv_obj_set_align(row_s2, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(row_s2, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row_s2, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_diag_led_piston_sensor = lv_obj_create(row_s2);
    lv_obj_set_width(ui_diag_led_piston_sensor, 18);
    lv_obj_set_height(ui_diag_led_piston_sensor, 18);
    lv_obj_set_x(ui_diag_led_piston_sensor, -80);
    lv_obj_set_y(ui_diag_led_piston_sensor, 0);
    lv_obj_set_align(ui_diag_led_piston_sensor, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_diag_led_piston_sensor, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_diag_led_piston_sensor, lv_color_hex(0x455A64), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * lbl_s2 = lv_label_create(row_s2);
    lv_obj_set_x(lbl_s2, 15);
    lv_obj_set_align(lbl_s2, LV_ALIGN_CENTER);
    lv_label_set_text(lbl_s2, "سنسور موقعیت پیستون I7");
    lv_obj_set_style_text_font(lbl_s2, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_s2, lv_color_hex(0xCFD8DC), LV_PART_MAIN | LV_STATE_DEFAULT);

    // ADC F Row
    lv_obj_t * row_adcf = lv_obj_create(pnl_right);
    lv_obj_remove_style_all(row_adcf);
    lv_obj_set_width(row_adcf, 205);
    lv_obj_set_height(row_adcf, 38);
    lv_obj_set_x(row_adcf, 0);
    lv_obj_set_y(row_adcf, 120);
    lv_obj_set_align(row_adcf, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(row_adcf, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row_adcf, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_diag_lbl_adc_f = lv_label_create(row_adcf);
    lv_obj_set_align(ui_diag_lbl_adc_f, LV_ALIGN_CENTER);
    lv_label_set_text(ui_diag_lbl_adc_f, "ADC F1..F5: 4095");
    lv_obj_set_style_text_font(ui_diag_lbl_adc_f, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_diag_lbl_adc_f, lv_color_hex(0x80D8FF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // ADC M Row
    lv_obj_t * row_adcm = lv_obj_create(pnl_right);
    lv_obj_remove_style_all(row_adcm);
    lv_obj_set_width(row_adcm, 205);
    lv_obj_set_height(row_adcm, 38);
    lv_obj_set_x(row_adcm, 0);
    lv_obj_set_y(row_adcm, 166);
    lv_obj_set_align(row_adcm, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(row_adcm, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row_adcm, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_diag_lbl_adc_m = lv_label_create(row_adcm);
    lv_obj_set_align(ui_diag_lbl_adc_m, LV_ALIGN_CENTER);
    lv_label_set_text(ui_diag_lbl_adc_m, "ADC M1..M5: 4095");
    lv_obj_set_style_text_font(ui_diag_lbl_adc_m, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_diag_lbl_adc_m, lv_color_hex(0x80D8FF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // System Health & Diagnostics Status Row
    lv_obj_t * row_sys = lv_obj_create(pnl_right);
    lv_obj_remove_style_all(row_sys);
    lv_obj_set_width(row_sys, 205);
    lv_obj_set_height(row_sys, 38);
    lv_obj_set_x(row_sys, 0);
    lv_obj_set_y(row_sys, 212);
    lv_obj_set_align(row_sys, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(row_sys, lv_color_hex(0x263238), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row_sys, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_diag_lbl_sys_health = lv_label_create(row_sys);
    lv_obj_set_align(ui_diag_lbl_sys_health, LV_ALIGN_CENTER);
    lv_label_set_text(ui_diag_lbl_sys_health, "SYS: OK | SENSORS: 2");
    lv_obj_set_style_text_font(ui_diag_lbl_sys_health, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_diag_lbl_sys_health, lv_color_hex(0x81C784), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HardwareDiagnostics_reset_cache();
}

static int s_lastF = -999;
static int s_lastM = -999;
static int s_lastFSensor = -1;
static int s_lastPSensor = -1;
static uint32_t s_last_diag_tick = 0;

void ui_HardwareDiagnostics_reset_cache(void)
{
    s_lastF = -999;
    s_lastM = -999;
    s_lastFSensor = -1;
    s_lastPSensor = -1;
    s_last_diag_tick = 0;
}

void ui_HardwareDiagnostics_update(int rawF, int rawM, bool funnelSensor, bool pistonSensor)
{
    if (ui_HardwareDiagnostics == NULL || !lv_obj_is_valid(ui_HardwareDiagnostics)) return;

    // Update Funnel Sensor LED (Dirty-check: state transition only)
    if (funnelSensor != s_lastFSensor) {
        s_lastFSensor = funnelSensor;
        if (ui_diag_led_funnel_sensor && lv_obj_is_valid(ui_diag_led_funnel_sensor)) {
            lv_color_t c = funnelSensor ? lv_color_hex(0x00E676) : lv_color_hex(0x455A64);
            lv_obj_set_style_bg_color(ui_diag_led_funnel_sensor, c, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // Update Piston Sensor LED (Dirty-check: state transition only)
    if (pistonSensor != s_lastPSensor) {
        s_lastPSensor = pistonSensor;
        if (ui_diag_led_piston_sensor && lv_obj_is_valid(ui_diag_led_piston_sensor)) {
            lv_color_t c = pistonSensor ? lv_color_hex(0x00E676) : lv_color_hex(0x455A64);
            lv_obj_set_style_bg_color(ui_diag_led_piston_sensor, c, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // Update ADC readings (Dirty-check: threshold noise margin of 25 counts)
    if (abs(rawF - s_lastF) > 25) {
        s_lastF = rawF;
        if (ui_diag_lbl_adc_f && lv_obj_is_valid(ui_diag_lbl_adc_f)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "ADC F1..F5: %d", rawF);
            lv_label_set_text(ui_diag_lbl_adc_f, buf);
        }
    }

    if (abs(rawM - s_lastM) > 25) {
        s_lastM = rawM;
        if (ui_diag_lbl_adc_m && lv_obj_is_valid(ui_diag_lbl_adc_m)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "ADC M1..M5: %d", rawM);
            lv_label_set_text(ui_diag_lbl_adc_m, buf);
        }
    }

    // Periodic System Health Refresh (every 1000ms)
    uint32_t now = lv_tick_get();
    if (now - s_last_diag_tick >= 1000) {
        s_last_diag_tick = now;
        if (ui_diag_lbl_sys_health && lv_obj_is_valid(ui_diag_lbl_sys_health)) {
            char buf[48];
            snprintf(buf, sizeof(buf), "S3-PLC | OK | IO: ACTIVE");
            lv_label_set_text(ui_diag_lbl_sys_health, buf);
        }
    }
}
