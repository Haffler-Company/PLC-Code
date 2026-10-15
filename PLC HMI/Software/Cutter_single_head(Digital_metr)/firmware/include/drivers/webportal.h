// Web setup portal + OTA firmware update (F1–F5 web app)
// F1 WiFi chip toggles a password-protected SoftAP. Captive → http://192.168.4.1
// SSID: "Haffler S-Head Cutter V" + PROJECT_VERSION_STRING (≤32 chars)
// Safety: settings/list/OTA writes rejected while machine busy.
#pragma once
#include <Update.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <math.h>
#include "screen_server_page.h"

extern char ap_ssid[33];
extern volatile bool web_portal_start_request;
extern volatile bool web_portal_stop_request;
extern volatile bool web_portal_running;
extern volatile bool web_portal_transitioning;
extern volatile bool web_portal_ota_active;
extern volatile int8_t web_portal_ap_result;

enum PortalWifiMode : uint8_t { PORTAL_WIFI_DIRECT = 0, PORTAL_WIFI_STA = 1 };
static volatile uint8_t portal_start_mode = PORTAL_WIFI_DIRECT;
static volatile uint8_t portal_active_mode = PORTAL_WIFI_DIRECT;
static bool portal_mdns_started = false;

// [UI] The F1 callbacks only queue work; WebPortalTask is the radio owner.
bool web_portal_request_direct(void) {
  portal_start_mode = PORTAL_WIFI_DIRECT;
  web_portal_stop_request = false;
  web_portal_start_request = true;
  return startWebPortalTask();
}
void update_flow_start_portal_sta();
void web_portal_request_sta_setup(void) { update_flow_start_portal_sta(); }
bool web_portal_is_sta(void) { return portal_active_mode == PORTAL_WIFI_STA; }
void web_portal_sta_ip(char *out, size_t out_sz) {
  if (!out || out_sz == 0) return;
  const String ip = WiFi.localIP().toString();
  strncpy(out, ip.c_str(), out_sz - 1);
  out[out_sz - 1] = '\0';
}
void web_portal_start_sta_on_connected(void) {
  portal_start_mode = PORTAL_WIFI_STA;
  web_portal_stop_request = false;
  web_portal_start_request = true;
  if (!startWebPortalTask()) {
    web_portal_start_request = false;
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
    wifi_owner_release(WIFI_OWNER_PORTAL);
    web_portal_ap_result = -3;
  }
}

static DNSServer portalDns;

static bool portal_machine_busy() {
  return STP.motorIs_moving || STP.moveStepperCMD || STP.jogEnabled ||
         keyboardJogActive || STP.goref_cmd || STP.go_sampling_CMD ||
         STP.begin_calibration ||
         STP.flag_is_cutting || IOS.saw_is_on || IOS.clamp_engaged ||
         IOS.cut_confirm_pending || IOS.both_start_button_state ||
         OPT.active || opt_calc_active ||
         f3_stepper_test_cmd || f3_stepper_test_busy ||
         device_locked || sensor_alert_motion_lock ||
         critical_task_start_failed || GUI.lock_screen ||
         IOS.stop_button_state;
}

static void portal_captive_redirect() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.sendHeader("Connection", "close");
  server.send(302, "text/plain", "");
}

static void portal_captive_probe() {
  portal_captive_redirect();
}

static void portal_send_index() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=UTF-8", "");

  const char *ptr = SCREEN_SERVER_PAGE;
  size_t total = strlen_P(SCREEN_SERVER_PAGE);
  size_t sent = 0;
  const size_t chunkSize = 2048; // 2KB chunks for stable WiFi SoftAP transmission

  while (sent < total && server.client().connected()) {
    size_t len = total - sent;
    if (len > chunkSize) len = chunkSize;
    server.sendContent_P(ptr + sent, len);
    sent += len;
    vTaskDelay(1);
  }
  server.sendContent("");
}

static int portal_start_mm() {
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm < 0.001f) ppmm = 1.0f;
  return (int)round((float)STP.initial_Position_p / ppmm);
}

