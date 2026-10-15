// ایمپورت لیست سایزها از پورت سریال صفر (UART0 → CP2102)
// ─────────────────────────────────────────────────────────────────────────────
// سناریو: کاربر از کامپیوتر (یا ماژول فایل‌خوان UART مثل CH376) متن سایزها را
// روی UART0 با baud 115200 می‌فرستد. فرمت هر جفت: «متراژXتعداد» جداشده با , یا خط جدید:
//     2500X10,1800X5
//     900X4
// بعد از 400ms سکوت، فریم پارس می‌شود؛ اگر معتبر بود دیالوگ تأیید روی HMI می‌آید؛
// با YES سایزها وارد لیست می‌شوند (از ردیف 1، تعداد ردیف فعال = تعداد جفت‌ها).
// توجه: CP2102 فقط پل USB↔UART است — فلش USB را مستقیم نمی‌خواند؛ برای فلش مستقیم
// ماژول USB-host مثل CH376 لازم است (خروجی همین پروتکل را بدهد، کار می‌کند).
// نکته: چون ARDUINO_USB_CDC_ON_BOOT=1 است، «Serial» = USB-CDC و «Serial0» = UART0 (پایه‌های TX0/RX0).
#pragma once

#ifndef SERIAL_DEBUG_BAUD
#define SERIAL_DEBUG_BAUD 921600
#endif
#define SERIAL_IMPORT_BAUD SERIAL_DEBUG_BAUD
#define SERIAL_IMPORT_IDLE_MS 400
#define SERIAL_IMPORT_BUF 1600
// 115200 baud carries at most about 231 bytes per 20ms (8N1).  A 256-byte
// snapshot budget drains line rate without allowing a continuously-fed UART to
// monopolize this core indefinitely.
#define SERIAL_IMPORT_RX_BUDGET_BYTES 256U

// احراز هویت سرویس UART — چالش-پاسخ (Challenge-Response) با SHA-256:
// رمز مدیر (admin_pin) هرگز به صورت متن ساده روی سیم نمی‌رود و ضبط/بازپخش فریم بی‌اثر است.
//   ۱) PC می‌فرستد:  AUTH?          → دستگاه پاسخ می‌دهد: NONCE=<16 hex>
//   ۲) PC می‌فرستد:  AUTH=<16 hex اول SHA-256(رشته NONCE + رمز مدیر)>
//      درست بودن ⇒ پنجرهٔ سرویس ۶۰ ثانیه‌ای باز می‌شود و nonce تازه می‌شود (ضد replay).
// همهٔ دستورات نوشتنی (SAVE, BON, BOFF, OPT, RBT, FRST, ایمپورت خام لیست) فقط داخل پنجرهٔ
// سرویس اجرا می‌شوند؛ دستورات خواندنی/دیباگ (SHOT, MEM, PERF, QSTA, SCR, TAB, UPD, QH…) بدون
// احراز کار می‌کنند. ۵ پاسخ غلط پیاپی ⇒ ۶۰ ثانیه قفل سرویس سریال (مستقل از قفل صفحه).
#ifndef UART_SVC_AUTH_SHARED
#define UART_SVC_AUTH_SHARED
#include <mbedtls/sha256.h>

static uint8_t uart_svc_fails = 0;
static unsigned long uart_svc_lock_until = 0;
static unsigned long uart_svc_window_until = 0;  // پایان پنجرهٔ سرویس باز
static char uart_svc_nonce[17];                  // ۸ بایت تصادفی به صورت ۱۶ نویسه hex
static bool uart_svc_nonce_ready = false;

static void uart_svc_new_nonce() {
  uint32_t r[2] = { esp_random(), esp_random() };
  static const char hexd[] = "0123456789abcdef";
  const uint8_t *b = (const uint8_t *)r;
  for (int i = 0; i < 8; i++) {
    uart_svc_nonce[i * 2] = hexd[b[i] >> 4];
    uart_svc_nonce[i * 2 + 1] = hexd[b[i] & 0x0F];
  }
  uart_svc_nonce[16] = 0;
  uart_svc_nonce_ready = true;
}

static bool uart_svc_window_open() {
  return millis() < uart_svc_window_until;
}

