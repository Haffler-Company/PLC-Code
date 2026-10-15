// Multi-model printer driver task: Model 0 (Standard ESC/POS UART) & Model 1 (CSN-A2 5-9V TTL + USB)
#pragma once
#include <Arduino.h>
#include <time.h>
#include <lvgl.h>
#include "app/admin_svc.h"
#include "jalali.h"
#include "ui/fonts.h"

extern String g_company_name;
extern SemaphoreHandle_t lable1_1_Semaphore;

static inline void printer_send_init() {
  printer.write(0x1B);
  printer.write('@'); // ESC @: Initialize printer
  vTaskDelay(pdMS_TO_TICKS(15));
  if (printer_model == 1) { // CSN-A2 (5-9V TTL + USB)
    // ESC 7 n1 n2 n3: Gentle heating parameters for 5-9V operation
    // max heating dots=4 (spreads peak load to prevent 5V power sag), heat time=64, heat interval=8
    printer.write(0x1B);
    printer.write(0x37);
    printer.write(4);
    printer.write(64);
    printer.write(8);
    // DC2 # n: Print density
    printer.write(0x12);
    printer.write(0x23);
    printer.write(0x40);
    // ESC 2: Default line spacing
    printer.write(0x1B);
    printer.write(0x32);
  }
  vTaskDelay(pdMS_TO_TICKS(15));
}

static inline void printer_send_align(uint8_t align) {
  printer.write(0x1B);
  printer.write(0x61);
  printer.write(align); // 0=Left, 1=Center, 2=Right
}

static inline void printer_send_size(uint8_t size) {
  printer.write(0x1D);
  printer.write(0x21);
  if (printer_model == 1) {
    // CSN-A2 safe size: map 0x77 (8x8) to 0x11 (2x2) to prevent text overflow
    if (size == 0x77) printer.write(0x11);
    else printer.write(size);
  } else {
    printer.write(size);
  }
}

static inline void printer_send_feed(uint8_t lines = 1) {
  if (printer_model == 1) {
    // ESC d n: feed exactly n lines
    printer.write(0x1B);
    printer.write(0x64);
    printer.write(lines);
  } else {
    for (uint8_t i = 0; i < lines; i++) printer.println();
  }
}

// Sends a raw monochrome bit-image using ESC/POS "GS v 0" (384 dots width = 48 bytes/row)
static inline void printer_send_bitmap_row(const uint8_t *bitmap_data, uint16_t width_bytes, uint16_t height_dots) {
  if (!bitmap_data || width_bytes == 0 || height_dots == 0) return;
  printer.write(0x1D);
  printer.write('v');
  printer.write('0');
  printer.write(0x00); // normal density mode 0
  printer.write((uint8_t)(width_bytes & 0xFF));
  printer.write((uint8_t)((width_bytes >> 8) & 0xFF));
  printer.write((uint8_t)(height_dots & 0xFF));
  printer.write((uint8_t)((height_dots >> 8) & 0xFF));
  printer.write(bitmap_data, width_bytes * height_dots);
  vTaskDelay(pdMS_TO_TICKS(10)); // gentle pacing to protect power rail
}

