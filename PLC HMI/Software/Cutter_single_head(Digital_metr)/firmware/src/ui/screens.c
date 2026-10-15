#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

#define FW_BUILD_TEXT "FW " FW_VERSION_TEXT " | " __DATE__ " " __TIME__

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_main_0() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_0 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // btn0_1
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.btn0_1 = obj;
                    lv_obj_set_pos(obj, 84, 70);
                    lv_obj_set_size(obj, 276, 144);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff006805), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // lablef0_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablef0_1 = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "موقعیت یابی");
                        }
                        {
                            // lablee0_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablee0_1 = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Reference\n     Zero");
                        }
                    }
                }
                {
                    // btn0_2
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.btn0_2 = obj;
                    lv_obj_set_pos(obj, 295, 70);
                    lv_obj_set_size(obj, 65, 55);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffa60d0d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 9, 4);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "X");
                        }
                    }
                }
                {
                    // panel0_1
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.panel0_1 = obj;
                    lv_obj_set_pos(obj, 21, 49);
                    lv_obj_set_size(obj, 393, 221);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // btn0_4
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn0_4 = obj;
                            lv_obj_set_pos(obj, 4, 140);
                            lv_obj_set_size(obj, 100, 50);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // lablef0_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablef0_4 = obj;
                                    lv_obj_set_pos(obj, 20, -3);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "بله");
                                }
                                {
                                    // lablee0_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablee0_4 = obj;
                                    lv_obj_set_pos(obj, 17, 5);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "YES");
                                }
                            }
                        }
                        {
                            // btn0_5
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn0_5 = obj;
                            lv_obj_set_pos(obj, 256, 140);
                            lv_obj_set_size(obj, 100, 50);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // lablef0_5
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablef0_5 = obj;
                                    lv_obj_set_pos(obj, 20, -3);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "انصراف");
                                }
                                {
                                    // lablee0_5
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablee0_5 = obj;
                                    lv_obj_set_pos(obj, 17, 5);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Cancel");
                                }
                            }
                        }
                        {
                            // btn0_3
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn0_3 = obj;
                            lv_obj_set_pos(obj, 317, -6);
                            lv_obj_set_size(obj, 39, 35);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "X");
                                }
                            }
                        }
                        {
                            // lablee0_3
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablee0_3 = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_pos(obj, -6, 37);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Enter without homing?\nCut-size measurement may be\nout of calibration.\nAre you sure?");
                        }
                        {
                            // lablef0_3
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablef0_3 = obj;
                            lv_obj_set_pos(obj, -9, 24);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_base_dir(obj, LV_BASE_DIR_RTL, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "ورود بدون موقعیت‌یابی\nاحتمال خارج‌شدن اندازه‌گیری سایز برش\nاز کالیبره زیاد است.\nآیا اطمینان دارید؟");
                        }
                    }
                }
                {
                    // spinner0_1
                    lv_obj_t *obj = lv_spinner_create(parent_obj);
                    objects.spinner0_1 = obj;
                    lv_obj_set_pos(obj, 187, 107);
                    lv_obj_set_size(obj, 70, 70);
                    lv_spinner_set_anim_params(obj, 1000, 60);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // emg
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.emg = obj;
                    lv_obj_set_pos(obj, 172, 92);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_emg);
                    lv_image_set_scale(obj, 600);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
            }
        }
    }
    
    tick_screen_main_0();
}

void tick_screen_main_0() {
    // مصرف فلگ خطا در Task2 → prcs_ref_homing_result (GUI_Func.h)
}

