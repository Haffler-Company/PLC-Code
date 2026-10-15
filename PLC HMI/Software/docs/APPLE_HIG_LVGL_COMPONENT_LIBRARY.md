# کتابخانه ویجت‌های آماده LVGL به سبک Apple HIG (Component Library)
## الگوهای کدنویسی آماده و تست‌شده C/C++ برای LVGL نسخه ۸ و ۹ روی نمایشگر ۴۸۰×۳۲۰

> **راهنمای استفاده برای ایجنت‌ها:** کدهای زیر بدون خطا، بدون سایه‌های سنگین نرم‌افزاری و بر مبنای شبکه پیکسلی ۸ اپل طراحی شده‌اند. ایجنت‌ها مجازند مستقیماً از این توابع برای خلق یا ارتقای صفحات استفاده نمایند.

---

## ۱. کیپد عددی مودال اپل (Apple Industrial Modal Numpad)

این ویجت برای ورود طول برش، وزن چانه، دمای هدف یا کالیبراسیون استفاده می‌شود.

```c
#include "lvgl.h"
#include <stdio.h>

typedef void (*numpad_callback_t)(float new_val);

typedef struct {
    lv_obj_t* modal_bg;
    lv_obj_t* card;
    lv_obj_t* lbl_display;
    char buffer[16];
    float min_val;
    float max_val;
    const char* unit_str;
    numpad_callback_t on_confirm;
} apple_numpad_t;

static apple_numpad_t s_numpad;

static void numpad_btn_event_cb(lv_event_t* e) {
    const char* txt = (const char*)lv_event_get_user_data(e);
    size_t len = strlen(s_numpad.buffer);

    if (strcmp(txt, "DEL") == 0) {
        if (len > 0) s_numpad.buffer[len - 1] = '\0';
    } else if (strcmp(txt, "OK") == 0) {
        float val = atof(s_numpad.buffer);
        if (val >= s_numpad.min_val && val <= s_numpad.max_val) {
            if (s_numpad.on_confirm) s_numpad.on_confirm(val);
            lv_obj_del(s_numpad.modal_bg);
        } else {
            // انیمیشن یا رنگ قرمز در صورت خروج از محدوده
            lv_obj_set_style_text_color(s_numpad.lbl_display, lv_color_hex(0xFF453A), 0);
        }
        return;
    } else if (strcmp(txt, "CLOSE") == 0) {
        lv_obj_del(s_numpad.modal_bg);
        return;
    } else {
        if (len < 10) {
            strcat(s_numpad.buffer, txt);
        }
    }
    lv_label_set_text(s_numpad.lbl_display, s_numpad.buffer[0] ? s_numpad.buffer : "0");
}

void open_apple_numpad(lv_obj_t* parent, float init_val, float min_v, float max_v, 
                       const char* title_fa, const char* unit, numpad_callback_t cb) 
{
    s_numpad.min_val = min_v;
    s_numpad.max_val = max_v;
    s_numpad.unit_str = unit;
    s_numpad.on_confirm = cb;
    snprintf(s_numpad.buffer, sizeof(s_numpad.buffer), "%.1f", init_val);

    // ۱. لایه تیره پس‌زمینه مودال (Dim Background)
    s_numpad.modal_bg = lv_obj_create(parent);
    lv_obj_set_size(s_numpad.modal_bg, 480, 320);
    lv_obj_set_style_bg_color(s_numpad.modal_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_numpad.modal_bg, LV_OPA_70, 0);
    lv_obj_clear_flag(s_numpad.modal_bg, LV_OBJ_FLAG_SCROLLABLE);

    // ۲. بدنه اصلی پنجره کیپد (Card)
    s_numpad.card = lv_obj_create(s_numpad.modal_bg);
    lv_obj_set_size(s_numpad.card, 280, 290);
    lv_obj_center(s_numpad.card);
    lv_obj_set_style_bg_color(s_numpad.card, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(s_numpad.card, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(s_numpad.card, 1, 0);
    lv_obj_set_style_radius(s_numpad.card, 14, 0);
    lv_obj_set_style_pad_all(s_numpad.card, 10, 0);

    // ۳. نمایشگر عدد ورودی (Display Strip)
    lv_obj_t* disp_box = lv_obj_create(s_numpad.card);
    lv_obj_set_size(disp_box, 256, 44);
    lv_obj_align(disp_box, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(disp_box, lv_color_hex(0x0A0C10), 0);
    lv_obj_set_style_radius(disp_box, 8, 0);
    lv_obj_set_style_border_width(disp_box, 0, 0);

    s_numpad.lbl_display = lv_label_create(disp_box);
    lv_label_set_text(s_numpad.lbl_display, s_numpad.buffer);
    lv_obj_set_style_text_color(s_numpad.lbl_display, lv_color_hex(0xF0F6FC), 0);
    lv_obj_set_style_text_font(s_numpad.lbl_display, &lv_font_montserrat_24, 0);
    lv_obj_align(s_numpad.lbl_display, LV_ALIGN_RIGHT_MID, -8, 0);

    // ۴. ماتریس کلیدها (شبکه ۴×۳)
    const char* keys[12] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        ".", "0", "DEL"
    };

    for (int i = 0; i < 12; i++) {
        int r = i / 3;
        int c = i % 3;
        lv_obj_t* btn = lv_btn_create(s_numpad.card);
        lv_obj_set_size(btn, 80, 42);
        lv_obj_set_pos(btn, c * 86 + 2, 54 + r * 46);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x21262D), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x30363D), LV_STATE_PRESSED);
        
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, keys[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xF0F6FC), 0);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, numpad_btn_event_cb, LV_EVENT_CLICKED, (void*)keys[i]);
    }

    // ۵. کلید تأیید بزرگ پایین (Confirm Button)
    lv_obj_t* btn_ok = lv_btn_create(s_numpad.card);
    lv_obj_set_size(btn_ok, 256, 42);
    lv_obj_align(btn_ok, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x30D158), 0); // سبز اپل
    lv_obj_set_style_radius(btn_ok, 8, 0);
    
    lv_obj_t* lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "تأیید");
    lv_obj_center(lbl_ok);
    lv_obj_add_event_cb(btn_ok, numpad_btn_event_cb, LV_EVENT_CLICKED, (void*)"OK");
}
```