// تأیید پاسخ: انتظار = ۱۶ هکس اولِ SHA-256(nonce_hex + admin_pin)
static bool uart_svc_verify(const char *hex_resp) {
  if (!uart_svc_nonce_ready) uart_svc_new_nonce();
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, (const unsigned char *)uart_svc_nonce, 16);
  mbedtls_sha256_update(&ctx, (const unsigned char *)admin_pin.c_str(), admin_pin.length());
  uint8_t digest[32];
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  char expected[17];
  static const char hexd[] = "0123456789abcdef";
  for (int i = 0; i < 8; i++) {
    expected[i * 2] = hexd[digest[i] >> 4];
    expected[i * 2 + 1] = hexd[digest[i] & 0x0F];
  }
  expected[16] = 0;
  return strncmp(hex_resp, expected, 16) == 0;
}

// پردازش فرمان AUTH/AUTH? داخل فریم؛ true یعنی فریم مصرف شد.
static bool uart_svc_auth_frame(char *buf) {
  if (strstr(buf, "AUTH?") != NULL) {
    if (!uart_svc_nonce_ready) uart_svc_new_nonce();
    Serial0.printf("NONCE=%s\n", uart_svc_nonce);
    return true;
  }
  char *p = strstr(buf, "AUTH=");
  if (p != NULL) {
    if (millis() < uart_svc_lock_until) {
      Serial0.println("ERR: service locked, wait 60s");
      return true;
    }
    p += 5;
    char resp[17];
    int n = 0;
    while (p[n] && n < 16 && isxdigit((int)((unsigned char)p[n]))) { resp[n] = p[n]; n++; }
    resp[n] = 0;
    bool ok = (n == 16) && uart_svc_verify(resp);
    if (ok) {
      uart_svc_fails = 0;
      uart_svc_window_until = millis() + 60000UL;
      uart_svc_new_nonce();  // هر توکن فقط یک بار قابل استفاده است
      Serial0.println("OK svc window 60s");
    } else {
      if (++uart_svc_fails >= 5) {
        uart_svc_lock_until = millis() + 60000UL;
        uart_svc_fails = 0;
        Serial0.println("ERR: service locked, wait 60s");
      } else {
        Serial0.println("ERR: bad AUTH response");
      }
    }
    return true;
  }
  if (strstr(buf, "SVCKEY=") != NULL) {
    // کلید متنی ساده حذف شده است — رمز هرگز روی سیم ارسال نمی‌شود.
    Serial0.println("ERR: SVCKEY removed — use AUTH? / AUTH=<sha256>");
    return true;
  }
  return false;
}

static bool uart_svc_auth(const char *buf) {
  (void)buf;
  return uart_svc_window_open();
}

static void uart_svc_deny(const char *cmd) {
  Serial0.printf("ERR: auth required — AUTH? then AUTH=<hash> before %s (60s window)\n", cmd);
}
#endif

static char si_buf[SERIAL_IMPORT_BUF];
static int si_len = 0;
static unsigned long si_last_rx = 0;
// دادهٔ آماده برای تأیید (فقط وقتی flag ست است معتبر است؛ تا مصرف دیالوگ دست نمی‌خورد)
static int si_pending_len[LIST_MAX];
static int si_pending_cnt[LIST_MAX];
static volatile int si_pending_n = 0;
static volatile bool si_dialog_busy = false;  // تا بستن دیالوگ، فریم جدید پارس نشود