void create_screen_main_1() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_1 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // arc1_1
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.arc1_1 = obj;
                    lv_obj_set_pos(obj, 12, -68);
                    lv_obj_set_size(obj, 420, 420);
                    lv_arc_set_range(obj, 0, 1000);
                    lv_arc_set_value(obj, 1000);
                    lv_arc_set_bg_start_angle(obj, 0);
                    lv_arc_set_bg_end_angle(obj, 360);
                    lv_obj_set_style_arc_width(obj, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 18, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // lable1_3
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.lable1_3 = obj;
                    lv_obj_set_pos(obj, 102, 220);
                    lv_obj_set_size(obj, 240, 89);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                    lv_obj_set_style_text_font(obj, &ui_font_digit80, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "---");
                }
                {
                    // panel1_2
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.panel1_2 = obj;
                    lv_obj_set_pos(obj, 116, -7);
                    lv_obj_set_size(obj, 213, 80);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // lable1_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.lable1_1 = obj;
                    lv_obj_set_pos(obj, 99, -6);
                    lv_obj_set_size(obj, 247, 71);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                    lv_obj_set_style_text_font(obj, &ui_font_digit80, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff0011d1), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "0");
                }
                {
                    // lable1_2
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.lable1_2 = obj;
                    lv_obj_set_pos(obj, -10, 67);
                    lv_obj_set_size(obj, 463, 155);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                    lv_obj_set_style_text_font(obj, &ui_font_digit160, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffb90000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 35, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "5555");
                }
                {
                    // btn1_2
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.btn1_2 = obj;
                    lv_obj_set_pos(obj, 407, -13);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 50);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_gear, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_gear, NULL);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                    lv_obj_set_style_transform_zoom(obj, 200, LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_translate_y(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_translate_x(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                }
                {
                    // btn1_1
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.btn1_1 = obj;
                    lv_obj_set_pos(obj, -13, -13);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 50);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_plus_silver, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_plus_silver, NULL);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                    lv_obj_set_style_translate_y(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_transform_zoom(obj, 200, LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_translate_x(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj2 = obj;
                    lv_obj_set_pos(obj, 116, 242);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff597393), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "X");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj3 = obj;
                    lv_obj_set_pos(obj, 291, 266);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff597393), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "PCS");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj4 = obj;
                    lv_obj_set_pos(obj, 335, 32);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff597393), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "mm");
                }
                {
                    // label_version_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.label_version_1 = obj;
                    lv_obj_set_pos(obj, 8, 6);
                    lv_obj_set_size(obj, 464, LV_SIZE_CONTENT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2b2b2b), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // anim1_1
                    lv_obj_t *obj = lv_animimg_create(parent_obj);
                    objects.anim1_1 = obj;
                    lv_obj_set_pos(obj, 340, 219);
                    lv_obj_set_size(obj, 38, 38);
                    static const lv_image_dsc_t *images[2] = {
                        &img_newff,
                        &img_newf,
                    };
                    lv_animimg_set_src(obj, (const void **)images, 2);
                    lv_animimg_set_duration(obj, 700);
                    lv_animimg_set_repeat_count(obj, LV_ANIM_REPEAT_INFINITE);
                    lv_animimg_start(obj);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                }
                {
                    // anim1_2
                    lv_obj_t *obj = lv_animimg_create(parent_obj);
                    objects.anim1_2 = obj;
                    lv_obj_set_pos(obj, 71, 219);
                    lv_obj_set_size(obj, 38, 38);
                    static const lv_image_dsc_t *images[2] = {
                        &img_newbb,
                        &img_newb,
                    };
                    lv_animimg_set_src(obj, (const void **)images, 2);
                    lv_animimg_set_duration(obj, 700);
                    lv_animimg_set_repeat_count(obj, LV_ANIM_REPEAT_INFINITE);
                    lv_animimg_start(obj);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                }
                {
                    // panel1_1
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.panel1_1 = obj;
                    lv_obj_set_pos(obj, 67, 32);
                    lv_obj_set_size(obj, 311, 221);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // btn1_4
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn1_4 = obj;
                            lv_obj_set_pos(obj, 4, 140);
                            lv_obj_set_size(obj, 100, 50);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // lablef1_2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablef1_2 = obj;
                                    lv_obj_set_pos(obj, 20, -3);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "بله");
                                }
                                {
                                    // lablee1_2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablee1_2 = obj;
                                    lv_obj_set_pos(obj, 17, 5);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "YES");
                                }
                            }
                        }
                        {
                            // btn1_5
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn1_5 = obj;
                            lv_obj_set_pos(obj, 169, 139);
                            lv_obj_set_size(obj, 100, 50);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // lablef1_3
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablef1_3 = obj;
                                    lv_obj_set_pos(obj, 20, -3);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "خیر");
                                }
                                {
                                    // lablee1_3
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablee1_3 = obj;
                                    lv_obj_set_pos(obj, 17, 5);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "NO");
                                }
                            }
                        }
                        {
                            // btn1_3
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn1_3 = obj;
                            lv_obj_set_pos(obj, 230, -8);
                            lv_obj_set_size(obj, 39, 35);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "X");
                                }
                            }
                        }
                        {
                            // lablee1_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablee1_1 = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_pos(obj, -11, 63);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "Your Selected Cut Program Has Finished\nDo You Wish To Start Next Program?");
                        }
                        {
                            // lablef1_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablef1_1 = obj;
                            lv_obj_set_pos(obj, 22, 40);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "برنامه برش شما به اتمام رسید \nآیا مایل به شروع برنامه بعد هستید؟");
                        }
                    }
                }
                {
                    // panel1_3
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.panel1_3 = obj;
                    lv_obj_set_pos(obj, 67, 32);
                    lv_obj_set_size(obj, 311, 221);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // btn1_6
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn1_6 = obj;
                            lv_obj_set_pos(obj, 88, 140);
                            lv_obj_set_size(obj, 100, 50);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // lablef1_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablef1_4 = obj;
                                    lv_obj_set_pos(obj, 20, -3);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "خب");
                                }
                                {
                                    // lablee1_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.lablee1_4 = obj;
                                    lv_obj_set_pos(obj, 17, 5);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "OK");
                                }
                            }
                        }
                        {
                            // lablee1_6
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablee1_6 = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_pos(obj, -7, 40);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_line_space(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Your List Cut Programs Finished. Either \nRefil the Table Or Operate  In  Another \nMode.");
                        }
                        {
                            // lablef1_6
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablef1_6 = obj;
                            lv_obj_set_size(obj, 258, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "برنامه های برش لیست شما به اتمام\n رسید, جدول را مجددا پر کنید یا در مود دیگری ادامه دهید.");
                        }
                        {
                            // btn1_7 — X close button (top-right of panel1_3)
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn1_7 = obj;
                            lv_obj_set_pos(obj, 250, -13);
                            lv_obj_set_size(obj, 40, 34);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffa60d0d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "X");
                                }
                            }
                        }
                        {
                            // bar1_1 — 5-second auto-close progress bar (bottom of panel1_3)
                            lv_obj_t *obj = lv_bar_create(parent_obj);
                            objects.bar1_1 = obj;
                            lv_obj_set_pos(obj, -12, 170);
                            lv_obj_set_size(obj, 295, 12);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_bar_set_range(obj, 0, 100);
                            lv_bar_set_value(obj, 0, LV_ANIM_OFF);
                            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffa60d0d), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                        }
                        {
                            // lbl_bar1_1 — percentage label above progress bar
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lbl_bar1_1 = obj;
                            lv_obj_set_pos(obj, -12, 153);
                            lv_obj_set_size(obj, 295, LV_SIZE_CONTENT);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "0%");
                        }
                    }
                }
                {
                    // led1_1
                    lv_obj_t *obj = lv_led_create(parent_obj);
                    objects.led1_1 = obj;
                    lv_obj_set_pos(obj, -12, 290);
                    lv_obj_set_size(obj, 5, 5);
                    lv_led_set_color(obj, lv_color_hex(0xffff0000));
                    lv_led_set_brightness(obj, 255);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
    
    tick_screen_main_1();
}

void tick_screen_main_1() {
}

