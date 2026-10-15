#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: EnS
//

void init_style_en_s_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[eez_flow_get_selected_theme_index()][7]));
};

lv_style_t *get_style_en_s_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_en_s_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_en_s(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_en_s_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_en_s(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_en_s_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: Dns
//

void init_style_dns_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[eez_flow_get_selected_theme_index()][6]));
};

lv_style_t *get_style_dns_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_dns_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_dns(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_dns_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_dns(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_dns_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: disable_Mode_btn
//

void init_style_disable_mode_btn_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xff1d3f49));
    lv_style_set_radius(style, 0);
};

lv_style_t *get_style_disable_mode_btn_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_disable_mode_btn_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_disable_mode_btn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_disable_mode_btn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    // فیدبکِ لمس: موقع فشار دکمه کمی پایین و کم‌رنگ می‌شود تا کاربر بفهمد کجا را زد (حتی بدون بوق)
    lv_obj_set_style_translate_y(obj, 3, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(obj, 170, LV_PART_MAIN | LV_STATE_PRESSED);
};

void remove_style_disable_mode_btn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_disable_mode_btn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: enable_Mode_btn
//

void init_style_enable_mode_btn_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[eez_flow_get_selected_theme_index()][1]));
    lv_style_set_radius(style, 0);
};

lv_style_t *get_style_enable_mode_btn_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_enable_mode_btn_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_enable_mode_btn_MAIN_CHECKED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(theme_colors[eez_flow_get_selected_theme_index()][1]));
};

lv_style_t *get_style_enable_mode_btn_MAIN_CHECKED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_enable_mode_btn_MAIN_CHECKED(style);
    }
    return style;
};

void add_style_enable_mode_btn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_enable_mode_btn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_enable_mode_btn_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_translate_y(obj, 3, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(obj, 170, LV_PART_MAIN | LV_STATE_PRESSED);
};

void remove_style_enable_mode_btn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_enable_mode_btn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_enable_mode_btn_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
};

//
// Style: kc
//

void init_style_kc_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xff000000));
    lv_style_set_bg_opa(style, 200);
};

lv_style_t *get_style_kc_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_kc_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_kc(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_kc_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_kc(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_kc_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: out_display
//

void init_style_out_display_INDICATOR_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xffff0000));
};

lv_style_t *get_style_out_display_INDICATOR_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_out_display_INDICATOR_DEFAULT(style);
    }
    return style;
};

void init_style_out_display_MAIN_PRESSED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xfff50202));
};

lv_style_t *get_style_out_display_MAIN_PRESSED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_out_display_MAIN_PRESSED(style);
    }
    return style;
};

void init_style_out_display_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xffff0101));
};

lv_style_t *get_style_out_display_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_out_display_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_out_display(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_out_display_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_out_display_MAIN_PRESSED(), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(obj, get_style_out_display_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_out_display(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_out_display_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_out_display_MAIN_PRESSED(), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_remove_style(obj, get_style_out_display_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: Error_Style
//

void init_style_error_style_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xffffc800));
    lv_style_set_align(style, LV_ALIGN_CENTER);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_font(style, &ui_font_kalameh_regular_12);
};

lv_style_t *get_style_error_style_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_error_style_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_error_style(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_error_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_error_style(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_error_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_en_s,
        add_style_dns,
        add_style_disable_mode_btn,
        add_style_enable_mode_btn,
        add_style_kc,
        add_style_out_display,
        add_style_error_style,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_en_s,
        remove_style_dns,
        remove_style_disable_mode_btn,
        remove_style_enable_mode_btn,
        remove_style_kc,
        remove_style_out_display,
        remove_style_error_style,
    };
    remove_style_funcs[styleIndex](obj);
}