static void serial_import_parse() {
  si_buf[si_len] = 0;
  if (uart_svc_auth_frame(si_buf)) { si_len = 0; return; }  // چالش-پاسخ سرویس
  if (strncmp(si_buf, "MEM", 3) == 0) {
    si_len = 0;
    shot_report_stack_watermarks(&Serial0);
    return;
  }
  if (strstr(si_buf, "SHOT") != NULL) {  // فرمان اسکرین‌شات از PC (COM4)
    si_len = 0;
    shot_request(&Serial0);
    return;
  }
  if (strstr(si_buf, "S_WAR") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 1;
    Serial0.println("ACK_WAR");
    return;
  }
  if (strstr(si_buf, "S_VOL") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 2;
    Serial0.println("ACK_VOL");
    return;
  }
  if (strstr(si_buf, "S_OTA") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 3;
    Serial0.println("ACK_OTA");
    return;
  }
  if (strstr(si_buf, "S_CLS") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 4;
    Serial0.println("ACK_CLS");
    return;
  }
  if (strstr(si_buf, "S_UPD") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 5;
    Serial0.println("ACK_UPD");
    return;
  }
  if (strstr(si_buf, "S_F1") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 6;
    Serial0.println("ACK_F1");
    return;
  }
  if (strstr(si_buf, "S_F2") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 7;
    Serial0.println("ACK_F2");
    return;
  }
  if (strstr(si_buf, "S_OOBE_P1") != NULL || strstr(si_buf, "S_OOBE_1") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 8;
    Serial0.println("ACK_OOBE_P1");
    return;
  }
  if (strstr(si_buf, "S_OOBE_P2") != NULL || strstr(si_buf, "S_OOBE_2") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 9;
    Serial0.println("ACK_OOBE_P2");
    return;
  }
  if (strstr(si_buf, "S_OOBE_P3") != NULL || strstr(si_buf, "S_OOBE_3") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 10;
    Serial0.println("ACK_OOBE_P3");
    return;
  }
  if (strstr(si_buf, "S_OOBE_RST") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 11;
    Serial0.println("ACK_OOBE_RST");
    return;
  }
  if (strstr(si_buf, "S_OOBE_VER") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 17;
    Serial0.println("ACK_OOBE_VER");
    return;
  }
  if (strstr(si_buf, "S_SPL0") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 12;
    Serial0.println("ACK_SPL0");
    return;
  }
  if (strstr(si_buf, "S_SPL1") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 13;
    Serial0.println("ACK_SPL1");
    return;
  }
  if (strstr(si_buf, "S_SCR0") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 14;
    Serial0.println("ACK_SCR0");
    return;
  }
  if (strstr(si_buf, "S_SCR2") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 15;
    Serial0.println("ACK_SCR2");
    return;
  }
  if (strstr(si_buf, "S_SCR3") != NULL) {
    si_len = 0;
    g_remote_ui_cmd = 16;
    Serial0.println("ACK_SCR3");
    return;
  }
  if (strstr(si_buf, "LCDB") != NULL) {
    si_len = 0;
    lcd_bench_request(&Serial0);
    Serial0.println("OK LCDB");
    return;
  }
  if (strstr(si_buf, "PERF") != NULL) {
    si_len = 0;
    perf_qa_start(&Serial0);
    Serial0.println("OK PERF");
    return;
  }
  if (strstr(si_buf, "Q_FAULTS") != NULL) {
    si_len = 0;
    int fc = haffler_fault_log_get_count();
    Serial0.printf("FAULTS_BEGIN count=%d\n", fc);
    for (int i = 0; i < fc; i++) {
      haffler_fault_entry_t fe;
      if (haffler_fault_log_get_entry(i, &fe)) {
        Serial0.printf("[%d] code=%u val=%d time=%u desc=%s\n",
                       i, (unsigned)fe.code, (int)fe.value, (unsigned)fe.timestamp,
                       haffler_fault_code_to_str((haffler_fault_code_t)fe.code));
      }
    }
    Serial0.println("FAULTS_END");
    return;
  }
  if (strstr(si_buf, "CLR_FAULTS") != NULL) {
    si_len = 0;
    haffler_fault_log_clear();
    Serial0.println("OK CLR_FAULTS");
    return;
  }
  const char *scr_cmd = strstr(si_buf, "SCR");
  if (scr_cmd != NULL && scr_cmd[3] >= '1' && scr_cmd[3] <= '3') {  // ناوبری ریموت برای دیباگ
    GUI.pending_goto_scr = scr_cmd[3] - '0';
    Serial0.printf("OK SCR%c\n", scr_cmd[3]);
    si_len = 0;
    return;
  }
  const char *tab_cmd = strstr(si_buf, "TAB");
  if (tab_cmd != NULL && tab_cmd[3] >= '1' && tab_cmd[3] <= '5') {  // تبِ تنظیمات F1..F5 (بعد از SCR2)
    GUI.tab_view_select = tab_cmd[3] - '0';
    Serial0.printf("OK TAB%c\n", tab_cmd[3]);
    si_len = 0;
    return;
  }
  const char *upd_cmd = strstr(si_buf, "UPD");
  if (upd_cmd != NULL && upd_cmd[3] >= '1' && upd_cmd[3] <= '5') {  // دیباگِ جریانِ بروزرسانی
    shot_uf_dbg = upd_cmd[3] - '0';
    Serial0.printf("OK UPD%c\n", upd_cmd[3]);
    si_len = 0;
    return;
  }
  const char *qh_cmd = strstr(si_buf, "QH");
  if (qh_cmd != NULL && qh_cmd[2] >= '1' && qh_cmd[2] <= '5') {
    qa_ui_action = qh_cmd[2] - '0';
    Serial0.printf("OK QH%c\n", qh_cmd[2]);
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "QLH") != NULL) {
    qa_ui_action = 6;
    Serial0.println("OK QLH");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "QOH") != NULL) {
    qa_ui_action = 7;
    Serial0.println("OK QOH");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "QF2") != NULL) {
    qa_ui_action = 8;
    Serial0.println("OK QF2");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "QDL") != NULL) {
    qa_ui_action = 9;
    Serial0.println("OK QDL");
    si_len = 0;
    return;
  }
  if (strcmp(si_buf, "QST") == 0) {
    qa_ui_action = 10;
    Serial0.println("OK QST");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "LFA") != NULL) {
    GUI.system_language = 1;
    PEND.system_language = 1;
    GUI.pending_lang_selected = 0;
    GUI.flag_apply_language = true;
    Serial0.println("OK LFA");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "LEN") != NULL) {
    GUI.system_language = 2;
    PEND.system_language = 2;
    GUI.pending_lang_selected = 1;
    GUI.flag_apply_language = true;
    Serial0.println("OK LEN");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "SAVE") != NULL) {
    if (!uart_svc_auth(si_buf)) { uart_svc_deny("SAVE"); si_len = 0; return; }
    load_pending_from_current();
    GUI.flag_save_all = true;
    Serial0.println("OK SAVE");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "BOFF") != NULL) {
    if (!uart_svc_auth(si_buf)) { uart_svc_deny("BOFF"); si_len = 0; return; }
    GUI.buzzer_enable = false;
    PEND.buzzer_enable = false;
    GUI.flag_save_all = true;
    Serial0.println("OK BOFF");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "BON") != NULL) {
    if (!uart_svc_auth(si_buf)) { uart_svc_deny("BON"); si_len = 0; return; }
    GUI.buzzer_enable = true;
    PEND.buzzer_enable = true;
    GUI.flag_save_all = true;
    Serial0.println("OK BON");
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "QSTA") != NULL) {
    Serial0.printf("QSTA scr=%d lang=%d speed=%d init=%d stock=%d kerf=%.1f rows=%d kb=%d buz=%d\n",
                   GUI.scr_id, GUI.system_language, STP.global_speed, STP.global_init_speed,
                   stock_len_mm, (double)kerf_mm, GUI.visible_raws,
                   GUI.virtual_kb_enable ? 1 : 0, GUI.buzzer_enable ? 1 : 0);
    si_len = 0;
    return;
  }
  if (strstr(si_buf, "RBT") != NULL) {
    if (!uart_svc_auth(si_buf)) { uart_svc_deny("RBT"); si_len = 0; return; }
    Serial0.println("OK RBT");
    Serial0.flush();
    si_len = 0;
    delay(100);
    ESP.restart();
    return;
  }
  if (strstr(si_buf, "FRST") != NULL) {
    if (!uart_svc_auth(si_buf)) { uart_svc_deny("FRST"); si_len = 0; return; }
    Serial0.println("OK FRST");
    Serial0.flush();
    si_len = 0;
    delay(100);
    factory_reset();
    return;
  }
  if (strstr(si_buf, "OPT") != NULL) {
    if (!uart_svc_auth(si_buf)) { uart_svc_deny("OPT"); si_len = 0; return; }
    opt_request_remote_open();
    Serial0.println("OK OPT");
    si_len = 0;
    return;
  }
  // ایمپورت خام لیست سایزها هم دستور نوشتنی است — بدون کلید سرویس پارس نمی‌شود
  if (!uart_svc_auth(si_buf)) { uart_svc_deny("LIST"); si_len = 0; return; }
  int n = 0;
  int i = 0;
  while (si_buf[i] && n < LIST_MAX) {
    // پرش از جداکننده‌ها
    while (si_buf[i] && !(si_buf[i] >= '0' && si_buf[i] <= '9')) i++;
    if (!si_buf[i]) break;
    long len = 0;
    while (si_buf[i] >= '0' && si_buf[i] <= '9') { len = len * 10 + (si_buf[i] - '0'); i++; }
    while (si_buf[i] == ' ' || si_buf[i] == '\t') i++;
    if (si_buf[i] != 'X' && si_buf[i] != 'x' && si_buf[i] != '*') continue;  // جفت ناقص — رد
    i++;
    while (si_buf[i] == ' ' || si_buf[i] == '\t') i++;
    if (!(si_buf[i] >= '0' && si_buf[i] <= '9')) continue;
    long cnt = 0;
    while (si_buf[i] >= '0' && si_buf[i] <= '9') { cnt = cnt * 10 + (si_buf[i] - '0'); i++; }
    if (len <= 0 || cnt <= 0) continue;
    si_pending_len[n] = (int)len;
    si_pending_cnt[n] = (int)constrain(cnt, 0L, 999L);
    n++;
  }
  si_len = 0;
  if (n == 0) {
    Serial0.println("ERR: no valid LENGTHxCOUNT pairs");
    return;
  }
  si_pending_n = n;
  si_dialog_busy = true;
  GUI.flag_serial_import_dialog = true;  // Task2 دیالوگ تأیید می‌سازد
  GUI.buzzer_cmd_short = true;
  Serial0.printf("OK: %d pairs — confirm on HMI\n", n);
}