// Renders shaped Persian text into an off-screen LVGL canvas and prints it as ESC/POS raster bitmap
static inline void printer_print_persian_bitmap(const char *text, const lv_font_t *font = &ui_font_naz20, lv_text_align_t align = LV_TEXT_ALIGN_CENTER, uint16_t custom_h = 0) {
  if (!text || strlen(text) == 0) {
    printer_send_feed(1);
    return;
  }
  if (!font) font = &ui_font_naz20;
  uint16_t h = custom_h > 0 ? custom_h : (font->line_height + 6);
  if (h < 22) h = 22;
  if (h > 64) h = 64;
  const uint16_t w = 384;
  const uint16_t w_bytes = 48;

  if (lable1_1_Semaphore && xSemaphoreTake(lable1_1_Semaphore, pdMS_TO_TICKS(100)) != pdTRUE) {
    printer.println(text);
    return;
  }

  lv_draw_buf_t *draw_buf = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
  if (!draw_buf) {
    if (lable1_1_Semaphore) xSemaphoreGive(lable1_1_Semaphore);
    printer.println(text);
    return;
  }

  lv_obj_t *canvas = lv_canvas_create(lv_layer_top());
  if (!canvas) {
    lv_draw_buf_destroy(draw_buf);
    if (lable1_1_Semaphore) xSemaphoreGive(lable1_1_Semaphore);
    printer.println(text);
    return;
  }
  lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
  lv_canvas_set_draw_buf(canvas, draw_buf);
  lv_canvas_fill_bg(canvas, lv_color_hex(0xffffff), LV_OPA_COVER);

  lv_layer_t layer;
  lv_canvas_init_layer(canvas, &layer);

  lv_draw_label_dsc_t label_dsc;
  lv_draw_label_dsc_init(&label_dsc);
  label_dsc.color = lv_color_hex(0x000000);
  label_dsc.font = font;
  label_dsc.text = text;
  label_dsc.bidi_dir = LV_BASE_DIR_RTL;
  label_dsc.align = align;

  lv_area_t coords = {0, 0, (int32_t)w, (int32_t)h};
  lv_draw_label(&layer, &label_dsc, &coords);
  lv_canvas_finish_layer(canvas, &layer);

  uint8_t *mono_buf = (uint8_t *)malloc(w_bytes * h);
  if (mono_buf) {
    memset(mono_buf, 0, w_bytes * h);
    for (uint16_t y = 0; y < h; y++) {
      for (uint16_t x = 0; x < w; x++) {
        lv_color32_t px = lv_canvas_get_px(canvas, x, y);
        if (px.red < 128 && px.green < 128 && px.blue < 128) {
          mono_buf[y * w_bytes + (x / 8)] |= (1 << (7 - (x % 8)));
        }
      }
    }
    printer_send_bitmap_row(mono_buf, w_bytes, h);
    free(mono_buf);
  } else {
    printer.println(text);
  }

  lv_obj_delete(canvas);
  lv_draw_buf_destroy(draw_buf);
  if (lable1_1_Semaphore) xSemaphoreGive(lable1_1_Semaphore);
}

// Token-aware printing: prevents numbers from breaking across line wrap
static inline void printer_print_token(const char *token, int &line_len, int max_cols = 30, const char *sep = " - ") {
  if (!token) return;
  int t_len = (int)strlen(token);
  int s_len = (line_len > 0 && sep) ? (int)strlen(sep) : 0;
  if (line_len + s_len + t_len > max_cols && line_len > 0) {
    printer.println();
    printer.print("  ");
    line_len = 2;
    s_len = 0;
  }
  if (line_len > 2 && s_len > 0 && sep) {
    printer.print(sep);
    line_len += s_len;
  }
  printer.print(token);
  line_len += t_len;
}
static inline void printer_print_token(const String &token, int &line_len, int max_cols = 30, const char *sep = " - ") {
  printer_print_token(token.c_str(), line_len, max_cols, sep);
}