static void portal_api_settings_get() {
  int spd_pct = map(STP.global_speed, 100, 8, 0, 100);
  int jspd_pct = map(jog_speed_interval, 100, 8, 0, 100);
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm < 0.001f) ppmm = 1.0f;
  const float current_mm = (float)STP.currentPosition_p / ppmm;
  const int active_row =
      (AUT.selected_count_val >= 0 && AUT.selected_count_val < LIST_MAX)
          ? AUT.selected_count_val + 1
          : 0;
  const int remaining = active_row > 0 ? AUT.temp_list_count_Val : -1;
  const char *mode = GUI.NumLock ? "jog"
                     : OPT.active ? "opt"
                     : active_row > 0 ? "list"
                                      : "manual";
  const int64_t clock_epoch = soft_now_epoch();
  const bool clock_valid = (clock_epoch > 0);
  String j;
  j.reserve(2048);  // Fixed response shape: full read-only I/O avoids repeated growth.
  char current_mm_text[16];
  char destination_mm_text[16];
  char kerf_mm_text[16];
  char clock_epoch_text[24];
  snprintf(current_mm_text, sizeof(current_mm_text), "%.1f", (double)current_mm);
  snprintf(destination_mm_text, sizeof(destination_mm_text), "%.1f", (double)STP.Man_Sp_mm);
  snprintf(kerf_mm_text, sizeof(kerf_mm_text), "%.1f", (double)kerf_mm);
  snprintf(clock_epoch_text, sizeof(clock_epoch_text), "%lld", (long long)clock_epoch);
  j = "{";
  j += "\"ver\":\"" PROJECT_VERSION_DISPLAY "\"";
  j += ",\"ssid\":\""; j += ap_ssid; j += "\"";
  j += ",\"wifi_mode\":\""; j += (portal_active_mode == PORTAL_WIFI_STA ? "sta" : "direct"); j += "\"";
  j += ",\"ip\":\""; j += (portal_active_mode == PORTAL_WIFI_STA ? WiFi.localIP().toString() : WiFi.softAPIP().toString()); j += "\"";
  j += ",\"host\":\""; j += (portal_active_mode == PORTAL_WIFI_STA ? "http://haffler.local" : "http://192.168.4.1"); j += "\"";
  j += ",\"tcuts\":"; j += total_cuts;
  j += ",\"ucuts\":"; j += user_cuts;
  j += ",\"cuts_life_manual\":"; j += cuts_life_manual;
  j += ",\"cuts_life_list\":"; j += cuts_life_list;
  j += ",\"cuts_life_opt\":"; j += cuts_life_opt;
  j += ",\"cuts_day_manual\":"; j += cuts_day_manual;
  j += ",\"cuts_day_list\":"; j += cuts_day_list;
  j += ",\"cuts_day_opt\":"; j += cuts_day_opt;
  j += ",\"cuts_day_key\":"; j += cuts_day_key;
  j += ",\"clock_valid\":"; j += (clock_valid ? 1 : 0);
  j += ",\"clock_epoch\":"; j += clock_epoch_text;
  j += ",\"heap\":"; j += ESP.getFreeHeap();
  j += ",\"uptime\":"; j += (uint32_t)(millis() / 1000UL);
  j += ",\"cur\":"; j += current_mm_text;
  j += ",\"dest\":"; j += destination_mm_text;
  j += ",\"mode\":\""; j += mode; j += "\"";
  j += ",\"row\":"; j += active_row;
  j += ",\"remaining\":"; j += remaining;
  j += ",\"moving\":"; j += ((STP.motorIs_moving || STP.moveStepperCMD) ? 1 : 0);
  j += ",\"cutting\":"; j += (STP.flag_is_cutting ? 1 : 0);
  j += ",\"saw\":"; j += (IOS.saw_is_on ? 1 : 0);
  j += ",\"ref\":"; j += (STP.referenced ? 1 : 0);
  j += ",\"stock\":"; j += stock_len_mm;
  j += ",\"kerf\":"; j += kerf_mm_text;
  j += ",\"trim\":"; j += trim_mm;
  j += ",\"lang\":"; j += GUI.system_language;
  j += ",\"vkb\":"; j += (int)GUI.vkb_mode;
  j += ",\"buz\":"; j += (GUI.buzzer_enable ? 1 : 0);
  j += ",\"estop\":"; j += (stopstate ? 1 : 0);
  j += ",\"startmm\":"; j += portal_start_mm();
  j += ",\"maxlen\":"; j += STP.final_position_mm;
  j += ",\"spd\":"; j += spd_pct;
  j += ",\"aramp\":"; j += auto_ramp_ms;
  j += ",\"jspd\":"; j += jspd_pct;
  j += ",\"jramp\":"; j += jog_ramp_ms;
  j += ",\"jinit\":"; j += jog_init_speed;
  j += ",\"ctimer\":"; j += IOS.cut_prcs_timer / 1000;
  j += ",\"bdir\":"; j += STP.system_base_dir;
  j += ",\"dayrem\":"; j += day_lock_remain;
  j += ",\"i_ref\":"; j += (IOS.ref_sensor_state ? 1 : 0);
  j += ",\"i_up\":"; j += (IOS.up_lim_sensor_state ? 1 : 0);
  j += ",\"i_stop\":"; j += (IOS.stop_button_state ? 1 : 0);
  j += ",\"i_s1\":"; j += (IOS.start1_button_state ? 1 : 0);
  j += ",\"i_s2\":"; j += (IOS.start2_button_state ? 1 : 0);
  j += ",\"i_saw\":"; j += (IOS.saw_is_on ? 1 : 0);
  // Complete mapped I/O snapshot for F3 diagnostics. This endpoint is read-only:
  // no web handler may drive an output or change the pin map.
  const bool input_state[10] = {
      IOS.ref_sensor_state, IOS.up_lim_sensor_state, IOS.guard_switch_state,
      IOS.stop_button_state, IOS.blade_return_state, IOS.motor_fault_state,
      IOS.start1_button_state, IOS.start2_button_state,
      IOS.joystick_state_R, IOS.joystick_state_L};
  const bool output_state[10] = {
      (STP.motorIs_moving || STP.moveStepperCMD),
      pinmap_out_read(IOS.dir_pin) == HIGH,
      pinmap_out_read(IOS.enb_pin) == LOW,
      pinmap_out_read(IOS.u_clip_pin) == LOW,
      pinmap_out_read(IOS.f_clip_pin) == LOW,
      pinmap_out_read(IOS.spray_pin) == LOW,
      pinmap_out_read(IOS.saw_pin) == LOW,
      pinmap_out_read(IOS.lamp_pin) == LOW,
      pinmap_out_read(IOS.sawl_ift_pin) == LOW,
      pinmap_out_read(IOS.q9_pin) == LOW};
  for (int role = 0; role < 10; ++role) {
    j += ",\"i"; j += role; j += "\":"; j += (input_state[role] ? 1 : 0);
    j += ",\"ci"; j += role; j += "\":";
    j += (pinmap_in_role_connected(role) ? 1 : 0);
    j += ",\"q"; j += role; j += "\":"; j += (output_state[role] ? 1 : 0);
    j += ",\"cq"; j += role; j += "\":";
    j += (pinmap_out_get_source(role) != PINMAP_OUT_NC ? 1 : 0);
  }
  j += "}";
  server.send(200, "application/json", j);
}

static void portal_api_get() {
  portal_api_settings_get();
}

static void portal_api_list_get() {
  String j;
  j.reserve(2200);
  j = "{\"rows\":";
  j += GUI.visible_raws;
  j += ",\"sp\":[";
  for (int i = 0; i < LIST_MAX; i++) {
    float sp = AUT.list_sp_Val_p[i];
    char b[16];
    float r = roundf(sp * 10.0f) / 10.0f;
    if (fabsf(r - roundf(r)) < 0.05f) snprintf(b, sizeof(b), "%d", (int)lroundf(r));
    else snprintf(b, sizeof(b), "%.1f", (double)r);
    j += b;
    j += (i < LIST_MAX - 1) ? "," : "],\"cnt\":[";
  }
  for (int i = 0; i < LIST_MAX; i++) {
    j += AUT.list_count_Val[i];
    j += (i < LIST_MAX - 1) ? "," : "],\"ang\":[";
  }
  for (int i = 0; i < LIST_MAX; i++) {
    uint8_t a = AUT.list_angle_Val[i];
    if (a != 90) a = 45;
    j += a;
    j += (i < LIST_MAX - 1) ? "," : "]}";
  }
  server.send(200, "application/json", j);
}

// [WebPortalTask] Read-only stream of the last completed optimizer result.
// OPT is committed only after calculation completes; stream fixed-size numeric
// fragments so a full 5000-piece plan never requires one large heap String.
static void portal_optimized_send_mm(float value) {
  char text[24];
  if (!isfinite(value)) {
    server.sendContent("0");
    return;
  }
  const float rounded = roundf(value * 10.0f) / 10.0f;
  if (fabsf(rounded - roundf(rounded)) < 0.05f)
    snprintf(text, sizeof(text), "%d", (int)lroundf(rounded));
  else
    snprintf(text, sizeof(text), "%.1f", (double)rounded);
  server.sendContent(text);
}