void create_screen_main_2() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_2 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // tabview2_1
                    lv_obj_t *obj = lv_tabview_create(parent_obj);
                    objects.tabview2_1 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 480, 320);
                    lv_tabview_set_tab_bar_position(obj, LV_DIR_RIGHT);
                    lv_tabview_set_tab_bar_size(obj, 60);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *tab_bar = lv_tabview_get_tab_bar(obj);
                        lv_obj_set_style_text_align(tab_bar, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_font(tab_bar, &lv_font_montserrat_20, LV_PART_ITEMS | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_top(tab_bar, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_bottom(tab_bar, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_left(tab_bar, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_right(tab_bar, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    }
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F1");
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj5 = obj;
                                    lv_obj_set_pos(obj, -16, -16);
                                    lv_obj_set_size(obj, 420, 336);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd1e9fb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, 40, -10);
                                            lv_obj_set_size(obj, 280, 50);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_1
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_1 = obj;
                                                    lv_obj_set_pos(obj, 0, -16);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_titr26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "تنظیمات عمومی");
                                                }
                                                {
                                                    // lablee2_1
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_1 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, -6);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_montseraat24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "General Settings");
                                                }
                                            }
                                        }
                                        {
                                            // lang_selector — کشویی (dropdown)
                                            // wrapper بی‌رنگ — roller2_1
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.roller2_1 = obj;
                                            lv_obj_set_pos(obj, 270, 60);
                                            lv_obj_set_size(obj, 120, 132);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                // ===== دکمه کامپکت (child 0) =====
                                                {
                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, 120, 32);
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                                    lv_obj_add_event_cb(obj, action_toggle_dropdown, LV_EVENT_RELEASED, NULL);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_border_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    {
                                                        lv_obj_t *parent_obj = obj;
                                                        // پرچم ایران — کامپکت (child 0) — نمایش پیش‌فرض
                                                        {
                                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                                            lv_obj_set_pos(obj, 5, 9);
                                                            lv_obj_set_size(obj, 22, 14);
                                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            {
                                                                lv_obj_t *parent_obj = obj;
                                                                { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_set_size(o, 22, 5); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0x239f40), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 5); lv_obj_set_size(o, 22, 4); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 9); lv_obj_set_size(o, 22, 5); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xda0000), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                            }
                                                        }
                                                        // پرچم UK — کامپکت (child 1) — پنهان در ابتدا
                                                        {
                                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                                            lv_obj_set_pos(obj, 5, 9);
                                                            lv_obj_set_size(obj, 22, 14);
                                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0x012169), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_color(obj, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                            {
                                                                lv_obj_t *parent_obj = obj;
                                                                static lv_point_precise_t uc_d1[2] = {{0, 0}, {22, 14}};
                                                                static lv_point_precise_t uc_d2[2] = {{22, 0}, {0, 14}};
                                                                { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 4, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, uc_d1, 2); }
                                                                { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 4, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, uc_d2, 2); }
                                                                { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 1, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, uc_d1, 2); }
                                                                { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 1, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, uc_d2, 2); }
                                                                { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 5); lv_obj_set_size(o, 22, 3); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 6); lv_obj_set_size(o, 22, 2); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 9, 0); lv_obj_set_size(o, 3, 14); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 10, 0); lv_obj_set_size(o, 2, 14); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                            }
                                                        }
                                                        // برچسب زبان فعلی (child 2)
                                                        {
                                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                            lv_obj_align(obj, LV_ALIGN_LEFT_MID, 30, 0);
                                                            lv_obj_set_style_text_color(obj, lv_color_hex(0x1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_label_set_text(obj, "فارسی");
                                                        }
                                                        // فلش پایین (child 3)
                                                        {
                                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                            lv_obj_align(obj, LV_ALIGN_RIGHT_MID, -6, 0);
                                                            lv_obj_set_style_text_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_label_set_text(obj, LV_SYMBOL_DOWN);
                                                        }
                                                    }
                                                }
                                                // ===== پنل dropdown (child 1) — پنهان در ابتدا =====
                                                {
                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 34);
                                                    lv_obj_set_size(obj, 120, 92);
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    {
                                                        lv_obj_t *parent_obj = obj;
                                                        // ===== ردیف فارسی (child 0) =====
                                                        {
                                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                                            lv_obj_set_pos(obj, 0, 0);
                                                            lv_obj_set_size(obj, 120, 44);
                                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                                            lv_obj_add_event_cb(obj, action_lang_farsi, LV_EVENT_RELEASED, NULL);
                                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xeef8fd), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            {
                                                                lv_obj_t *parent_obj = obj;
                                                                // پرچم ایران (child 0)
                                                                {
                                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                                    lv_obj_set_pos(obj, 6, 15);
                                                                    lv_obj_set_size(obj, 22, 14);
                                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    {
                                                                        lv_obj_t *parent_obj = obj;
                                                                        { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_set_size(o, 22, 5); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0x239f40), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                        { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 5); lv_obj_set_size(o, 22, 4); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                        { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 9); lv_obj_set_size(o, 22, 5); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xda0000), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                    }
                                                                }
                                                                // برچسب "فارسی" (child 1)
                                                                {
                                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                                    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 33, 0);
                                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0x0077bb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_label_set_text(obj, "فارسی");
                                                                }
                                                                // radio فارسی (child 2)
                                                                {
                                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                                    lv_obj_set_pos(obj, 96, 14);
                                                                    lv_obj_set_size(obj, 16, 16);
                                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_border_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    {
                                                                        lv_obj_t *parent_obj = obj;
                                                                        lv_obj_t *dot = lv_obj_create(parent_obj);
                                                                        lv_obj_set_size(dot, 8, 8);
                                                                        lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
                                                                        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                                        lv_obj_set_style_radius(dot, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_bg_color(dot, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        // ===== خط جداکننده (child 1) =====
                                                        {
                                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                                            lv_obj_set_pos(obj, 0, 44);
                                                            lv_obj_set_size(obj, 120, 4);
                                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                        }
                                                        // ===== ردیف انگلیسی (child 2) =====
                                                        {
                                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                                            lv_obj_set_pos(obj, 0, 48);
                                                            lv_obj_set_size(obj, 120, 44);
                                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                                            lv_obj_add_event_cb(obj, action_lang_english, LV_EVENT_RELEASED, NULL);
                                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            {
                                                                lv_obj_t *parent_obj = obj;
                                                                // پرچم UK (child 0)
                                                                {
                                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                                    lv_obj_set_pos(obj, 6, 15);
                                                                    lv_obj_set_size(obj, 22, 14);
                                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x012169), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_border_color(obj, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    {
                                                                        lv_obj_t *parent_obj = obj;
                                                                        static lv_point_precise_t ukp_d1[2] = {{0, 0}, {22, 14}};
                                                                        static lv_point_precise_t ukp_d2[2] = {{22, 0}, {0, 14}};
                                                                        { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 4, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, ukp_d1, 2); }
                                                                        { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 4, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, ukp_d2, 2); }
                                                                        { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 1, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, ukp_d1, 2); }
                                                                        { lv_obj_t *o = lv_line_create(parent_obj); lv_obj_set_pos(o, 0, 0); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_line_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_width(o, 1, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_line_rounded(o, false, LV_PART_MAIN | LV_STATE_DEFAULT); lv_line_set_points(o, ukp_d2, 2); }
                                                                        { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 5); lv_obj_set_size(o, 22, 3); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                        { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 0, 6); lv_obj_set_size(o, 22, 2); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                        { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 9, 0); lv_obj_set_size(o, 3, 14); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                        { lv_obj_t *o = lv_obj_create(parent_obj); lv_obj_set_pos(o, 10, 0); lv_obj_set_size(o, 2, 14); lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); lv_obj_set_style_bg_color(o, lv_color_hex(0xc8102e), LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_border_width(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_radius(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT); }
                                                                    }
                                                                }
                                                                // برچسب "English" (child 1)
                                                                {
                                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                                    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 33, 0);
                                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_label_set_text(obj, "English");
                                                                }
                                                                // radio انگلیسی (child 2)
                                                                {
                                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                                    lv_obj_set_pos(obj, 96, 14);
                                                                    lv_obj_set_size(obj, 16, 16);
                                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_border_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                    {
                                                                        lv_obj_t *parent_obj = obj;
                                                                        lv_obj_t *dot = lv_obj_create(parent_obj);
                                                                        lv_obj_set_size(dot, 8, 8);
                                                                        lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
                                                                        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                                        lv_obj_set_style_radius(dot, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_bg_color(dot, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                                        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        {
                                            // img2_2
                                            lv_obj_t *obj = lv_image_create(parent_obj);
                                            objects.img2_2 = obj;
                                            lv_obj_set_pos(obj, 200, 67);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_image_set_src(obj, &img_unmute);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                        }
                                        {
                                            // img2_1
                                            lv_obj_t *obj = lv_image_create(parent_obj);
                                            objects.img2_1 = obj;
                                            lv_obj_set_pos(obj, 200, 67);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_image_set_src(obj, &img_mute);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN|LV_OBJ_FLAG_CLICKABLE);
                                        }
                                        {
                                            // دکمه تست چاپ — سفید با بوردر آبی
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, 90, 220);
                                            lv_obj_set_size(obj, 240, 54);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                            lv_obj_add_event_cb(obj, action_printertest, LV_EVENT_RELEASED, (void *)0);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                // خط جداکننده عمودی آبی
                                                {
                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                    lv_obj_set_pos(obj, 62, 6);
                                                    lv_obj_set_size(obj, 2, 42);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x00aaee), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_radius(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                // بخش راست با متن
                                                {
                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                    lv_obj_set_pos(obj, 64, 0);
                                                    lv_obj_set_size(obj, 176, 54);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xeef8fd), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    {
                                                        lv_obj_t *parent_obj = obj;
                                                        {
                                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                                            objects.lablef_print = obj;
                                                            lv_obj_set_size(obj, 176, LV_SIZE_CONTENT);
                                                            lv_obj_set_align(obj, LV_ALIGN_CENTER);
                                                            lv_obj_set_style_text_color(obj, lv_color_hex(0x1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_text_font(obj, &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_label_set_text(obj, "تست چاپ");
                                                        }
                                                        {
                                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                                            objects.lablee_print = obj;
                                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                            lv_obj_set_size(obj, 176, LV_SIZE_CONTENT);
                                                            lv_obj_set_align(obj, LV_ALIGN_CENTER);
                                                            lv_obj_set_style_text_color(obj, lv_color_hex(0x1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                            lv_label_set_text(obj, "Test Print");
                                                        }
                                                    }
                                                }
                                                // آیکون پرینتر — وسط بخش چپ سفید (62px)
                                                {
                                                    lv_obj_t *obj = lv_image_create(parent_obj);
                                                    lv_obj_set_pos(obj, 6, 4);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_image_set_src(obj, &img_printer);
                                                }
                                            }
                                        }
                                    }
                                }
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(obj, 3, 55);
                                    lv_obj_set_size(obj, 187, 146);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                    lv_obj_set_style_radius(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // normally_closed
                                            lv_obj_t *obj = lv_image_create(parent_obj);
                                            objects.normally_closed = obj;
                                            lv_obj_set_pos(obj, -9, 74);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_image_set_src(obj, &img_normally_closed);
                                        }
                                        {
                                            // st_bt_co
                                            lv_obj_t *obj = lv_image_create(parent_obj);
                                            objects.st_bt_co = obj;
                                            lv_obj_set_pos(obj, 25, -24);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_image_set_src(obj, &img_emg);
                                            lv_image_set_scale(obj, 200);
                                            lv_obj_add_event_cb(obj, action_stopstate, LV_EVENT_RELEASED, (void *)0);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                        }
                                        {
                                            // normally_open
                                            lv_obj_t *obj = lv_image_create(parent_obj);
                                            objects.normally_open = obj;
                                            lv_obj_set_pos(obj, -10, 59);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_image_set_src(obj, &img_normally_open);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                        }
                                    }
                                }
                                {
                                    // panel_no
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel_no = obj;
                                    lv_obj_set_pos(obj, 68, 9);
                                    lv_obj_set_size(obj, 285, 135);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
                                    lv_obj_set_style_radius(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, 8, 3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "کلید استاپ\n .نرمالی اپن خواهد بود");
                                        }
                                    }
                                }
                                {
                                    // panel_nc
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel_nc = obj;
                                    lv_obj_set_pos(obj, 68, 9);
                                    lv_obj_set_size(obj, 285, 135);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
                                    lv_obj_set_style_radius(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, 8, 3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "کلید استاپ\n .نرمالی کلوز خواهد بود");
                                        }
                                    }
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F2");
                            objects.obj6 = obj;
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe70e0e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj7 = obj;
                                    lv_obj_set_pos(obj, -16, -16);
                                    lv_obj_set_size(obj, 420, 336);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd1e9fb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, 40, -10);
                                            lv_obj_set_size(obj, 280, 50);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_2
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_2 = obj;
                                                    lv_obj_set_pos(obj, 0, -16);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_titr26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "تنظیمات موتور");
                                                }
                                                {
                                                    // lablee2_2
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_2 = obj;
                                                    lv_obj_set_pos(obj, 0, -6);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_montseraat24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Motor Settings");
                                                }
                                            }
                                        }
                                        {
                                            // slider2_1
                                            lv_obj_t *obj = lv_slider_create(parent_obj);
                                            objects.slider2_1 = obj;
                                            lv_obj_set_pos(obj, 12, 80);
                                            lv_obj_set_size(obj, 360, 20);
                                            lv_slider_set_range(obj, 0, 1000);
                                            lv_slider_set_value(obj, 250, LV_ANIM_OFF);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                        }
                                        {
                                            // slider2_2
                                            lv_obj_t *obj = lv_slider_create(parent_obj);
                                            objects.slider2_2 = obj;
                                            lv_obj_set_pos(obj, 12, 147);
                                            lv_obj_set_size(obj, 360, 20);
                                            lv_slider_set_range(obj, 0, 1000);
                                            lv_slider_set_value(obj, 250, LV_ANIM_OFF);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                        }
                                        {
                                            // lbl_spd2_1
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lbl_spd2_1 = obj;
                                            lv_obj_set_pos(obj, 155, -80);
                                            lv_obj_set_size(obj, 42, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "25%");
                                        }
                                        {
                                            // lbl_spd2_2
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lbl_spd2_2 = obj;
                                            lv_obj_set_pos(obj, 155, -13);
                                            lv_obj_set_size(obj, 42, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "25%");
                                        }
                                        {
                                            // lablef2_6
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lablef2_6 = obj;
                                            lv_obj_set_pos(obj, 5, -80);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "سرعت");
                                        }
                                        {
                                            // lablef2_7
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lablef2_7 = obj;
                                            lv_obj_set_pos(obj, 8, -13);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "سرعت راه اندازی");
                                        }
                                        {
                                            // lablee2_6
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lablee2_6 = obj;
                                            lv_obj_set_pos(obj, 8, -80);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "Speed");
                                        }
                                        {
                                            // lablee2_7
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lablee2_7 = obj;
                                            lv_obj_set_pos(obj, 5, -13);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "Initial Speed");
                                        }
                                    }
                                }
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj8 = obj;
                                    lv_obj_set_pos(obj, -5, 186);
                                    lv_obj_set_size(obj, 231, 105);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff989898), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.obj9 = obj;
                                            lv_obj_set_pos(obj, 87, -10);
                                            lv_obj_set_size(obj, 120, 42);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lable2_2
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lable2_2 = obj;
                                                    lv_obj_set_pos(obj, -23, -18);
                                                    lv_obj_set_size(obj, 91, 40);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "5555");
                                                }
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 71, 5);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "MM");
                                                }
                                            }
                                        }
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.obj10 = obj;
                                            lv_obj_set_pos(obj, 87, 42);
                                            lv_obj_set_size(obj, 120, 42);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lable2_3
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lable2_3 = obj;
                                                    lv_obj_set_pos(obj, -23, -18);
                                                    lv_obj_set_size(obj, 91, 40);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "5555");
                                                }
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 71, 5);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "MM");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_3
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_3 = obj;
                                            lv_obj_set_pos(obj, -10, -10);
                                            lv_obj_set_size(obj, 92, 42);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_8
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_8 = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "طول نمونه");
                                                }
                                                {
                                                    // lablee2_8
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_8 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Sample\nLength");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_4
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_4 = obj;
                                            lv_obj_set_pos(obj, -10, 42);
                                            lv_obj_set_size(obj, 92, 42);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_9
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_9 = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "طول اولیه");
                                                }
                                                {
                                                    // lablee2_9
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_9 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, " Initial\nLength");
                                                }
                                            }
                                        }
                                    }
                                }
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj11 = obj;
                                    lv_obj_set_pos(obj, 238, 186);
                                    lv_obj_set_size(obj, 151, 105);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff989898), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.obj12 = obj;
                                            lv_obj_set_pos(obj, 1, 38);
                                            lv_obj_set_size(obj, 120, 42);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lable2_5
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lable2_5 = obj;
                                                    lv_obj_set_pos(obj, -18, -17);
                                                    lv_obj_set_size(obj, 91, 40);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff212121), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "5555");
                                                }
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 76, 6);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "MM");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_8
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_8 = obj;
                                            lv_obj_set_pos(obj, 14, -10);
                                            lv_obj_set_size(obj, 92, 42);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_25
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_25 = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_line_space(obj, -20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "طول نهایی\nمحور");
                                                }
                                                {
                                                    // lablee2_25
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_25 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_line_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Max Axis\nLength");
                                                }
                                            }
                                        }
                                    }
                                }
                                {
                                    // panel2_1
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel2_1 = obj;
                                    lv_obj_set_pos(obj, 39, 32);
                                    lv_obj_set_size(obj, 311, 221);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // btn2_5
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_5 = obj;
                                            lv_obj_set_pos(obj, 4, 140);
                                            lv_obj_set_size(obj, 100, 50);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_10
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_10 = obj;
                                                    lv_obj_set_pos(obj, 20, -3);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "بله");
                                                }
                                                {
                                                    // lablee2_10
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_10 = obj;
                                                    lv_obj_set_pos(obj, 17, 5);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "YES");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_6
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_6 = obj;
                                            lv_obj_set_pos(obj, 169, 139);
                                            lv_obj_set_size(obj, 100, 50);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_11
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_11 = obj;
                                                    lv_obj_set_pos(obj, 20, -3);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "خیر");
                                                }
                                                {
                                                    // lablee2_11
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_11 = obj;
                                                    lv_obj_set_pos(obj, 17, 5);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "NO");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_7
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_7 = obj;
                                            lv_obj_set_pos(obj, 230, -8);
                                            lv_obj_set_size(obj, 39, 35);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "X");
                                                }
                                            }
                                        }
                                        {
                                            // lablee2_12
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lablee2_12 = obj;
                                            lv_obj_set_pos(obj, 15, 49);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                            lv_label_set_text(obj, "Calibration Process Will \nDelete Axis Parameters\nAre You Sure?");
                                        }
                                        {
                                            // lablef2_12
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lablef2_12 = obj;
                                            lv_obj_set_pos(obj, 20, 25);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "شروع کالیبراسیون پارامتر\n های فعلی محور را پاک میکند\nآیا اطمینان دارید؟");
                                        }
                                    }
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F3");
                            objects.obj13 = obj;
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe70e0e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj14 = obj;
                                    lv_obj_set_pos(obj, -16, -16);
                                    lv_obj_set_size(obj, 420, 336);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd1e9fb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, 40, -10);
                                            lv_obj_set_size(obj, 280, 50);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_3
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_3 = obj;
                                                    lv_obj_set_pos(obj, 0, -16);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_titr26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "تست دستی المان ها");
                                                }
                                                {
                                                    // lablee2_3
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_3 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, -6);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_montseraat24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Manual Test");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_3_2
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_3_2 = obj;
                                            lv_obj_set_pos(obj, 293, 56);
                                            lv_obj_set_size(obj, 98, 70);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff07ff00), LV_PART_MAIN | LV_STATE_CHECKED);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_13
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_13 = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "جک\nنگهدارنده");
                                                }
                                                {
                                                    // lablee2_13
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_13 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Holder\nJack");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_3_4
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_3_4 = obj;
                                            lv_obj_set_pos(obj, 293, 138);
                                            lv_obj_set_size(obj, 98, 70);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff07ff00), LV_PART_MAIN | LV_STATE_CHECKED);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_15
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_15 = obj;
                                                    lv_obj_set_pos(obj, 0, 1);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "جک\nمحافظ");
                                                }
                                                {
                                                    // lablee2_15
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_15 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 1);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Guard\nJack");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_3_6
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_3_6 = obj;
                                            lv_obj_set_pos(obj, 293, 219);
                                            lv_obj_set_size(obj, 98, 70);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff07ff00), LV_PART_MAIN | LV_STATE_CHECKED);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_17
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_17 = obj;
                                                    lv_obj_set_pos(obj, 0, 1);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "چراغ");
                                                }
                                                {
                                                    // lablee2_17
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_17 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 1);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Lamp");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_3_1
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_3_1 = obj;
                                            lv_obj_set_pos(obj, 183, 56);
                                            lv_obj_set_size(obj, 98, 70);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff07ff00), LV_PART_MAIN | LV_STATE_CHECKED);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_14
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_14 = obj;
                                                    lv_obj_set_pos(obj, 0, 1);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "اسپری");
                                                }
                                                {
                                                    // lablee2_14
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_14 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 1);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Spray");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_3_3
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_3_3 = obj;
                                            lv_obj_set_pos(obj, 183, 138);
                                            lv_obj_set_size(obj, 98, 70);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff07ff00), LV_PART_MAIN | LV_STATE_CHECKED);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_16
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_16 = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "تیغه");
                                                }
                                                {
                                                    // lablee2_16
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_16 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Saw");
                                                }
                                            }
                                        }
                                        {
                                            // btn2_3_5
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.btn2_3_5 = obj;
                                            lv_obj_set_pos(obj, 184, 219);
                                            lv_obj_set_size(obj, 97, 70);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff07ff00), LV_PART_MAIN | LV_STATE_CHECKED);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_18
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_18 = obj;
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "جک\nتیغه");
                                                }
                                                {
                                                    // lablee2_18
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_18 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Lift Saw");
                                                }
                                            }
                                        }
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, -10, 56);
                                            lv_obj_set_size(obj, 183, 247);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_29 - Micro Switch (ROW 1 - TOP)
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_29 = obj;
                                                    lv_obj_set_pos(obj, 22, -7);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "سوییچ درب محافظ");
                                                }
                                                {
                                                    // lablee2_29 - Micro Switch
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_29 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 19, -2);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Door Guard Sw");
                                                }
                                                {
                                                    // led2_3_8 - Micro Switch
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_8 = obj;
                                                    lv_obj_set_pos(obj, -10, -5);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 0);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // lablef2_19 - Ref Sensor (ROW 2)
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_19 = obj;
                                                    lv_obj_set_pos(obj, 22, 31);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "سنسور اول خط");
                                                }
                                                {
                                                    // lablee2_19
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_19 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 19, 50);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "First Sensor");
                                                }
                                                {
                                                    // led2_3_1
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_1 = obj;
                                                    lv_obj_set_pos(obj, -10, 33);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 0);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // lablef2_20 - Up Limit (ROW 3)
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_20 = obj;
                                                    lv_obj_set_pos(obj, 23, 69);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "سنسور آخر خط");
                                                }
                                                {
                                                    // lablee2_20
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_20 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 29, 88);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Last Sensor");
                                                }
                                                {
                                                    // led2_3_2
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_2 = obj;
                                                    lv_obj_set_pos(obj, -10, 71);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 255);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // lablef2_22 - Stop (ROW 4)
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_22 = obj;
                                                    lv_obj_set_pos(obj, 23, 107);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "کلید استپ");
                                                }
                                                {
                                                    // lablee2_22
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_22 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 23, 112);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Stop btn");
                                                }
                                                {
                                                    // led2_3_4
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_4 = obj;
                                                    lv_obj_set_pos(obj, -9, 109);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 255);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // lablef2_21 - Start 1 (ROW 5 left)
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_21 = obj;
                                                    lv_obj_set_pos(obj, 15, 145);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "استارت1");
                                                }
                                                {
                                                    // lablee2_21
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_21 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 24, 151);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Start 1");
                                                }
                                                {
                                                    // led2_3_3 - Start 1
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_3 = obj;
                                                    lv_obj_set_pos(obj, -10, 147);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 0);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // lablef2_28 - Start 2 (ROW 5 right)
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_28 = obj;
                                                    lv_obj_set_pos(obj, 76, 145);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "استارت٢");
                                                }
                                                {
                                                    // lablee2_28
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_28 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 76, 151);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Start 2");
                                                }
                                                {
                                                    // led2_3_7 - Start 2
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_7 = obj;
                                                    lv_obj_set_pos(obj, 136, 147);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 0);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // lablef2_23 - Right joystick (ROW 6)
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_23 = obj;
                                                    lv_obj_set_pos(obj, 92, 183);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "راست");
                                                }
                                                {
                                                    // lablef2_26 - Left joystick
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_26 = obj;
                                                    lv_obj_set_pos(obj, 17, 184);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "چپ");
                                                }
                                                {
                                                    // lablee2_23
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_23 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 74, 188);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Right");
                                                }
                                                {
                                                    // lablee2_26
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_26 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 14, 189);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Left");
                                                }
                                                {
                                                    // led2_3_6
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_6 = obj;
                                                    lv_obj_set_pos(obj, 136, 185);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 255);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // led2_3_5
                                                    lv_obj_t *obj = lv_led_create(parent_obj);
                                                    objects.led2_3_5 = obj;
                                                    lv_obj_set_pos(obj, -10, 185);
                                                    lv_obj_set_size(obj, 20, 20);
                                                    lv_led_set_color(obj, lv_color_hex(0xff07ff00));
                                                    lv_led_set_brightness(obj, 255);
                                                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F4");
                            objects.obj15 = obj;
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe70e0e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj16 = obj;
                                    lv_obj_set_pos(obj, -16, -16);
                                    lv_obj_set_size(obj, 420, 336);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd1e9fb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, 40, -10);
                                            lv_obj_set_size(obj, 280, 50);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_4
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_4 = obj;
                                                    lv_obj_set_pos(obj, 0, -16);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_titr26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "تنظیمات مدیریت");
                                                }
                                                {
                                                    // lablee2_4
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_4 = obj;
                                                    lv_obj_set_pos(obj, 0, -6);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_montseraat24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Admin Settings");
                                                }
                                            }
                                        }
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.obj17 = obj;
                                            lv_obj_set_pos(obj, 254, 46);
                                            lv_obj_set_size(obj, 140, 147);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff989898), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // roller2_2
                                                    lv_obj_t *obj = lv_roller_create(parent_obj);
                                                    objects.roller2_2 = obj;
                                                    lv_obj_set_pos(obj, -3, 19);
                                                    lv_obj_set_size(obj, 111, 100);
                                                    lv_roller_set_options(obj, "1    s\n2    s\n3    s\n4    s\n5    s\n6    s\n7    s\n8    s\n9    s\n10   s\n11   s\n12   s\n13   s\n14   s\n15   s", LV_ROLLER_MODE_NORMAL);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                }
                                                {
                                                    // lablee2_24
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_24 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 2, -7);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_label_set_text(obj, "Cut Timer");
                                                }
                                                {
                                                    // lablef2_24
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_24 = obj;
                                                    lv_obj_set_pos(obj, 6, -21);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "تایمر برش");
                                                }
                                            }
                                        }
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.obj18 = obj;
                                            lv_obj_set_pos(obj, 100, 46);
                                            lv_obj_set_size(obj, 140, 150);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff989898), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablee2_27
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_27 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 2, -7);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_label_set_text(obj, "Base Dir");
                                                }
                                                {
                                                    // lablef2_27
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_27 = obj;
                                                    lv_obj_set_pos(obj, 6, -21);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_naz30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "جهت مبنا");
                                                }
                                                {
                                                    // sw2_1
                                                    lv_obj_t *obj = lv_switch_create(parent_obj);
                                                    objects.sw2_1 = obj;
                                                    lv_obj_set_pos(obj, 7, 23);
                                                    lv_obj_set_size(obj, 90, 34);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff2424), LV_PART_KNOB | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_CHECKED);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_INDICATOR | LV_STATE_CHECKED);
                                                }
                                                {
                                                    // wrapper — centered horizontally, y=78
                                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                                    lv_obj_remove_style_all(obj);
                                                    lv_obj_set_size(obj, 120, 40);
                                                    lv_obj_set_style_pad_all(obj, 0, 0);
                                                    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 4);
                                                    lv_obj_set_style_base_dir(obj, LV_BASE_DIR_LTR, 0);
                                                    {
                                                        lv_obj_t *parent_obj = obj;
                                                        {
                                                            // btn_dir_rast — blue button
                                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                                            objects.btn_dir_rast = obj;
                                                            lv_obj_remove_style_all(obj);
                                                            lv_obj_set_pos(obj, 60, 0);
                                                            lv_obj_set_size(obj, 60, 40);
                                                            lv_obj_set_style_radius(obj, 0, 0);
                                                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
                                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2196f3), 0);
                                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1565C0), LV_PART_MAIN | LV_STATE_PRESSED);
                                                            {
                                                                lv_obj_t *parent_obj = obj;
                                                                {
                                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                                    objects.lablef_dir_rast = obj;
                                                                    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
                                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, 0);
                                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), 0);
                                                                    lv_label_set_text(obj, "راست");
                                                                }
                                                                {
                                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                                    objects.lablee_dir_rast = obj;
                                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                                    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
                                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, 0);
                                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), 0);
                                                                    lv_label_set_text(obj, "Right");
                                                                }
                                                            }
                                                        }
                                                        {
                                                            // btn_dir_chap — red button
                                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                                            objects.btn_dir_chap = obj;
                                                            lv_obj_remove_style_all(obj);
                                                            lv_obj_set_pos(obj, 0, 0);
                                                            lv_obj_set_size(obj, 60, 40);
                                                            lv_obj_set_style_radius(obj, 0, 0);
                                                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
                                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffE24B4A), 0);
                                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff991F1F), LV_PART_MAIN | LV_STATE_PRESSED);
                                                            {
                                                                lv_obj_t *parent_obj = obj;
                                                                {
                                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                                    objects.lablef_dir_chap = obj;
                                                                    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
                                                                    lv_obj_set_style_text_font(obj, &ui_font_naz24, 0);
                                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), 0);
                                                                    lv_label_set_text(obj, "چپ");
                                                                }
                                                                {
                                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                                    objects.lablee_dir_chap = obj;
                                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                                    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
                                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, 0);
                                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), 0);
                                                                    lv_label_set_text(obj, "Left");
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "F5");
                            objects.obj19 = obj;
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffe70e0e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.obj20 = obj;
                                    lv_obj_set_pos(obj, -16, -16);
                                    lv_obj_set_size(obj, 420, 336);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd1e9fb), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, 40, -10);
                                            lv_obj_set_size(obj, 280, 50);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // lablef2_5
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablef2_5 = obj;
                                                    lv_obj_set_pos(obj, 0, -16);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_titr26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "تنظیمات توسعه دهنده");
                                                }
                                                {
                                                    // lablee2_5
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.lablee2_5 = obj;
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                                                    lv_obj_set_pos(obj, 0, -6);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_montseraat24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Developer Settings");
                                                }
                                            }
                                        }
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            lv_obj_set_pos(obj, 84, 64);
                                            lv_obj_set_size(obj, 231, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                            lv_obj_set_style_radius(obj, 9, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 4, -14);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Software Version :\n " BOARD_INFO_LINE1 "\n" BOARD_INFO_LINE2);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                {
                    // btn2_1
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.btn2_1 = obj;
                    lv_obj_set_pos(obj, 3, 8);
                    lv_obj_set_size(obj, 50, 50);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1976d2), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0d47a1), LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                    lv_obj_set_style_translate_y(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_translate_x(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, LV_SYMBOL_HOME);
                        }
                    }
                }
                {
                    // btn_save
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.btn_save = obj;
                    lv_obj_set_pos(obj, 348, 13);
                    lv_obj_set_size(obj, 65, 38);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff007c89), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff005f69), LV_PART_MAIN | LV_STATE_PRESSED);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff007c89), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_pad(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_opa(obj, 120, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff007c89), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablef_save = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "ذخیره");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablee_save = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Save");
                        }
                    }
                }
            }
        }
    }

    tick_screen_main_2();
}