static inline void printer_print_header(const char *doc_title) {
  printer_send_init();
  const bool fa = (GUI.system_language == 1);

  if (fa) {
    printer_print_persian_bitmap("هافلر - برش تک‌سر", &ui_font_naz24, LV_TEXT_ALIGN_CENTER, 30);
    printer.println("================================");

    if (g_company_name.length() > 0) {
      char comp_buf[64];
      snprintf(comp_buf, sizeof(comp_buf), "کارگاه: %s", g_company_name.c_str());
      printer_print_persian_bitmap(comp_buf, &ui_font_naz20, LV_TEXT_ALIGN_RIGHT, 26);
    }

    int y = 2026, m = 1, d = 1, hh = 0, mi = 0, ss = 0;
    bool time_ok = false;
    if (soft_clock_valid && soft_now_epoch() > 0) {
      epoch_to_civil(soft_now_epoch(), &y, &m, &d, &hh, &mi, &ss);
      time_ok = true;
    }
    if (!time_ok) time_ok = ds3231_read_time(&y, &m, &d, &hh, &mi, &ss);
    if (!time_ok) {
      time_t now_sec;
      time(&now_sec);
      struct tm ti;
      localtime_r(&now_sec, &ti);
      y = ti.tm_year + 1900; m = ti.tm_mon + 1; d = ti.tm_mday;
      hh = ti.tm_hour; mi = ti.tm_min;
    }

    int jy = 0, jm = 0, jd = 0;
    gregorian_to_jalali(y, m, d, &jy, &jm, &jd);
    char dt_buf[64];
    snprintf(dt_buf, sizeof(dt_buf), "تاریخ: %04d/%02d/%02d  ساعت: %02d:%02d", jy, jm, jd, hh, mi);
    printer_print_persian_bitmap(dt_buf, &ui_font_naz18b, LV_TEXT_ALIGN_RIGHT, 24);

    char meta_buf[64];
    snprintf(meta_buf, sizeof(meta_buf), "واحد: میلیمتر  ضخامت تیغه: %.1f mm", (double)OPT.kerf_mm);
    printer_print_persian_bitmap(meta_buf, &ui_font_naz18b, LV_TEXT_ALIGN_RIGHT, 24);

    char stat_buf[64];
    snprintf(stat_buf, sizeof(stat_buf), "کل شاخه‌ها: %d  کل قطعات: %d", OPT.bar_count, OPT.piece_count);
    printer_print_persian_bitmap(stat_buf, &ui_font_naz18b, LV_TEXT_ALIGN_RIGHT, 24);

    printer.println("--------------------------------");
    printer_print_persian_bitmap(doc_title, &ui_font_naz20b, LV_TEXT_ALIGN_CENTER, 28);
    printer.println("--------------------------------");
  } else {
    printer_send_align(1); // Center
    printer_send_size(0x11); // Double width & height
    printer.println("HAFFLER CUTTER");
    printer_send_size(0x00); // Normal size
    printer.println("================================");

    if (g_company_name.length() > 0) {
      printer.print("Company: ");
      printer.println(g_company_name.c_str());
    }

    int y = 2026, m = 1, d = 1, hh = 0, mi = 0, ss = 0;
    bool time_ok = false;
    if (soft_clock_valid && soft_now_epoch() > 0) {
      epoch_to_civil(soft_now_epoch(), &y, &m, &d, &hh, &mi, &ss);
      time_ok = true;
    }
    if (!time_ok) time_ok = ds3231_read_time(&y, &m, &d, &hh, &mi, &ss);
    if (!time_ok) {
      time_t now_sec;
      time(&now_sec);
      struct tm ti;
      localtime_r(&now_sec, &ti);
      y = ti.tm_year + 1900; m = ti.tm_mon + 1; d = ti.tm_mday;
      hh = ti.tm_hour; mi = ti.tm_min;
    }

    char dt_buf[64];
    snprintf(dt_buf, sizeof(dt_buf), "Date: %04d-%02d-%02d  Time: %02d:%02d", y, m, d, hh, mi);
    printer.println(dt_buf);

    char meta_buf[64];
    snprintf(meta_buf, sizeof(meta_buf), "Unit: mm  Saw Kerf: %.1f mm", (double)OPT.kerf_mm);
    printer.println(meta_buf);

    char stat_buf[64];
    snprintf(stat_buf, sizeof(stat_buf), "Total Bars: %d  Total Cuts: %d", OPT.bar_count, OPT.piece_count);
    printer.println(stat_buf);

    printer.println("--------------------------------");
    printer_send_size(0x01); // Double height for section header
    printer.println(doc_title);
    printer_send_size(0x00);
    printer.println("--------------------------------");
    printer_send_align(0); // Left align for list body
  }
}