static void portal_api_optimized_get() {
  const int piece_count = OPT.piece_count;
  const int bar_count = OPT.bar_count;
  const bool complete = opt_plan_ready && !opt_calc_active &&
                        piece_count > 0 && piece_count <= OPT_MAX_PIECES &&
                        bar_count > 0 && bar_count <= OPT_MAX_BARS;

  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"complete\":");
  server.sendContent(complete ? "true" : "false");
  server.sendContent(",\"plan_id\":");
  server.sendContent(complete ? "\"device-optimized\"" : "\"\"");
  server.sendContent(",\"stock_mm\":");
  portal_optimized_send_mm((float)stock_len_mm);
  server.sendContent(",\"trim_mm\":");
  portal_optimized_send_mm((float)trim_mm);
  server.sendContent(",\"kerf_mm\":");
  portal_optimized_send_mm(kerf_mm);
  server.sendContent(",\"bars\":[");

  float total_waste_mm = 0.0f;
  if (complete) {
    int piece = 0;
    for (int bar = 0; bar < bar_count; bar++) {
      if (bar > 0) server.sendContent(",");
      server.sendContent("{\"bar\":");
      char bar_text[12];
      snprintf(bar_text, sizeof(bar_text), "%d", bar + 1);
      server.sendContent(bar_text);
      server.sendContent(",\"pieces_mm\":[");
      bool first_piece = true;
      while (piece < piece_count && OPT.piece_bar[piece] == (uint16_t)bar) {
        if (!first_piece) server.sendContent(",");
        portal_optimized_send_mm(OPT.piece_len[piece]);
        first_piece = false;
        piece++;
      }
      server.sendContent("],\"waste_mm\":");
      portal_optimized_send_mm(OPT.bar_waste[bar]);
      server.sendContent("}");
      total_waste_mm += OPT.bar_waste[bar];
    }
  }

  server.sendContent("],\"total_waste_mm\":");
  portal_optimized_send_mm(total_waste_mm);
  server.sendContent(",\"unallocated\":[]}");
  server.sendContent("");
}

// Owner decision WEB-PIN-REMOVAL-01: web saves, lists, counters, and the
// screenshot download are password-free; only /update OTA authenticates once
// against the LCD manager PIN. The 409 machine/maintenance guards still apply.
static bool portal_check_write_allowed() {
  if (web_portal_ota_active || g_update_mode || update_flow_network_busy()) {
    server.send(409, "text/plain; charset=UTF-8", "REJECTED: maintenance busy");
    return false;
  }
  if (portal_machine_busy()) {
    server.send(409, "text/plain; charset=UTF-8", "REJECTED: machine busy");
    return false;
  }
  return true;
}

static inline void portal_bmp_le16(uint8_t *dst, uint16_t value) {
  dst[0] = (uint8_t)(value & 0xFFU);
  dst[1] = (uint8_t)(value >> 8);
}

static inline void portal_bmp_le32(uint8_t *dst, uint32_t value) {
  dst[0] = (uint8_t)(value & 0xFFU);
  dst[1] = (uint8_t)((value >> 8) & 0xFFU);
  dst[2] = (uint8_t)((value >> 16) & 0xFFU);
  dst[3] = (uint8_t)(value >> 24);
}

// [WebPortalTask] Bounded bridge to the Task2-owned LVGL frame.
// The captured RGB565 buffer is immutable after claim and is streamed as a
// standard 24-bit BMP so Android/iOS can save it without a decoder library.
static void portal_api_screen_bmp() {
  if (web_portal_ota_active || g_update_mode || update_flow_network_busy()) {
    server.send(409, "text/plain; charset=UTF-8", "REJECTED: maintenance busy");
    return;
  }
  if (portal_machine_busy()) {
    server.send(409, "text/plain; charset=UTF-8", "REJECTED: machine busy");
    return;
  }
  if (!shot_request_web()) {
    server.send(409, "text/plain; charset=UTF-8", "REJECTED: screenshot busy");
    return;
  }

  const uint16_t *frame = nullptr;
  const uint32_t wait_started_ms = millis();
  while (!frame && millis() - wait_started_ms < 4000U) {
    frame = shot_claim_web_frame();
    if (!frame) vTaskDelay(pdMS_TO_TICKS(10));
  }
  if (!frame) {
    shot_cancel_web_request();
    server.send(504, "text/plain; charset=UTF-8", "SCREENSHOT TIMEOUT");
    return;
  }

  static constexpr size_t BMP_HEADER_BYTES = 54U;
  static constexpr size_t BMP_ROW_BYTES = SHOT_W * 3U;
  static constexpr size_t BMP_IMAGE_BYTES = BMP_ROW_BYTES * SHOT_H;
  static constexpr size_t BMP_FILE_BYTES = BMP_HEADER_BYTES + BMP_IMAGE_BYTES;
  uint8_t header[BMP_HEADER_BYTES] = {};
  header[0] = 'B';
  header[1] = 'M';
  portal_bmp_le32(&header[2], (uint32_t)BMP_FILE_BYTES);
  portal_bmp_le32(&header[10], (uint32_t)BMP_HEADER_BYTES);
  portal_bmp_le32(&header[14], 40U);
  portal_bmp_le32(&header[18], SHOT_W);
  portal_bmp_le32(&header[22], SHOT_H);
  portal_bmp_le16(&header[26], 1U);
  portal_bmp_le16(&header[28], 24U);
  portal_bmp_le32(&header[34], (uint32_t)BMP_IMAGE_BYTES);

  server.sendHeader("Content-Disposition", "attachment; filename=haffler-plc-screen.bmp");
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(BMP_FILE_BYTES);
  server.send(200, "image/bmp", "");
  server.sendContent((const char *)header, sizeof(header));

  static uint8_t row[BMP_ROW_BYTES];
  for (int y = SHOT_H - 1; y >= 0 && server.client().connected(); --y) {
    const uint16_t *src = &frame[y * SHOT_W];
    for (int x = 0; x < SHOT_W; ++x) {
      const uint16_t pixel = src[x];
      const uint8_t r5 = (uint8_t)((pixel >> 11) & 0x1FU);
      const uint8_t g6 = (uint8_t)((pixel >> 5) & 0x3FU);
      const uint8_t b5 = (uint8_t)(pixel & 0x1FU);
      row[x * 3] = (uint8_t)((b5 << 3) | (b5 >> 2));
      row[x * 3 + 1] = (uint8_t)((g6 << 2) | (g6 >> 4));
      row[x * 3 + 2] = (uint8_t)((r5 << 3) | (r5 >> 2));
    }
    server.sendContent((const char *)row, sizeof(row));
    if ((y & 7) == 0) vTaskDelay(1);
  }
  shot_release_web_frame();
}

