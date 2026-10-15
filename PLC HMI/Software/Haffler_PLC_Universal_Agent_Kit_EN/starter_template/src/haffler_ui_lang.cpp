#include "haffler_ui_lang.h"
#include <stdio.h>
#include <string.h>

#define MAX_REGISTERED_LABELS 64

typedef struct {
    lv_obj_t *label;
    str_id_t  str_id;
} registered_label_t;

static app_lang_t s_current_lang = LANG_EN;
static registered_label_t s_registered_labels[MAX_REGISTERED_LABELS];
static uint8_t s_registered_count = 0;

// Static Trilingual String Matrix: FA (Persian), EN (English), TR (Turkish)
static const char* const s_dict[STR_MAX_KEYS][3] = {
    /* STR_READY         */ {"آماده به کار",   "READY",             "HAZIR"},
    /* STR_RUNNING       */ {"در حال کار",     "RUNNING",           "CALISIYOR"},
    /* STR_STOPPED       */ {"متوقف شد",       "STOPPED",           "DURDURULDU"},
    /* STR_TARGET_VAL    */ {"مقدار هدف",      "Target Value",      "Hedef Deger"},
    /* STR_ACTUAL_VAL    */ {"مقدار فعلی",     "Actual Value",      "Mevcut Deger"},
    /* STR_PIECE_COUNT   */ {"تعداد قطعات",    "Piece Count",       "Parca Sayisi"},
    /* STR_SETTINGS      */ {"تنظیمات دستگاه", "Settings",          "Ayarlar"},
    /* STR_DIAGNOSTICS   */ {"دیاگ سخت‌افزار", "Diagnostics",       "Teshis"},
    /* STR_DAY_LOCK_WARN */ {"مهلت قسط",       "Lease Expiring",    "Taksit Uyari"},
    /* STR_CONFIRM       */ {"تایید",          "Confirm",           "Onayla"},
    /* STR_CANCEL        */ {"انصراف",         "Cancel",            "Iptal"},
    /* STR_RESET         */ {"بازنشانی",       "Reset",             "Sifirla"},
    /* STR_ERROR_ESTOP   */ {"توقف اضطراری",   "EMERGENCY STOP",    "ACIL DURDURMA"}
};

void ui_lang_init(app_lang_t default_lang) {
    s_current_lang = default_lang;
    s_registered_count = 0;
}

const char* ui_lang_get(str_id_t id) {
    if (id >= STR_MAX_KEYS) return "";
    return s_dict[id][s_current_lang];
}

void ui_lang_set(app_lang_t lang) {
    s_current_lang = lang;
    // Zero-leak update of all registered UI labels
    for (uint8_t i = 0; i < s_registered_count; ++i) {
        if (lv_obj_is_valid(s_registered_labels[i].label)) {
            lv_label_set_text(s_registered_labels[i].label, ui_lang_get(s_registered_labels[i].str_id));
        }
    }
}

app_lang_t ui_lang_current(void) {
    return s_current_lang;
}

void ui_lang_register_label(lv_obj_t *lbl, str_id_t str_id) {
    if (!lbl || s_registered_count >= MAX_REGISTERED_LABELS) return;
    s_registered_labels[s_registered_count].label = lbl;
    s_registered_labels[s_registered_count].str_id = str_id;
    s_registered_count++;
    lv_label_set_text(lbl, ui_lang_get(str_id));
}

static void pill_badge_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = (lv_obj_t *)lv_event_get_user_data(e);

    // Cycle language: EN -> FA -> TR -> EN
    app_lang_t next = (app_lang_t)((s_current_lang + 1) % 3);
    ui_lang_set(next);

    switch (next) {
        case LANG_FA:
            lv_label_set_text(lbl, "فا");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x16A34A), 0); // Emerald Green
            break;
        case LANG_EN:
            lv_label_set_text(lbl, "EN");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0284C7), 0); // Apple Blue
            break;
        case LANG_TR:
            lv_label_set_text(lbl, "TR");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xDC2626), 0); // Crimson Red
            break;
    }
}

lv_obj_t* ui_lang_create_pill_badge(lv_obj_t *parent, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 52, 24);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 12, 0); // Pill shape
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0284C7), 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "EN");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, pill_badge_event_cb, LV_EVENT_CLICKED, lbl);
    return btn;
}