void PrinterTask(void *pvParameters) {
  while (1) {
    if (IOS.opt_print_request) {
      IOS.opt_print_request = false;
      GUI.buzzer_cmd_short = true;

      const bool fa = (GUI.system_language == 1);
      printer_print_header(fa ? "[برنامه برش قطعات]" : "[REMAINING CUT PLAN]");

      int rem_n = 0;
      for (int i = 0; i < OPT.piece_count; i++) {
        if (!OPT.piece_done[i]) rem_n++;
      }

      float prof_len = STP.final_position_mm.toFloat();
      if (prof_len <= 0.0f) prof_len = 6000.0f;

      for (int b = 0; b < OPT.bar_count && b < OPT_MAX_BARS; b++) {
        bool has_rem = false;
        for (int i = 0; i < OPT.piece_count; i++) {
          if (OPT.piece_bar[i] == (uint16_t)b && !OPT.piece_done[i]) { has_rem = true; break; }
        }
        if (!has_rem) continue;

        int ang = (b < OPT_MAX_BARS && OPT.bar_angle[b] == 90) ? 90 : 45;
        char bar_title[64];
        if (fa) {
          snprintf(bar_title, sizeof(bar_title), "شاخه %d (%d mm - %d°):", b + 1, (int)prof_len, ang);
          printer_print_persian_bitmap(bar_title, &ui_font_naz20b, LV_TEXT_ALIGN_RIGHT, 26);
        } else {
          printer.print("Bar ");
          printer.print(b + 1);
          printer.print(" (");
          printer.print((int)prof_len);
          printer.print("mm - ");
          printer.print(ang);
          printer.println("°):");
        }

        int line_len = 2;
        printer.print("  ");

        for (int i = 0; i < OPT.piece_count; i++) {
          if (OPT.piece_bar[i] != (uint16_t)b) continue;
          if (OPT.piece_done[i]) continue; // فقط قطعات برش‌نخورده

          char num_str[16];
          float val = OPT.piece_len[i];
          float r = roundf(val * 10.0f) / 10.0f;
          if (fabsf(r - roundf(r)) < 0.05f) snprintf(num_str, sizeof(num_str), "%d", (int)lroundf(r));
          else snprintf(num_str, sizeof(num_str), "%.1f", (double)r);

          printer_print_token(num_str, line_len, 30, " - ");
        }
        printer.println();
        if (OPT.bar_waste[b] > 0) {
          char waste_str[32];
          if (fa) {
            snprintf(waste_str, sizeof(waste_str), "پرت: %d mm", (int)OPT.bar_waste[b]);
            printer_print_persian_bitmap(waste_str, &ui_font_naz18b, LV_TEXT_ALIGN_RIGHT, 22);
          } else {
            printer.print("  Waste: ");
            printer.print((int)OPT.bar_waste[b]);
            printer.println(" mm");
          }
        }
        printer.println();
        vTaskDelay(pdMS_TO_TICKS(5));
      }
      if (rem_n <= 0) {
        if (fa) printer_print_persian_bitmap("(تمام قطعات برش خوردند / تکمیل)", &ui_font_naz20, LV_TEXT_ALIGN_CENTER, 26);
        else printer.println("  (All pieces cut / Complete)");
      }
      printer.println("================================");
      printer_send_feed(1); // 1-line tear margin
    }

    if (IOS.opt_print_done_request) {
      IOS.opt_print_done_request = false;
      GUI.buzzer_cmd_short = true;

      const bool fa = (GUI.system_language == 1);
      printer_print_header(fa ? "[گزارش قطعات برش‌خورده]" : "[CUT SIZES HISTORY]");

      if (fa) {
        char cuts_cnt_str[32];
        snprintf(cuts_cnt_str, sizeof(cuts_cnt_str), "تعداد برش: %d", OPT.cut_log_n);
        printer_print_persian_bitmap(cuts_cnt_str, &ui_font_naz20, LV_TEXT_ALIGN_RIGHT, 24);
      } else {
        printer.print("Cut Pieces: ");
        printer.println(OPT.cut_log_n);
      }
      printer.println("--------------------------------");

      int i = 0;
      while (i < OPT.cut_log_n && i < OPT_CUT_LOG_MAX) {
        int16_t bar = OPT.cut_log_bar[i];
        int j = i + 1;
        while (j < OPT.cut_log_n && j < OPT_CUT_LOG_MAX && OPT.cut_log_bar[j] == bar) j++;
        if (bar >= 0) {
          if (fa) {
            char bar_hdr[32];
            snprintf(bar_hdr, sizeof(bar_hdr), "شاخه %d:", (int)bar + 1);
            printer_print_persian_bitmap(bar_hdr, &ui_font_naz20b, LV_TEXT_ALIGN_RIGHT, 24);
          } else {
            printer.print("Bar ");
            printer.print((int)bar + 1);
            printer.println(":");
          }
        } else {
          if (fa) printer_print_persian_bitmap("برش دستی:", &ui_font_naz20b, LV_TEXT_ALIGN_RIGHT, 24);
          else printer.println("Manual Cuts:");
        }

        int item_num = 1;
        for (int k = i; k < j; k++) {
          float v = OPT.cut_log_mm[k];
          float r = roundf(v * 10.0f) / 10.0f;
          char v_buf[16];
          if (fabsf(r - roundf(r)) < 0.05f) snprintf(v_buf, sizeof(v_buf), "%d", (int)lroundf(r));
          else snprintf(v_buf, sizeof(v_buf), "%.1f", (double)r);

          printer.print("  ");
          printer.print(item_num++);
          printer.print(". ");
          printer.print(v_buf);
          printer.println(" mm");
        }
        printer.println();
        i = j;
        vTaskDelay(pdMS_TO_TICKS(5));
      }
      if (OPT.cut_log_n <= 0) {
        if (fa) printer_print_persian_bitmap("(هنوز برشی ثبت نشده است)", &ui_font_naz20, LV_TEXT_ALIGN_CENTER, 26);
        else printer.println("  (No cuts logged yet)");
      }
      printer.println("================================");
      printer_send_feed(1); // 1-line tear margin
    }

    if (IOS.cut_print_request) {
      IOS.cut_print_request = false;
      GUI.buzzer_cmd_short = true;

      printer_send_init();
      printer_send_align(1);
      printer_send_size(0x77);

      vTaskDelay(pdMS_TO_TICKS(15));

      printer.println(buff_currentPosition_mm);
      printer_send_feed(1); // 1-line tear margin
    }

    if (printertest) {
      printertest = 0;
      GUI.buzzer_cmd_short = true;

      printer_send_init();
      const bool fa = (GUI.system_language == 1);

      if (fa) {
        printer_print_persian_bitmap("هافلر - برش تک‌سر", &ui_font_naz24, LV_TEXT_ALIGN_CENTER, 30);
        printer.println("================================");
        if (g_company_name.length() > 0) {
          char comp_buf[64];
          snprintf(comp_buf, sizeof(comp_buf), "کارگاه: %s", g_company_name.c_str());
          printer_print_persian_bitmap(comp_buf, &ui_font_naz20, LV_TEXT_ALIGN_RIGHT, 26);
        }
        printer_print_persian_bitmap("چاپگر: حرارتی CSN-A2 / ESC-POS", &ui_font_naz20, LV_TEXT_ALIGN_RIGHT, 26);
        printer_print_persian_bitmap("سرعت: ۱۹۲۰۰ 8N1", &ui_font_naz20, LV_TEXT_ALIGN_RIGHT, 26);
        printer_print_persian_bitmap("وضعیت: آماده به کار / تأیید", &ui_font_naz20, LV_TEXT_ALIGN_RIGHT, 26);
        printer.println("================================");
        printer_print_persian_bitmap("تست چاپ با موفقیت انجام شد", &ui_font_naz24, LV_TEXT_ALIGN_CENTER, 32);
        printer.println("================================");
      } else {
        printer_send_align(1);
        printer_send_size(0x11);
        printer.println("HAFFLER CUTTER");
        printer_send_size(0x00);
        printer.println("================================");
        if (g_company_name.length() > 0) {
          printer.print("Company: ");
          printer.println(g_company_name.c_str());
        }
        printer.println("Printer: CSN-A2 (5-9V TTL+USB)");
        printer.println("Baud: 19200 8N1");
        printer.println("Status: READY / OK");
        printer.println("================================");
        printer.println("Printer Test OK");
        printer.println("================================");
      }
      printer_send_feed(1); // 1-line tear margin
      Serial.println("Haffler Printer Test OK");
    }

    vTaskDelay(pdMS_TO_TICKS(15));
  }
}