static void portal_apply_start_mm(long mm) {
  float ppmm = (float)STP.drv_ppr / STP.Axis_Pitch;
  if (ppmm < 0.001f) ppmm = 1.0f;
  mm = constrain(mm, 0L, 20000L);
  GUI.L2_GUI = (int)mm;
  STP.initial_Position_p = (int)round((float)mm * ppmm);
  save_L2_GUI();
  save_init_pos();
}

static bool portal_parse_list_uint(const String &text, int maximum, int *value);

static bool portal_pin_format_valid(const String &text, unsigned int minimum,
                                    unsigned int maximum) {
  if (text.length() < minimum || text.length() > maximum) return false;
  for (unsigned int i = 0; i < text.length(); ++i) {
    if (!isDigit(text[i])) return false;
  }
  return true;
}

struct WebArgSrc {
  bool change_admin_pin = false;
  String requested_admin_pin = "";

  bool has_stock = false;
  int stock_val = 0;

  bool has_kerf = false;
  float kerf_val = 0.0f;

  bool has_trim = false;
  int trim_val = 0;

  bool has_lang = false;
  int lang_val = 0;

  bool has_vkb = false;
  uint8_t vkb_val = 0;

  bool has_buz = false;
  bool buz_val = false;

  bool has_estop = false;
  bool estop_val = false;

  bool has_startmm = false;
  int startmm_val = 0;

  bool has_maxlen = false;
  long maxlen_val = 0;

  bool has_spd = false;
  int spd_val = 0;

  bool has_aramp = false;
  int aramp_val = 0;

  bool has_jspd = false;
  int jspd_val = 0;

  bool has_jramp = false;
  int jramp_val = 0;

  bool has_jinit = false;
  int jinit_val = 0;

  bool has_ctimer = false;
  int ctimer_val = 0;

  bool has_bdir = false;
  int bdir_val = 0;

  int list_rows = 0;
  float list_sp[LIST_MAX];
  int list_cnt[LIST_MAX];
  uint8_t list_ang[LIST_MAX];
};

static WebArgSrc web_pending_args;
static WebArgSrc web_incoming_args;

static void web_settings_write(const WebArgSrc &src);
static void web_list_write(const WebArgSrc &src);

static void portal_api_settings_set() {
  if (!portal_check_write_allowed()) return;

  // Validate every request field that can reject the request before the first
  // persistent write.  An error response must never leave one PIN changed.
  const String requested_admin_pin = server.hasArg("newadmin") ? server.arg("newadmin") : String();
  const bool change_admin_pin = requested_admin_pin.length() > 0;
  if (change_admin_pin) {
    if (server.arg("adminpin") != admin_pin) {
      server.send(403, "text/plain; charset=UTF-8", "WRONG ADMIN PIN");
      return;
    }
    if (!portal_pin_format_valid(requested_admin_pin, 4, 12)) {
      server.send(422, "text/plain; charset=UTF-8", "INVALID ADMIN PIN: expected 4..12 digits");
      return;
    }
  }

  int requested_start_mm = portal_start_mm();
  int requested_final_mm = STP.final_position_mm.toInt();
  const bool range_change = server.hasArg("startmm") || server.hasArg("maxlen");
  if (server.hasArg("startmm") &&
      !portal_parse_list_uint(server.arg("startmm"), 20000, &requested_start_mm)) {
    server.send(422, "text/plain; charset=UTF-8", "INVALID START: expected integer 0..20000");
    return;
  }
  if (server.hasArg("maxlen") &&
      (!portal_parse_list_uint(server.arg("maxlen"), 20000, &requested_final_mm) ||
       requested_final_mm < 100)) {
    server.send(422, "text/plain; charset=UTF-8", "INVALID END: expected integer 100..20000");
    return;
  }
  if (range_change && requested_start_mm > requested_final_mm) {
    server.send(422, "text/plain; charset=UTF-8", "INVALID RANGE: start must not exceed end");
    return;
  }

  if (server.hasArg("reset_ucuts") && server.arg("reset_ucuts").toInt() == 1) {
    user_cuts = 0;
    save_user_cuts();
    GUI.buzzer_cmd_short = true;
    server.send(200, "text/plain; charset=UTF-8", "OK user cuts reset");
    return;
  }

  web_incoming_args = WebArgSrc();
  WebArgSrc &args = web_incoming_args;
  args.change_admin_pin = change_admin_pin;
  args.requested_admin_pin = requested_admin_pin;
  if (server.hasArg("stock")) { args.has_stock = true; args.stock_val = constrain(server.arg("stock").toInt(), 10, 100000); }
  if (server.hasArg("kerf")) { args.has_kerf = true; args.kerf_val = constrain(roundf(server.arg("kerf").toFloat() * 10.0f) / 10.0f, 0.0f, 50.0f); }
  if (server.hasArg("trim")) { args.has_trim = true; args.trim_val = constrain(server.arg("trim").toInt(), 0, 500); }
  if (server.hasArg("lang")) {
    int lang = server.arg("lang").toInt();
    if (lang == 1 || lang == 2) { args.has_lang = true; args.lang_val = lang; }
  }
  if (server.hasArg("vkb")) { args.has_vkb = true; args.vkb_val = (uint8_t)constrain(server.arg("vkb").toInt(), 0, 2); }
  if (server.hasArg("buz")) { args.has_buz = true; args.buz_val = (server.arg("buz").toInt() == 1); }
  if (server.hasArg("estop")) { args.has_estop = true; args.estop_val = (server.arg("estop").toInt() != 0); }
  if (server.hasArg("startmm")) { args.has_startmm = true; args.startmm_val = requested_start_mm; }
  if (server.hasArg("maxlen")) { args.has_maxlen = true; args.maxlen_val = requested_final_mm; }
  if (server.hasArg("spd")) { args.has_spd = true; args.spd_val = server.arg("spd").toInt(); }
  if (server.hasArg("aramp")) { args.has_aramp = true; args.aramp_val = server.arg("aramp").toInt(); }
  if (server.hasArg("jspd")) { args.has_jspd = true; args.jspd_val = server.arg("jspd").toInt(); }
  if (server.hasArg("jramp")) { args.has_jramp = true; args.jramp_val = server.arg("jramp").toInt(); }
  if (server.hasArg("jinit")) { args.has_jinit = true; args.jinit_val = server.arg("jinit").toInt(); }
  if (server.hasArg("ctimer")) { args.has_ctimer = true; args.ctimer_val = server.arg("ctimer").toInt(); }
  if (server.hasArg("bdir")) { args.has_bdir = true; args.bdir_val = server.arg("bdir").toInt(); }

  if (web_save_policy == 1) {
    web_settings_write(args);
    server.send(200, "text/plain; charset=UTF-8", "OK settings saved");
  } else {
    if (web_pending_kind != 0) {
      server.send(409, "text/plain; charset=UTF-8", "CONFLICT: pending change waiting for confirmation");
      return;
    }
    web_pending_args = args;
    web_pending_kind = 1;
    web_pending_ms = millis();
    server.send(202, "text/plain; charset=UTF-8", "ACCEPTED: waiting for on-device confirmation");
  }
}

