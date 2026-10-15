// Haffler Secure Serial Remote Control Protocol (HSSRP-v1)
// ─────────────────────────────────────────────────────────────────────────────
// Enables unified, secure authenticated remote navigation, testing, screen switching,
// language changing, diagnostics, and raw SHOT stream for BOTH Serial (USB CDC) and Serial0 (CP210x).
#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <mbedtls/md.h>
#include "defines.h"
#include "screens.h"
#include "vars.h"
#include "hwtest.h"
#include "t_screens.h"
#include "lang.h"
#include "screenshot.h"
#include "fw_changelog.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#define HAF_SERIAL_SECRET_KEY "Haffler_HTV3A_Secure_Key_2026!#"

extern objects_t objects;
extern bool g_hwtest_active;
extern Lang g_lang;
extern "C" void app_set_language(int lang);
extern "C" void t_loadScreen(enum T_ScreensEnum screenId);
extern "C" void eez_flow_set_screen(int16_t screenId, lv_scr_load_anim_t animType, uint32_t speed, uint32_t delay);
extern void my_sim_touch(int16_t x, int16_t y);
extern "C" void hwtest_close_all_modals(void);

static inline bool haf_verify_signature(const char *cmd, const char *args, const char *nonce, const char *sig) {
  char payload[128];
  snprintf(payload, sizeof(payload), "%s:%s:%s", cmd, args ? args : "", nonce);

  uint8_t hmac_res[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *)HAF_SERIAL_SECRET_KEY, strlen(HAF_SERIAL_SECRET_KEY));
  mbedtls_md_hmac_update(&ctx, (const unsigned char *)payload, strlen(payload));
  mbedtls_md_hmac_finish(&ctx, hmac_res);
  mbedtls_md_free(&ctx);

  char expected_hex[17];
  for (int i = 0; i < 8; i++) {
    snprintf(&expected_hex[i * 2], 3, "%02x", hmac_res[i]);
  }
  expected_hex[16] = '\0';

  return (strncasecmp(expected_hex, sig, 16) == 0);
}