void SerialImportTask(void *pvParameters) {
  vTaskDelay(700 / portTICK_PERIOD_MS);
  shot_uart0_owned = true;  // از این به بعد این تسک تنها خوانندهٔ UART0 است
  Serial0.begin(SERIAL_IMPORT_BAUD);
  Serial0.println("Metr&Cutter serial import ready (LENGTHxCOUNT,... )");
  while (1) {
    for (uint16_t rx_count = 0;
         rx_count < SERIAL_IMPORT_RX_BUDGET_BYTES && Serial0.available();
         ++rx_count) {
      char c = (char)Serial0.read();
      si_last_rx = millis();
      if (si_dialog_busy) continue;  // تا تعیین تکلیف دیالوگ، ورودی جدید دور ریخته می‌شود
      if (si_len < SERIAL_IMPORT_BUF - 1) si_buf[si_len++] = c;
    }
    if (si_len > 0 && !si_dialog_busy && (millis() - si_last_rx) > SERIAL_IMPORT_IDLE_MS) {
      serial_import_parse();
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// ─── اعمال دادهٔ تأییدشده (فقط از کانتکست Task2/LVGL صدا زده شود) ───
void serial_import_apply() {
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  int initial_mm = (int)round((float)STP.initial_Position_p / ppmm);
  int final_mm = STP.final_position_mm.toInt();
  int n = si_pending_n;
  bool clamped = false;
  for (int i = 0; i < n && i < LIST_MAX; i++) {
    int v = si_pending_len[i];
    if (v < initial_mm) { v = initial_mm; clamped = true; }
    if (v > final_mm) { v = final_mm; clamped = true; }
    AUT.list_sp_Val_p[i] = (float)v;
    GUI.inputBuffer_L_3_1_x[i] = String(v);
    save_permanently_preferences_sp_val(i, AUT.list_sp_Val_p[i]);
    AUT.list_count_Val[i] = si_pending_cnt[i];
    GUI.inputBuffer_L_3_2_x[i] = String(si_pending_cnt[i]);
    save_permanently_preferences_count_val(i, si_pending_cnt[i]);
  }
  GUI.visible_raws = n;
  save_visible_raws();
  GUI.list_page = 0;
  GUI.flag_update_labels_3_1 = true;
  GUI.flag_update_labels_3_2 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  GUI.flag_refresh_visible = true;
  GUI.flag_update_page = true;
  char fa[72], en[64];
  snprintf(fa, sizeof(fa), "%d ردیف وارد لیست شد%s", n, clamped ? " (با اصلاح محدوده)" : "");
  snprintf(en, sizeof(en), "%d rows imported%s", n, clamped ? " (range-clamped)" : "");
  ui_toast(fa, en, 3500);
  Serial0.println("IMPORTED");
}