static void web_settings_write(const WebArgSrc &src) {
  if (src.change_admin_pin) {
    admin_pin = src.requested_admin_pin;
    save_admin_pin();
  }

  if (src.has_stock) stock_len_mm = src.stock_val;
  if (src.has_kerf) kerf_mm = src.kerf_val;
  if (src.has_trim) trim_mm = src.trim_val;

  if (src.has_lang) {
    GUI.system_language = src.lang_val;
    GUI.pending_lang_selected = (src.lang_val == 2) ? 1 : 0;
    GUI.flag_apply_language = true;
    save_setlang();
  }
  if (src.has_vkb) {
    GUI.vkb_mode = src.vkb_val;
    GUI.virtual_kb_enable = (GUI.vkb_mode != 2);
    GUI.flag_save_vkb = true;
  }
  if (src.has_buz) {
    GUI.buzzer_enable = src.buz_val;
    GUI.flag_save_buzzer = true;
  }
  if (src.has_estop) {
    stopstate = src.estop_val;
    save_stopstate();
  }

  if (src.has_startmm) portal_apply_start_mm(src.startmm_val);
  if (src.has_maxlen) {
    const long ml = src.maxlen_val;
    STP.final_position_mm = String((int)ml);
    STP.final_position_p = (int)round((float)ml * ((float)STP.drv_ppr / STP.Axis_Pitch));
    save_final_pos();
  }
  if (src.has_spd) STP.global_speed = constrain(map(constrain(src.spd_val, 0, 100), 0, 100, 100, 8), 8, 100);
  if (src.has_aramp) auto_ramp_ms = constrain(src.aramp_val, 0, 5000);
  if (src.has_jspd) jog_speed_interval = constrain(map(constrain(src.jspd_val, 0, 100), 0, 100, 100, 8), 8, 100);
  if (src.has_jramp) jog_ramp_ms = constrain(src.jramp_val, 0, 5000);
  if (src.has_jinit) jog_init_speed = constrain(src.jinit_val, 40, 400);
  if (src.has_ctimer) IOS.cut_prcs_timer = constrain(src.ctimer_val, 0, 60) * 1000;
  if (src.has_bdir) STP.system_base_dir = (src.bdir_val == -1) ? -1 : 1;

  save_all();
  save_jog_speed();
  save_ramp_times();
  save_stock_kerf();
  save_cut_timer();
  save_base_dir();
  GUI.flag_reinit_settings = true;
  GUI.flag_update_lable2_5 = true;
  GUI.flag_sync_stock_kerf = true;
  GUI.buzzer_cmd_short = true;
}

static void portal_api_set() {
  portal_api_settings_set();
}

static bool portal_parse_list_uint(const String &text, int maximum, int *value) {
  if (text.length() == 0) return false;
  int parsed = 0;
  for (unsigned int i = 0; i < text.length(); ++i) {
    const char c = text[i];
    if (c < '0' || c > '9') return false;
    const int digit = c - '0';
    if (parsed > (maximum - digit) / 10) return false;
    parsed = parsed * 10 + digit;
  }
  *value = parsed;
  return true;
}

static bool portal_parse_list_length(const String &text, float *value) {
  if (text.length() == 0) return false;
  bool decimal = false;
  bool digit_seen = false;
  float parsed = 0.0f;
  float fraction = 0.1f;
  for (unsigned int i = 0; i < text.length(); ++i) {
    const char c = text[i];
    if (c >= '0' && c <= '9') {
      digit_seen = true;
      const int digit = c - '0';
      if (!decimal) parsed = parsed * 10.0f + (float)digit;
      else {
        parsed += (float)digit * fraction;
        fraction *= 0.1f;
      }
      if (parsed > 1000000.0f) return false;
      continue;
    }
    if (c != '.' || decimal || !digit_seen || i + 1U == text.length()) return false;
    decimal = true;
  }
  *value = roundf(parsed * 10.0f) / 10.0f;
  return true;
}

static void portal_list_validation_error(int row, int initial_mm, int final_mm,
                                         const char *reason) {
  char message[144];
  snprintf(message, sizeof(message),
           "INVALID LIST ROW %d: %s; length %d..%d mm, count 1..999",
           row, reason, initial_mm, final_mm);
  server.send(422, "text/plain; charset=UTF-8", message);
}

static void portal_api_list_set() {
  if (!portal_check_write_allowed()) return;
  int rows = 0;
  if (!server.hasArg("rows") ||
      !portal_parse_list_uint(server.arg("rows"), LIST_MAX, &rows)) {
    server.send(422, "text/plain; charset=UTF-8",
                "INVALID LIST ROWS: expected integer 0..100");
    return;
  }
  const int initial_mm = portal_start_mm();
  const int final_mm = STP.final_position_mm.toInt();
  if (rows > 0 && initial_mm > final_mm) {
    portal_list_validation_error(1, initial_mm, final_mm, "invalid machine range");
    return;
  }

  // Validate every active row before changing arrays, UI flags, or NVS.
  for (int i = 0; i < rows; ++i) {
    String k_sp = "sp" + String(i), k_cnt = "cnt" + String(i);
    float length_mm = 0.0f;
    int count = 0;
    if (!server.hasArg(k_sp) || !server.hasArg(k_cnt) ||
        !portal_parse_list_length(server.arg(k_sp), &length_mm) ||
        !portal_parse_list_uint(server.arg(k_cnt), 999, &count) ||
        count < 1 || length_mm <= 0.0f || length_mm < (float)initial_mm ||
        length_mm > (float)final_mm) {
      portal_list_validation_error(i + 1, initial_mm, final_mm,
                                   "invalid length or count");
      return;
    }
  }

  web_incoming_args = WebArgSrc();
  WebArgSrc &args = web_incoming_args;
  args.list_rows = rows;
  for (int i = 0; i < rows; i++) {
    String k_sp = "sp" + String(i), k_cnt = "cnt" + String(i), k_ang = "ang" + String(i);
    if (server.hasArg(k_sp)) {
      float v = server.arg(k_sp).toFloat();
      v = roundf(v * 10.0f) / 10.0f;
      if (v > (float)final_mm) v = (float)final_mm;
      if (v < (float)initial_mm) v = (float)initial_mm;
      args.list_sp[i] = v;
    } else {
      args.list_sp[i] = 0.0f;
    }
    if (server.hasArg(k_cnt)) {
      args.list_cnt[i] = constrain(server.arg(k_cnt).toInt(), 0, 999);
    } else {
      args.list_cnt[i] = 0;
    }
    int a = 45;
    if (server.hasArg(k_ang)) {
      a = server.arg(k_ang).toInt();
      if (a != 90) a = 45;
    }
    args.list_ang[i] = (uint8_t)a;
  }

  if (web_save_policy == 1) {
    web_list_write(args);
    server.send(200, "text/plain; charset=UTF-8", "OK list saved");
  } else {
    if (web_pending_kind != 0) {
      server.send(409, "text/plain; charset=UTF-8", "CONFLICT: pending change waiting for confirmation");
      return;
    }
    web_pending_args = args;
    web_pending_kind = 2;
    web_pending_ms = millis();
    server.send(202, "text/plain; charset=UTF-8", "ACCEPTED: waiting for on-device confirmation");
  }
}