---

## ۲. سلکتور چندحالته اپل (Apple Segmented Control)

این ویجت برای تغییر زوایای برش (۴۵° / ۹۰° / ۱۳۵°) یا تغییر مودهای کاری (دستی / اتوماتیک) به کار می‌رود:

```c
lv_obj_t* create_apple_segmented_control(lv_obj_t* parent, lv_coord_t w, lv_coord_t h, 
                                        const char* options[], uint8_t count, 
                                        lv_event_cb_t on_change_cb) 
{
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, w, h);
    lv_obj_set_style_radius(container, 8, 0);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_border_color(container, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_pad_all(container, 3, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_coord_t seg_w = (w - 6) / count;

    for (uint8_t i = 0; i < count; i++) {
        lv_obj_t* btn = lv_btn_create(container);
        lv_obj_set_size(btn, seg_w, h - 8);
        lv_obj_set_pos(btn, i * seg_w, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        
        // تب پیش‌فرض یا تب انتخاب‌شده:
        if (i == 0) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0A84FF), 0); // آبی اپل
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x21262D), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        }

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, options[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xF0F6FC), 0);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, on_change_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    return container;
}
```

---

## ۳. بنر اعلان و آلارم کشویی اپل (Apple Industrial Alarm Sheet)

```c
lv_obj_t* show_apple_alarm_banner(lv_obj_t* parent, const char* title, const char* msg, bool is_critical) {
    lv_obj_t* banner = lv_obj_create(parent);
    lv_obj_set_size(banner, 460, 56);
    lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 8);
    
    lv_color_t border_col = is_critical ? lv_color_hex(0xFF453A) : lv_color_hex(0xFF9F0A);
    lv_obj_set_style_bg_color(banner, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(banner, border_col, 0);
    lv_obj_set_style_border_width(banner, 2, 0);
    lv_obj_set_style_radius(banner, 10, 0);
    lv_obj_set_style_pad_all(banner, 8, 0);

    lv_obj_t* lbl_title = lv_label_create(banner);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, border_col, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_RIGHT, 0, 0);

    lv_obj_t* lbl_msg = lv_label_create(banner);
    lv_label_set_text(lbl_msg, msg);
    lv_obj_set_style_text_color(lbl_msg, lv_color_hex(0x8B949E), 0);
    lv_obj_align(lbl_msg, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    return banner;
}
```
