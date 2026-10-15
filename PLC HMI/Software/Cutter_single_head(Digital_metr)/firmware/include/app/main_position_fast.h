#pragma once

// Fast main-position renderer. The legacy full-width label invalidates about
// 72K pixels on every number change. These cached labels redraw only digits
// whose characters changed while preserving the full stop-motion touch area.
static lv_obj_t *main_fast_hitbox = NULL;
static lv_obj_t *main_fast_parent = NULL;
static const lv_font_t *main_fast_font = NULL;
static bool main_fast_fallback_active = false;
static int16_t main_fast_digit_x[6] = { INT16_MIN, INT16_MIN, INT16_MIN, INT16_MIN, INT16_MIN, INT16_MIN };
static int16_t main_fast_digit_w[6] = { 0, 0, 0, 0, 0, 0 };
// LVGL's normal label setter allocates and frees text storage on every call.
// Persistent one-character strings remove that heap churn from the 40 Hz path.
static char main_fast_digit_text[6][2] = {};

// Destination value uses the same rule: keep the original label as the
// touch hitbox, but redraw only characters whose pixels actually changed.
static lv_obj_t *main_fast_dest_parent = NULL;
static lv_obj_t *main_fast_dest_digit[8] = {};
static char main_fast_dest_text[8][2] = {};
static char main_fast_dest_cache[9] = "";
static int8_t main_fast_dest_len = -1;
static int16_t main_fast_dest_x[8] = {};
static int16_t main_fast_dest_w[8] = {};

static void main_fast_destination_reset() {
  main_fast_dest_parent = NULL;
  main_fast_dest_len = -1;
  main_fast_dest_cache[0] = '\0';
  for (int i = 0; i < 8; i++) {
    main_fast_dest_digit[i] = NULL;
    main_fast_dest_text[i][0] = '\0';
    main_fast_dest_text[i][1] = '\0';
    main_fast_dest_x[i] = INT16_MIN;
    main_fast_dest_w[i] = 0;
  }
}