static void web_list_write(const WebArgSrc &src) {
  const int rows = src.list_rows;
  GUI.visible_raws = rows;
  GUI.flag_refresh_visible = true;
  // The page posts all 100 controls, but only rows [0, rows) belong to this
  // request.  Ignore inactive controls so hidden/stale values cannot mutate NVS.
  for (int i = 0; i < rows; i++) {
    float v = src.list_sp[i];
    AUT.list_sp_Val_p[i] = v;
    char b[16];
    if (fabsf(v - roundf(v)) < 0.05f) snprintf(b, sizeof(b), "%d", (int)lroundf(v));
    else snprintf(b, sizeof(b), "%.1f", (double)v);
    GUI.inputBuffer_L_3_1_x[i] = b;
    GUI.pending_sp_save_bits[i >> 3] |= (uint8_t)(1u << (i & 7));

    int c = src.list_cnt[i];
    AUT.list_count_Val[i] = c;
    AUT.list_count_base_Val[i] = c;
    GUI.inputBuffer_L_3_2_x[i] = String(c);
    GUI.pending_count_save_bits[i >> 3] |= (uint8_t)(1u << (i & 7));
    save_permanently_preferences_count_base(i, c);

    uint8_t ang = src.list_ang[i];
    if (ang != 90) ang = 45;
    AUT.list_angle_Val[i] = ang;
    save_permanently_preferences_angle_val(i, ang);
  }
  opt_notify_list_edited();
  GUI.flag_save_visible_raws = true;
  flush_pending_list_saves_now();
  GUI.flag_update_labels_3_1 = true;
  GUI.flag_update_labels_3_2 = true;
  GUI.flag_update_panel3_2_x_clr = true;
  GUI.buzzer_cmd_short = true;
}

void web_apply_pending_now() {
  if (web_pending_kind == 1) {
    web_settings_write(web_pending_args);
  } else if (web_pending_kind == 2) {
    web_list_write(web_pending_args);
  }
  web_pending_kind = 0;
  web_pending_ms = 0;
}

void web_discard_pending_now() {
  web_pending_kind = 0;
  web_pending_ms = 0;
}

static void portal_api_pending_get() {
  uint32_t age_s = (web_pending_kind != 0) ? (uint32_t)((millis() - web_pending_ms) / 1000UL) : 0;
  if (web_pending_kind != 0 && age_s > 60) {
    web_pending_kind = 0;
    web_pending_ms = 0;
  }
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"pending\":%d,\"kind\":%d,\"age_s\":%u,\"policy\":%d}",
           (web_pending_kind != 0 ? 1 : 0), (int)web_pending_kind, (unsigned)age_s, (int)web_save_policy);
  server.send(200, "application/json; charset=UTF-8", buf);
}

static void portal_api_backup_get() {
  if (portal_machine_busy()) {
    server.send(409, "application/json; charset=UTF-8", "{\"error\":\"machine_busy\"}");
    return;
  }
  String json = "{\n  \"app\": \"HAFFLER_CUTTER\",\n";
  json += "  \"version\": \"" PROJECT_VERSION_STRING "\",\n";
  json += "  \"stock_len\": " + String(stock_len_mm) + ",\n";
  json += "  \"kerf\": " + String(OPT.kerf_mm, 1) + ",\n";
  json += "  \"company\": \"" + g_company_name + "\",\n";
  json += "  \"blade_limit\": " + String(blade_limit) + ",\n";
  json += "  \"blade_cuts\": " + String(blade_cuts) + ",\n";
  json += "  \"total_cuts\": " + String(total_cuts) + ",\n";
  json += "  \"visible_rows\": " + String(GUI.visible_raws) + ",\n";
  json += "  \"list\": [\n";
  int rows = constrain(GUI.visible_raws, 1, LIST_MAX);
  for (int i = 0; i < rows; i++) {
    json += "    {\"row\":" + String(i + 1) +
            ", \"len\":" + String(AUT.list_sp_Val_p[i], 1) +
            ", \"count\":" + String(AUT.list_count_Val[i]) +
            ", \"angle\":" + String(AUT.list_angle_Val[i]) + "}";
    if (i < rows - 1) json += ",";
    json += "\n";
  }
  json += "  ]\n}\n";
  server.sendHeader("Content-Disposition", "attachment; filename=haffler_backup.json");
  server.send(200, "application/json; charset=UTF-8", json);
}

static void portal_api_report_get() {
  uint32_t hp = blade_health_pct();
  String json = "{\n";
  json += "  \"blade_health_pct\": " + String(hp) + ",\n";
  json += "  \"blade_cuts\": " + String(blade_cuts) + ",\n";
  json += "  \"blade_limit\": " + String(blade_limit) + ",\n";
  json += "  \"blade_service_date\": \"" + String(blade_service_date) + "\",\n";
  json += "  \"total_cuts_lifetime\": " + String(total_cuts) + ",\n";
  json += "  \"total_cuts_today\": " + String(cuts_day_sum()) + ",\n";
  json += "  \"cuts_manual\": " + String(cuts_life_manual) + ",\n";
  json += "  \"cuts_list\": " + String(cuts_life_list) + ",\n";
  json += "  \"cuts_opt\": " + String(cuts_life_opt) + "\n";
  json += "}\n";
  server.send(200, "application/json; charset=UTF-8", json);
}

enum PortalOtaState : uint8_t {
  PORTAL_OTA_IDLE = 0,
  PORTAL_OTA_ACTIVE,
  PORTAL_OTA_COMPLETE,
  PORTAL_OTA_WRONG_PIN,
  PORTAL_OTA_MACHINE_BUSY,
  PORTAL_OTA_BEGIN_FAILED,
  PORTAL_OTA_WRITE_FAILED,
  PORTAL_OTA_END_FAILED,
  PORTAL_OTA_ABORTED,
};
static PortalOtaState portal_ota_state = PORTAL_OTA_IDLE;
static bool portal_ota_lock_owned = false;