void tick_screen_main_2() {
}

void create_screen_main_3() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_3 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_scroll_dir(obj, LV_DIR_HOR);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj21 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj22 = obj;
                    lv_obj_set_pos(obj, -13, -13);
                    lv_obj_set_size(obj, 471, 311);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                    lv_obj_set_scroll_dir(obj, LV_DIR_VER);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // btn3_1
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn3_1 = obj;
                            lv_obj_set_pos(obj, -13, -13);
                            lv_obj_set_size(obj, 50, 50);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1976d2), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0d47a1), LV_PART_MAIN | LV_STATE_PRESSED);
                            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                            lv_obj_set_style_translate_y(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                            lv_obj_set_style_translate_x(obj, 2, LV_PART_MAIN | LV_STATE_PRESSED);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, LV_SYMBOL_HOME);
                                }
                            }
                        }
                        {
                            // lablef3_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablef3_1 = obj;
                            lv_obj_set_pos(obj, 107, 33);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "متراژ");
                        }
                        {
                            // lablee3_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablee3_1 = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_pos(obj, 93, 41);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Length");
                        }
                        {
                            // lablef3_2
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablef3_2 = obj;
                            lv_obj_set_pos(obj, 247, 35);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_naz24, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "تعداد");
                        }
                        {
                            // lablee3_2
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lablee3_2 = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_pos(obj, 226, 41);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Quantity");
                        }
                        {
                            // panel3_1
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.panel3_1 = obj;
                            lv_obj_set_pos(obj, -9, 66);
                            lv_obj_set_size(obj, 455, 233);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // panel3_1_1
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_1 = obj;
                                    lv_obj_set_pos(obj, -7, -2);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_1
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_1 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_1
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_1 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_1
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_1 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj23 = obj;
                                                    lv_obj_set_pos(obj, -17, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "1");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_1
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_1 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj24 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_2
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_2 = obj;
                                    lv_obj_set_pos(obj, -7, 71);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_2
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_2 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_2
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_2 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_2
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_2 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj25 = obj;
                                                    lv_obj_set_pos(obj, -11, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "2");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_2
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_2 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj26 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_3
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_3 = obj;
                                    lv_obj_set_pos(obj, -7, 144);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_3
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_3 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_3
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_3 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_3
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_3 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj27 = obj;
                                                    lv_obj_set_pos(obj, -12, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "3");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_3
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_3 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj28 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_4
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_4 = obj;
                                    lv_obj_set_pos(obj, -7, 217);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_4
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_4 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_4
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_4 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_4
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_4 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj29 = obj;
                                                    lv_obj_set_pos(obj, -11, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "4");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_4
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_4 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj30 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_5
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_5 = obj;
                                    lv_obj_set_pos(obj, -7, 290);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_5
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_5 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_5
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_5 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_5
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_5 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj31 = obj;
                                                    lv_obj_set_pos(obj, -8, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "5");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_5
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_5 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj32 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_6
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_6 = obj;
                                    lv_obj_set_pos(obj, -7, 362);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_6
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_6 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_6
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_6 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_6
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_6 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj33 = obj;
                                                    lv_obj_set_pos(obj, -8, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "6");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_6
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_6 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj34 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_7
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_7 = obj;
                                    lv_obj_set_pos(obj, -7, 435);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_7
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_7 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_7
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_7 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_7
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_7 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj35 = obj;
                                                    lv_obj_set_pos(obj, -13, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "7");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_7
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_7 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj36 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_8
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_8 = obj;
                                    lv_obj_set_pos(obj, -7, 508);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_8
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_8 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_8
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_8 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_8
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_8 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj37 = obj;
                                                    lv_obj_set_pos(obj, -12, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "8");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_8
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_8 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj38 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_9
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_9 = obj;
                                    lv_obj_set_pos(obj, -7, 581);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_9
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_9 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_9
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_9 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_9
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_9 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj39 = obj;
                                                    lv_obj_set_pos(obj, -10, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "9");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_9
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_9 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj40 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                                {
                                    // panel3_1_10
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.panel3_1_10 = obj;
                                    lv_obj_set_pos(obj, -7, 654);
                                    lv_obj_set_size(obj, 374, 57);
                                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // lable3_1_10
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_1_10 = obj;
                                            lv_obj_set_pos(obj, 59, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "5555");
                                        }
                                        {
                                            // lable3_2_10
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.lable3_2_10 = obj;
                                            lv_obj_set_pos(obj, 206, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "555");
                                        }
                                        {
                                            // panel3_2_10
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.panel3_2_10 = obj;
                                            lv_obj_set_pos(obj, -17, -17);
                                            lv_obj_set_size(obj, 48, 56);
                                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00cf37), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.obj41 = obj;
                                                    lv_obj_set_pos(obj, -26, -10);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_font(obj, &ui_font_digit40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "10");
                                                }
                                            }
                                        }
                                        {
                                            // btn3_1_10
                                            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                                            objects.btn3_1_10 = obj;
                                            lv_obj_set_pos(obj, 300, -9);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_edit_pen, NULL);
                                            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_edit_pen, NULL);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj42 = obj;
                                            lv_obj_set_pos(obj, 171, -3);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "X");
                                        }
                                    }
                                }
                            }
                        }
                        {
                            // btn3_2
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.btn3_2 = obj;
                            lv_obj_set_pos(obj, 333, -7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_add);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                        }
                        {
                            // btn3_3
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.btn3_3 = obj;
                            lv_obj_set_pos(obj, 406, -7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, &img_remove);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                        }
                    }
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 63, 8);
            lv_obj_set_size(obj, 264, 50);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // lablef3_0
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.lablef3_0 = obj;
                    lv_obj_set_pos(obj, 65, -16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &ui_font_titr26, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "لیست برش");
                }
                {
                    // lablee3_0
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.lablee3_0 = obj;
                    lv_obj_set_pos(obj, 66, -6);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_text_font(obj, &ui_font_montseraat24, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Cut List");
                }
            }
        }
    }
    
    tick_screen_main_3();
}

void tick_screen_main_3() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main_0,
    tick_screen_main_1,
    tick_screen_main_2,
    tick_screen_main_3,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main_0();
    create_screen_main_1();
#if defined(MC_PRECREATE_HEAVY_SCREENS) && MC_PRECREATE_HEAVY_SCREENS
    create_screen_main_2();
    create_screen_main_3();
#endif
}