static inline void haf_handle_command(Stream &port, const char *cmd, const char *args) {
  if (strcmp(cmd, "SHOT") == 0) {
    if (shot_request(&port)) {
      port.println("HAF_OK:SHOT_REQUESTED");
    } else {
      port.println("HAF_ERR:SHOT_BUSY");
    }
  } else if (strcmp(cmd, "STAT") == 0 || strcmp(cmd, "STATUS") == 0) {
    uint32_t free_sram = ESP.getFreeHeap();
    uint32_t free_psram = ESP.getFreePsram();
    extern AppRestoreContext s_rtc_restore_ctx;
    port.printf("HAF_OK:STAT sram=%u psram=%u uptime_ms=%lu lang=%d hwtest=%d rst=%d rtc_mag=0x%08X rtc_scr=%d rtc_hwt=%d\n",
                (unsigned)free_sram, (unsigned)free_psram, (unsigned long)millis(), (int)g_lang, g_hwtest_active ? 1 : 0,
                (int)esp_reset_reason(), (unsigned)s_rtc_restore_ctx.magic, (int)s_rtc_restore_ctx.screen_id, (int)s_rtc_restore_ctx.is_hwtest);
  } else if (strcmp(cmd, "INFO") == 0) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    port.printf("HAF_OK:INFO ver=%s date=%s time=%s part=%s sram=%u psram=%u\n",
                VERSION, FW_BUILD_DATE, FW_BUILD_TIME,
                running ? running->label : "factory",
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
  } else if (strcmp(cmd, "NAV") == 0) {
    hwtest_close_all_modals();
    if (strcmp(args, "splash") == 0) {
      if (g_hwtest_active) hwtest_exit();
      eez_flow_set_screen(SCREEN_ID_SCREEN_SPLASH, LV_SCR_LOAD_ANIM_NONE, 0, 0);
      port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
    } else if (strcmp(args, "password") == 0) {
      if (g_hwtest_active) hwtest_exit();
      set_var_is_admin_password_verifyed(false);
      eez_flow_set_screen(SCREEN_ID_SCREEN_PASSWORD, LV_SCR_LOAD_ANIM_NONE, 0, 0);
      port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
    } else if (strcmp(args, "wifi") == 0) {
      if (g_hwtest_active) hwtest_exit();
      set_var_is_admin_password_verifyed(false);
      eez_flow_set_screen(SCREEN_ID_SCREEN_WIFI, LV_SCR_LOAD_ANIM_NONE, 0, 0);
      port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
    } else if (strcmp(args, "firmware") == 0) {
      if (g_hwtest_active) hwtest_exit();
      set_var_is_admin_password_verifyed(true);
      eez_flow_set_screen(SCREEN_ID_SCREEN_FRAMWARE, LV_SCR_LOAD_ANIM_NONE, 0, 0);
      port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
    } else {
      // HW Test screens (15 total)
      if (!g_hwtest_active) {
        hwtest_enter_ui();
      }
      if (strcmp(args, "hw_main") == 0) {
        t_loadScreen(T_SCREEN_ID_MAIN);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_setup") == 0) {
        t_loadScreen(T_SCREEN_ID_SETUP);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_rs485") == 0) {
        t_loadScreen(T_SCREEN_ID_RS485);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_gsm") == 0) {
        t_loadScreen(T_SCREEN_ID_GSM);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_ble") == 0 || strcmp(args, "hw_bluetooth") == 0) {
        t_loadScreen(T_SCREEN_ID_BLUETOOTH);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_wifi") == 0) {
        t_loadScreen(T_SCREEN_ID_WIFI);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_disp_w") == 0) {
        t_loadScreen(T_SCREEN_ID_DISPLAY_W);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_disp_b") == 0) {
        t_loadScreen(T_SCREEN_ID_DISPLAY_B);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_disp_r") == 0) {
        t_loadScreen(T_SCREEN_ID_DISPLAY_TEST_R);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_disp_g") == 0) {
        t_loadScreen(T_SCREEN_ID_DISPLAY_G);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_disp_blue") == 0) {
        t_loadScreen(T_SCREEN_ID_DISPLAY_BLUE);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_sys_info") == 0) {
        t_loadScreen(T_SCREEN_ID_SYS_INFO);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_touch") == 0 || strcmp(args, "hw_touch_test") == 0) {
        t_loadScreen(T_SCREEN_ID_TOUCH_TEST);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_clock") == 0) {
        t_loadScreen(T_SCREEN_ID_CLOCK_SETUP);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else if (strcmp(args, "hw_ssr") == 0) {
        t_loadScreen(T_SCREEN_ID_SSR);
        port.printf("HAF_OK:NAV_SUCCESS target=%s\n", args);
      } else {
        port.printf("HAF_ERR:UNKNOWN_SCREEN target=%s\n", args);
      }
    }
  } else if (strcmp(cmd, "LANG") == 0) {
    int lang_idx = -1;
    if (strcmp(args, "en") == 0) lang_idx = LANG_EN;
    else if (strcmp(args, "tr") == 0) lang_idx = LANG_TR;
    else if (strcmp(args, "fa") == 0) lang_idx = LANG_FA;
    else if (strcmp(args, "de") == 0) lang_idx = LANG_DE;
    else if (strcmp(args, "ru") == 0) lang_idx = LANG_RU;
    else if (isdigit(args[0])) lang_idx = atoi(args);

    if (lang_idx >= 0 && lang_idx < LANG_COUNT) {
      app_set_language(lang_idx);
      port.printf("HAF_OK:LANG_SUCCESS lang=%d\n", lang_idx);
    } else {
      port.printf("HAF_ERR:INVALID_LANG %s\n", args);
    }
  } else if (strcmp(cmd, "TAP") == 0) {
    int x = -1, y = -1;
    if (sscanf(args, "%d,%d", &x, &y) == 2 || sscanf(args, "%d %d", &x, &y) == 2) {
      my_sim_touch((int16_t)x, (int16_t)y);
      port.printf("HAF_OK:TAP_ACK x=%d y=%d\n", x, y);
    } else {
      port.printf("HAF_ERR:INVALID_TAP_ARGS %s\n", args);
    }
  } else if (strcmp(cmd, "CLOSE_MODAL") == 0) {
    hwtest_close_all_modals();
    port.println("HAF_OK:MODALS_CLOSED");
  } else if (strcmp(cmd, "AUTOQC") == 0) {
    if (!g_hwtest_active) {
      hwtest_enter_ui();
    }
    t_loadScreen(T_SCREEN_ID_MAIN);
    hwtest_start_auto_qc();
    port.println("HAF_OK:AUTOQC_STARTED");
  } else if (strcmp(cmd, "PRINT") == 0) {
    hwtest_print_haffler();
    port.println("HAF_OK:PRINT_DISPATCHED");
  } else if (strcmp(cmd, "REBOOT") == 0) {
    port.println("HAF_OK:REBOOTING");
    delay(100);
    ESP.restart();
  } else {
    port.printf("HAF_ERR:UNKNOWN_CMD %s\n", cmd);
  }
}

struct SerialRxState {
  char buf[256];
  uint16_t len;
};

static SerialRxState s_rx_usb = {{0}, 0};
static SerialRxState s_rx_uart0 = {{0}, 0};

static inline void haf_poll_stream(Stream &port, SerialRxState &st) {
  uint16_t bytes_processed = 0;
  while (port.available() && bytes_processed < 512) {
    bytes_processed++;
    char c = (char)port.read();
    if (c == '\n' || c == '\r') {
      if (st.len > 0) {
        st.buf[st.len] = '\0';
        if (strcmp(st.buf, "SHOT") == 0) {
          if (!shot_request(&port)) {
            port.println("SHOT_ERR busy");
          }
        } else if (strcmp(st.buf, "PING") == 0) {
          port.println("PONG");
        } else if (strncmp(st.buf, "HAF:", 4) == 0) {
          char *p1 = st.buf + 4;
          char *c1 = strchr(p1, ':');
          if (c1) {
            *c1 = '\0';
            char *cmd = p1;
            char *p2 = c1 + 1;
            char *c2 = strchr(p2, ':');
            if (c2) {
              *c2 = '\0';
              char *args = p2;
              char *p3 = c2 + 1;
              char *c3 = strchr(p3, ':');
              if (c3) {
                *c3 = '\0';
                char *nonce = p3;
                char *sig = c3 + 1;
                // Trim trailing CR/LF/spaces from sig
                char *end = sig + strlen(sig) - 1;
                while (end >= sig && (*end == '\r' || *end == '\n' || *end == ' ')) {
                  *end = '\0';
                  end--;
                }
                if (haf_verify_signature(cmd, args, nonce, sig)) {
                  haf_handle_command(port, cmd, args);
                } else {
                  port.println("HAF_ERR:AUTH_FAILED");
                }
              } else {
                port.println("HAF_ERR:MALFORMED_PACKET");
              }
            } else {
              port.println("HAF_ERR:MALFORMED_PACKET");
            }
          } else {
            port.println("HAF_ERR:MALFORMED_PACKET");
          }
        }
        st.len = 0;
      }
    } else if (st.len < sizeof(st.buf) - 1) {
      st.buf[st.len++] = c;
    }
  }
}

static inline void haf_poll() {
  haf_poll_stream(Serial, s_rx_usb);
  haf_poll_stream(Serial0, s_rx_uart0);
}