static bool main_fast_destination_ready() {
  if (!objects.lable1_1 || !lv_obj_is_valid(objects.lable1_1)) return false;
  if (main_fast_dest_parent != objects.lable1_1) main_fast_destination_reset();
  if (main_fast_dest_parent) return true;

  main_fast_dest_parent = objects.lable1_1;
  // The parent remains the existing clickable scrub/help target.
  lv_label_set_text_static(main_fast_dest_parent, "");
  lv_obj_add_flag(main_fast_dest_parent, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  for (int i = 0; i < 8; i++) {
    main_fast_dest_digit[i] = lv_label_create(main_fast_dest_parent);
    if (!main_fast_dest_digit[i]) {
      for (int j = 0; j < i; j++) {
        if (main_fast_dest_digit[j]) lv_obj_delete(main_fast_dest_digit[j]);
      }
      main_fast_destination_reset();
      return false;
    }
    lv_obj_clear_flag(main_fast_dest_digit[i],
                      (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_add_flag(main_fast_dest_digit[i], LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_text_font(main_fast_dest_digit[i], &ui_font_digit80,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(main_fast_dest_digit[i], LV_TEXT_ALIGN_LEFT,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(main_fast_dest_digit[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(main_fast_dest_digit[i], LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(main_fast_dest_digit[i], main_fast_dest_text[i]);
    lv_obj_add_flag(main_fast_dest_digit[i], LV_OBJ_FLAG_HIDDEN);
  }
  return true;
}

static void main_fast_destination_layout(const char *txt, int len) {
  const lv_font_t *font = &ui_font_digit80;
  const int box_w = 307;
  const int box_h = 82;
  const int usable_w = box_w - 4;
  int total_w = 0;
  for (int i = 0; i < len; i++)
    total_w += lv_font_get_glyph_width(font, (uint8_t)txt[i], (uint8_t)txt[i + 1]);
  int pen_x = (usable_w - total_w) / 2;
  if (pen_x < 0) pen_x = 0;
  const int slot_h = 80;
  const int y = 0;  // ارقام مقصد کمی بالاتر داخل باکس
  for (int i = 0; i < len; i++) {
    lv_font_glyph_dsc_t glyph;
    lv_font_get_glyph_dsc(font, &glyph, (uint8_t)txt[i], (uint8_t)txt[i + 1]);
    const int draw_w = LV_MAX((int)glyph.adv_w,
                              (int)glyph.ofs_x + (int)glyph.box_w) + 4;
    if (main_fast_dest_x[i] != pen_x || main_fast_dest_w[i] != draw_w) {
      lv_obj_set_pos(main_fast_dest_digit[i], pen_x, y);
      lv_obj_set_size(main_fast_dest_digit[i], draw_w, slot_h);
      main_fast_dest_x[i] = pen_x;
      main_fast_dest_w[i] = draw_w;
    }
    pen_x += glyph.adv_w;
  }
  main_fast_dest_len = len;
}

static bool main_fast_destination_set_text(const char *txt) {
  const int len = (txt && txt[0]) ? (int)strlen(txt) : 0;
  if (len < 1 || len > 8 || !main_fast_destination_ready()) return false;

  if (len != main_fast_dest_len ||
      (strchr(main_fast_dest_cache, '.') != NULL) != (strchr(txt, '.') != NULL) ||
      main_fast_dest_cache[0] == '-' || txt[0] == '-') {
    main_fast_destination_layout(txt, len);
  }
  for (int i = 0; i < 8; i++) {
    if (i < len) {
      if (main_fast_dest_cache[i] != txt[i]) {
        main_fast_dest_text[i][0] = txt[i];
        main_fast_dest_text[i][1] = '\0';
        lv_label_set_text_static(main_fast_dest_digit[i], main_fast_dest_text[i]);
      }
      lv_obj_clear_flag(main_fast_dest_digit[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(main_fast_dest_digit[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
  memcpy(main_fast_dest_cache, txt, (size_t)len);
  main_fast_dest_cache[len] = '\0';
  return true;
}

static void main_fast_reset_refs() {
  main_fast_hitbox = NULL;
  main_fast_parent = NULL;
  main_fast_font = NULL;
  main_fast_fallback_active = false;
  for (int i = 0; i < 6; i++) {
    main_pos_digit_lbl[i] = NULL;
    main_fast_digit_x[i] = INT16_MIN;
    main_fast_digit_w[i] = 0;
    main_fast_digit_text[i][0] = '\0';
    main_fast_digit_text[i][1] = '\0';
  }
  main_pos_digit_cache[0] = '\0';
  main_pos_digit_len = -1;
}

static bool main_fast_position_digits_ready() {
  if (!main1_safe_mode || !objects.main_1 || !objects.lable1_2) return false;

  if (main_fast_parent != objects.main_1) main_fast_reset_refs();
  if (main_fast_hitbox) {
    if (main_fast_fallback_active) return false;
    lv_obj_clear_flag(main_fast_hitbox, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.lable1_2, LV_OBJ_FLAG_HIDDEN);
    return true;
  }

  main_fast_parent = objects.main_1;
  main_fast_hitbox = lv_obj_create(main_fast_parent);
  if (!main_fast_hitbox) {
    main_fast_reset_refs();
    return false;
  }
  // کمی بالاتر/بلندتر تا نوک ایتالیک digit160 (مثلاً «۱») بریده نشود
  lv_obj_set_pos(main_fast_hitbox, 5, 78);
  lv_obj_set_size(main_fast_hitbox, 470, 167);
  lv_obj_clear_flag(main_fast_hitbox, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(main_fast_hitbox, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_OVERFLOW_VISIBLE));
  lv_obj_set_style_bg_opa(main_fast_hitbox, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(main_fast_hitbox, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(main_fast_hitbox, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_clip_corner(main_fast_hitbox, false, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(main_fast_hitbox, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(main_fast_hitbox, lable1_2_click_event_handler, LV_EVENT_CLICKED, NULL);

  for (int i = 0; i < 6; i++) {
    main_pos_digit_lbl[i] = lv_label_create(main_fast_hitbox);
    if (!main_pos_digit_lbl[i]) {
      lv_obj_delete(main_fast_hitbox);
      main_fast_reset_refs();
      return false;
    }
    lv_obj_clear_flag(main_pos_digit_lbl[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(main_pos_digit_lbl[i], LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_text_align(main_pos_digit_lbl[i], LV_TEXT_ALIGN_LEFT,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(main_pos_digit_lbl[i], lv_color_hex(main_pos_digit_color),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(main_pos_digit_lbl[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(main_pos_digit_lbl[i], false, LV_PART_MAIN | LV_STATE_DEFAULT);
    // CLIP با باکس تنگ نوک ایتالیک را می‌بُرد؛ WRAP برای تک‌رقم بی‌خطر است
    lv_label_set_long_mode(main_pos_digit_lbl[i], LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(main_pos_digit_lbl[i], main_fast_digit_text[i]);
    lv_obj_add_flag(main_pos_digit_lbl[i], LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_add_flag(objects.lable1_2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(main_fast_hitbox);
  return true;
}

static void main_fast_position_digits_layout(const char *txt, int len) {
  if (!main_fast_position_digits_ready()) return;

  // Hitbox کمی بزرگ‌تر از لیبل قدیمی؛ حاشیه راست برای overhang ایتالیک.
  const int box_w = 470;
  const int box_h = 167;
  const int pad_right = 35;
  lv_obj_set_pos(main_fast_hitbox, 5, 78);
  lv_obj_set_size(main_fast_hitbox, box_w, box_h);

  const lv_font_t *font = &ui_font_digit160;
  // اعشار: کمی کوچک‌تر تا رقم+ممیز در دیسک جا شود و خوانا بماند
  if (strchr(txt, '.') != NULL) font = &ui_font_digit120;
  int total_w = 0;
  for (int i = 0; i < len; i++) total_w += lv_font_get_glyph_width(font, (uint8_t)txt[i], (uint8_t)txt[i + 1]);
  // A five-digit digit160 value is 430 px wide and still fits the original
  // 463 px clip box; keep its typography instead of shrinking it prematurely.
  // overflow: فقط digit120 (digit140 حذف شد برای فضای فلش — سناریوی عادی ≤۴ رقم عوض نمی‌شود)
  if (total_w > (box_w - pad_right)) {
    font = &ui_font_digit120;
    total_w = 0;
    for (int i = 0; i < len; i++) total_w += lv_font_get_glyph_width(font, (uint8_t)txt[i], (uint8_t)txt[i + 1]);
  }

  int pen_x = (box_w - pad_right - total_w) / 2;
  if (pen_x < 4) pen_x = 4;
  // +8 ارتفاع و +4 از بالا: نوک ایتالیک «۱» دیگر به لبهٔ CLIP نمی‌خورد
  int slot_h = lv_font_get_line_height(font) + 8;
  if (slot_h > box_h - 4) slot_h = box_h - 4;
  // فونت کوچک‌تر (اعشار) هم وسط عمودی جعبه بماند — نه چسبیده به بالا
  int y = (box_h - slot_h) / 2;
  if (y < 2) y = 2;

  for (int i = 0; i < 6; i++) {
    if (main_fast_font != font) {
      lv_obj_set_style_text_font(main_pos_digit_lbl[i], font, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (i < len) {
      lv_font_glyph_dsc_t glyph;
      const uint32_t letter = (uint8_t)txt[i];
      const uint32_t next = (uint8_t)txt[i + 1];
      lv_font_get_glyph_dsc(font, &glyph, letter, next);
      // italic overhang + AA — قبلاً +2 کافی نبود و گوشهٔ بالا-راست «۱» بریده می‌شد
      int draw_w = LV_MAX((int)glyph.adv_w, (int)glyph.ofs_x + (int)glyph.box_w) + 10;
      if (main_fast_digit_x[i] != pen_x || main_fast_digit_w[i] != draw_w || main_fast_font != font) {
        lv_obj_set_pos(main_pos_digit_lbl[i], pen_x, y);
        lv_obj_set_size(main_pos_digit_lbl[i], draw_w, slot_h);
        main_fast_digit_x[i] = pen_x;
        main_fast_digit_w[i] = draw_w;
      }
      pen_x += glyph.adv_w;
    }
  }
  main_fast_font = font;
  main_pos_digit_len = len;
  // اگر دیالوگ باز است ارقام را روی آن نیاور
  if (objects.panel1_3 && !lv_obj_has_flag(objects.panel1_3, LV_OBJ_FLAG_HIDDEN))
    lv_obj_move_foreground(objects.panel1_3);
  else if (objects.panel1_1 && !lv_obj_has_flag(objects.panel1_1, LV_OBJ_FLAG_HIDDEN))
    lv_obj_move_foreground(objects.panel1_1);
  else
    lv_obj_move_foreground(main_fast_hitbox);
}

static void main_fast_position_digits_set_color(uint32_t color) {
  if (main_pos_digit_color == color && main_fast_hitbox) return;
  main_pos_digit_color = color;
  if (!main_fast_position_digits_ready()) return;
  for (int i = 0; i < 6; i++) {
    lv_obj_set_style_text_color(main_pos_digit_lbl[i], lv_color_hex(color),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static bool main_fast_position_digits_set_text(const char *txt) {
  const int len = (txt && txt[0]) ? (int)strlen(txt) : 0;
  if (len < 1 || len > 6) {
    main_fast_fallback_active = true;
    if (main_fast_hitbox) lv_obj_add_flag(main_fast_hitbox, LV_OBJ_FLAG_HIDDEN);
    if (objects.lable1_2) {
      lv_obj_set_style_text_color(objects.lable1_2, lv_color_hex(main_pos_digit_color),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_clear_flag(objects.lable1_2, LV_OBJ_FLAG_HIDDEN);
    }
    return false;
  }

  main_fast_fallback_active = false;
  if (!main_fast_position_digits_ready()) return false;
  const bool was_dec = (strchr(main_pos_digit_cache, '.') != NULL);
  const bool now_dec = (strchr(txt, '.') != NULL);
  const bool layout_changed = len != main_pos_digit_len ||
                              was_dec != now_dec ||
                              main_pos_digit_cache[0] == '-' || txt[0] == '-' ||
                              main_fast_font == NULL;
  if (layout_changed) main_fast_position_digits_layout(txt, len);

  for (int i = 0; i < 6; i++) {
    if (i < len) {
      if (main_pos_digit_cache[i] != txt[i]) {
        main_fast_digit_text[i][0] = txt[i];
        main_fast_digit_text[i][1] = '\0';
        lv_label_set_text_static(main_pos_digit_lbl[i], main_fast_digit_text[i]);
      }
      lv_obj_clear_flag(main_pos_digit_lbl[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(main_pos_digit_lbl[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  memcpy(main_pos_digit_cache, txt, (size_t)len);
  main_pos_digit_cache[len] = '\0';
  lv_obj_add_flag(objects.lable1_2, LV_OBJ_FLAG_HIDDEN);
  return true;
}

// Task2 calls these functions directly; the actively locked ui_extras.h stays untouched.