static void portal_ota_release_lock() {
  if (!portal_ota_lock_owned) return;
  // Clear every pending motion command before reopening Task3/Task8.
  panel_request_stop_motion();
  (void)wifi_owner_transition(WIFI_OWNER_LOCAL_OTA, WIFI_OWNER_PORTAL);
  portal_ota_lock_owned = false;
  web_portal_ota_active = false;
  g_update_mode = false;
}

static void portal_ota_finish() {
  if (portal_ota_state == PORTAL_OTA_WRONG_PIN) {
    server.send(403, "text/plain; charset=UTF-8",
                "WRONG MANAGER PIN — رمز مدیر LCD اشتباه است");
    return;
  }
  if (portal_ota_state == PORTAL_OTA_MACHINE_BUSY) {
    server.send(409, "text/plain; charset=UTF-8", "REJECTED: machine busy");
    return;
  }
  if (portal_ota_state != PORTAL_OTA_COMPLETE || Update.hasError()) {
    if (Update.isRunning()) Update.abort();
    server.send(500, "text/plain", "FAIL: " + String(Update.errorString()));
    portal_ota_release_lock();
    portal_ota_state = PORTAL_OTA_IDLE;
  } else {
    server.send(200, "text/plain", "OK — Flashed. Rebooting... ری‌استارت");
    delay(600);
    ESP.restart();
  }
}

static void portal_ota_upload() {
  HTTPUpload &up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    portal_ota_state = PORTAL_OTA_IDLE;
    // WEB-PIN-REMOVAL-01: the only remaining web credential is the LCD
    // manager PIN, checked once here on the /update upload itself.
    if (server.arg("pin") != admin_pin) {
      portal_ota_state = PORTAL_OTA_WRONG_PIN;
      return;
    }
    if (portal_machine_busy() || g_update_mode || update_flow_network_busy()) {
      portal_ota_state = PORTAL_OTA_MACHINE_BUSY;
      return;
    }

    // مالکیت PORTAL→LOCAL_OTA و قفل حرکت بدون شکاف گرفته می‌شود. سپس idle
    // دوباره بررسی می‌شود تا فرمانی که بین check اول و lock رسیده رد شود.
    if (!wifi_owner_transition(WIFI_OWNER_PORTAL, WIFI_OWNER_LOCAL_OTA)) {
      portal_ota_state = PORTAL_OTA_MACHINE_BUSY;
      return;
    }
    g_update_mode = true;
    portal_ota_lock_owned = true;
    web_portal_ota_active = true;
    if (portal_machine_busy()) {
      portal_ota_state = PORTAL_OTA_MACHINE_BUSY;
      portal_ota_release_lock();
      return;
    }
    Serial.printf("OTA start: %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      portal_ota_state = PORTAL_OTA_BEGIN_FAILED;
      return;
    }
    portal_ota_state = PORTAL_OTA_ACTIVE;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (portal_ota_state != PORTAL_OTA_ACTIVE || !Update.isRunning()) return;
    if (Update.write(up.buf, up.currentSize) != up.currentSize) {
      portal_ota_state = PORTAL_OTA_WRITE_FAILED;
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (portal_ota_state != PORTAL_OTA_ACTIVE || !Update.isRunning()) return;
    if (!Update.end(true)) {
      portal_ota_state = PORTAL_OTA_END_FAILED;
      return;
    }
    portal_ota_state = PORTAL_OTA_COMPLETE;
    Serial.println("OTA end");
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    portal_ota_state = PORTAL_OTA_ABORTED;
    portal_ota_release_lock();
  }
}

static void portal_register_routes_once() {
  static bool done = false;
  if (done) return;
  done = true;
  server.on("/", HTTP_GET, portal_send_index);
  server.on("/index.html", HTTP_GET, portal_send_index);
  server.on("/api/get", HTTP_GET, portal_api_get);
  server.on("/api/set", HTTP_POST, portal_api_set);
  server.on("/api/settings/get", HTTP_GET, portal_api_settings_get);
  server.on("/api/settings/set", HTTP_POST, portal_api_settings_set);
  server.on("/api/list/get", HTTP_GET, portal_api_list_get);
  server.on("/api/list/set", HTTP_POST, portal_api_list_set);
  server.on("/api/pending/get", HTTP_GET, portal_api_pending_get);
  server.on("/api/optimized/get", HTTP_GET, portal_api_optimized_get);
  server.on("/api/backup/get", HTTP_GET, portal_api_backup_get);
  server.on("/api/report/get", HTTP_GET, portal_api_report_get);
  server.on("/api/screen.bmp", HTTP_POST, portal_api_screen_bmp);
  server.on("/update", HTTP_POST, portal_ota_finish, portal_ota_upload);
  server.on("/generate_204", HTTP_GET, portal_captive_probe);
  server.on("/gen_204", HTTP_GET, portal_captive_probe);
  server.on("/hotspot-detect.html", HTTP_GET, portal_captive_probe);
  server.on("/library/test/success.html", HTTP_GET, portal_captive_probe);
  server.on("/connecttest.txt", HTTP_GET, portal_captive_probe);
  server.on("/fwlink", HTTP_GET, portal_captive_redirect);
  server.on("/redirect", HTTP_GET, portal_captive_redirect);
  server.on("/ncsi.txt", HTTP_GET, portal_captive_probe);
  server.on("/canonical.html", HTTP_GET, portal_captive_probe);
  server.on("/success.txt", HTTP_GET, portal_captive_probe);
  server.onNotFound([]() {
    if (server.method() != HTTP_GET) {
      portal_captive_redirect();
      return;
    }
    // WebServer is single-client: a full 255KB chunked page for every unknown
    // GET (favicon.ico, apple-touch-icon, probe tails) queues the real
    // document/API request behind it. Asset-like paths get an empty 404;
    // extensionless document paths still fall through to the app shell.
    const String uri = server.uri();
    const int dot = uri.lastIndexOf('.');
    const int slash = uri.lastIndexOf('/');
    if (dot > slash && dot >= 0) {
      server.sendHeader("Connection", "close");
      server.send(404, "text/plain", "");
      return;
    }
    portal_send_index();
  });
}

static void portal_release_or_handoff_owner() {
  if (!wifi_owner_transition(WIFI_OWNER_UPDATER_PENDING, WIFI_OWNER_UPDATER))
    wifi_owner_release(WIFI_OWNER_PORTAL);
}

static void portal_start_ap() {
  // UpdateNetTask owns the radio while update mode/network work is active.
  // Refuse here as well as in Task2 so a queued/racing start cannot steal WiFi.
  if (g_update_mode || update_flow_network_busy() ||
      !wifi_owner_try_acquire(WIFI_OWNER_PORTAL)) {
    web_portal_running = false;
    web_portal_ap_result = -2;
    GUI.flag_web_portal_chip_refresh = true;
    Serial.println("[Portal] start rejected: update network owns WiFi");
    return;
  }

  // Rebuild SSID here too (task may restart AP after Stop without recreating task)
  snprintf(ap_ssid, sizeof(ap_ssid), "Haffler-Cutter-V%s", PROJECT_VERSION_STRING);
  ap_ssid[sizeof(ap_ssid) - 1] = '\0';
  Serial.printf("[Portal] start AP ssid='%s' heap=%u\n", ap_ssid, (unsigned)ESP.getFreeHeap());

  // Move to OFF without calling softAPdisconnect() on an OFF/STA-only driver.
  // In this Arduino core softAPdisconnect(true) calls AP.begin() first and can
  // start an unconfigured AP, which is the known cold-start crash path.
  WiFi.persistent(false);
  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
    WiFi.softAPdisconnect(false);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  WiFi.mode(WIFI_OFF);
  vTaskDelay(pdMS_TO_TICKS(250));

  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  bool ok = false;
  const int channels[] = {1, 6, 11};
  for (int attempt = 0; attempt < 3 && !ok; attempt++) {
    WiFi.mode(WIFI_AP);
    vTaskDelay(pdMS_TO_TICKS(120));
    WiFi.softAPConfig(apIP, gateway, subnet);
    // Protected local maintenance network; credentials are shown on the LCD.
    ok = WiFi.softAP(ap_ssid, ap_pass, channels[attempt], 0, 4);
    Serial.printf("[Portal] softAP attempt=%d ch=%d ok=%d\n", attempt + 1, channels[attempt], (int)ok);
    if (!ok) {
      WiFi.mode(WIFI_OFF);
      vTaskDelay(pdMS_TO_TICKS(150));
    }
  }
  if (!ok) {
    Serial.println("[Portal] softAP failed — retry WIFI_AP_STA");
    WiFi.mode(WIFI_AP_STA);
    vTaskDelay(pdMS_TO_TICKS(120));
    WiFi.softAPConfig(apIP, gateway, subnet);
    ok = WiFi.softAP(ap_ssid, ap_pass, 1, 0, 4);
  }

  vTaskDelay(pdMS_TO_TICKS(500));
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[Portal] softAP ok=%d ip=%s mode=%d heap=%u\n",
                (int)ok, ip.toString().c_str(), (int)WiFi.getMode(), (unsigned)ESP.getFreeHeap());

  if (!ok || ip == IPAddress(0, 0, 0, 0)) {
    wifi_mode_t fail_mode = WiFi.getMode();
    if (fail_mode == WIFI_MODE_AP || fail_mode == WIFI_MODE_APSTA)
      WiFi.softAPdisconnect(false);
    WiFi.mode(WIFI_OFF);
    portal_release_or_handoff_owner();
    web_portal_running = false;
    web_portal_ap_result = -1;
    GUI.flag_web_portal_chip_refresh = true;
    return;
  }

  portalDns.stop();
  portalDns.start(53, "*", ip);
  portal_register_routes_once();
  server.begin();
  web_portal_running = true;
  web_portal_started_ms = millis();
  if (web_save_policy == 0) web_policy_ask_request = true;
  portal_active_mode = PORTAL_WIFI_DIRECT;
  web_wifi_save_mode(WEB_WIFI_DIRECT);
  web_portal_resume_commit(true);
  web_portal_ap_result = 1;
  GUI.flag_web_portal_chip_refresh = true;
  GUI.buzzer_cmd_short = true;
}

static void portal_start_sta() {
  if (wifi_owner_get() != WIFI_OWNER_PORTAL || WiFi.status() != WL_CONNECTED) {
    Serial.println("[Portal] STA not connected — starting Direct SoftAP fallback");
    portal_start_ap();
    return;
  }
  strncpy(ap_ssid, WiFi.SSID().c_str(), sizeof(ap_ssid) - 1);
  ap_ssid[sizeof(ap_ssid) - 1] = '\0';
  portalDns.stop();
  portal_register_routes_once();
  server.begin();
  portal_mdns_started = MDNS.begin("haffler");
  portal_active_mode = PORTAL_WIFI_STA;
  web_portal_running = true;
  web_portal_started_ms = millis();
  if (web_save_policy == 0) web_policy_ask_request = true;
  web_wifi_save_mode(WEB_WIFI_STA);
  web_portal_ap_result = 2;  // Task2 reports a connected portal; IP/API are truthful.
  GUI.flag_web_portal_chip_refresh = true;
}

static void portal_stop_ap() {
  server.stop();
  portalDns.stop();
  if (portal_mdns_started) { MDNS.end(); portal_mdns_started = false; }
  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA)
    WiFi.softAPdisconnect(false);
  if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  portal_release_or_handoff_owner();
  web_portal_running = false;
  web_portal_ap_result = 0;
  GUI.flag_web_portal_chip_refresh = true;
  Serial.println("[Portal] AP stopped");
}

void WebPortalTask(void *pvParameters) {
  for (;;) {
    while (!web_portal_start_request) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    web_portal_start_request = false;
    web_portal_transitioning = true;
    // Do not clear a stop request until ownership has been checked. The update
    // flow may have cancelled this start between the wake-up and this point.
    if (g_update_mode || update_flow_network_busy()) {
      web_portal_running = false;
      web_portal_transitioning = false;
      web_portal_ap_result = -2;
      GUI.flag_web_portal_chip_refresh = true;
      continue;
    }
    web_portal_stop_request = false;
    if (portal_start_mode == PORTAL_WIFI_STA) portal_start_sta();
    else                                      portal_start_ap();
    web_portal_transitioning = false;

    while (web_portal_running) {
      if (web_portal_stop_request) {
        if (web_portal_ota_active) {
          web_portal_stop_request = false;
          vTaskDelay(pdMS_TO_TICKS(20));
          continue;
        }
        web_portal_stop_request = false;
        // Stop is now accepted. Clear the committed RTC state before the
        // asynchronous radio shutdown so a reset mid-transition stays off.
        web_portal_resume_commit(false);
        web_portal_transitioning = true;
        portal_stop_ap();
        web_portal_transitioning = false;
        break;
      }
      if (portal_active_mode == PORTAL_WIFI_DIRECT) portalDns.processNextRequest();
      server.handleClient();
      // 200 polls/s is ample for this local portal and avoids stealing the
      // render core at 1 kHz while WiFi is enabled.
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }
}
