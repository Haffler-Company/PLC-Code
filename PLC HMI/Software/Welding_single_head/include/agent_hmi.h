#pragma once

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <string.h>
#include <mbedtls/md.h>

#include "screenshot.h"
#include "defines.h"
#include "vars.h"

#ifdef __cplusplus
extern "C" {
#endif
extern int SSR_1, TABLE, PRESS_LEFT, PRESS_RIGHT, ELEMENT, SHABLON, ZARBEH1, ZARBEH2;
extern int EMERGENCY, PEDAL;
extern void eez_flow_set_screen(int16_t screenId, lv_screen_load_anim_t animType, uint32_t speed, uint32_t delay);
extern void apply_language_now(int32_t lang);
extern bool display_port_touch_detected(void);
extern void display_port_touch_info(uint8_t *chip, uint8_t *addr, uint32_t *reads, uint32_t *errs);
#ifdef __cplusplus
}
#endif

#ifndef SERIAL_DEBUG_BAUD
#define SERIAL_DEBUG_BAUD 921600
#endif

#ifndef MASTER_SECRET_KEY
#define MASTER_SECRET_KEY "HAFFLER_WELDING_SEC_KEY_2026"
#endif

// فلگ حالت سازگاری توسعه: در حالت ۱ دستورات خام سریال بدون امضا نیز در ترمینال پذیرفته می‌شوند
// ولی بسته‌های $WELD باید حتماً دارای امضای معتبر HMAC-SHA256 باشند.
#ifndef SERIAL_SEC_BYPASS
#define SERIAL_SEC_BYPASS 1
#endif

extern volatile bool g_agent_touch_active;
extern volatile int16_t g_agent_touch_x;
extern volatile int16_t g_agent_touch_y;
extern volatile uint32_t g_agent_touch_release_ms;
extern volatile int g_agent_nav_request;
extern volatile bool g_warranty_expired_dialog_request;
extern volatile bool g_warranty_screen_request;
extern volatile bool g_mfg_setup_request;
#ifdef __cplusplus
extern "C" {
#endif
void hide_warranty_expired_dialog(void);
void close_warranty_screen(void);
void pedal_operation(void);
void emergency_operation(void);
#ifdef __cplusplus
}
#endif

static inline int agent_arg_int(const char *text, int index, int fallback)
{
    const char *p = text;
    for (int i = 0; i < index; i++) {
        p = strchr(p, ' ');
        if (!p) return fallback;
        while (*p == ' ') p++;
    }
    if (!p) return fallback;
    return atoi(p);
}

static inline bool agent_verify_hmac(const char *payload, size_t len, const char *key, const char *sig_hex)
{
    if (!payload || !key || !sig_hex || strlen(sig_hex) < 64) return false;
    unsigned char hmac[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    if (mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, strlen(key));
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)payload, len);
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    char hex[65];
    for (int i = 0; i < 32; i++) {
        snprintf(&hex[i * 2], 3, "%02x", hmac[i]);
    }
    hex[64] = '\0';
    return (strcasecmp(hex, sig_hex) == 0);
}

static inline void agent_print_help(Stream *port)
{
    port->println("OK ===== WELDING HMI INDUSTRIAL AGENT PROTOCOL =====");
    port->println("OK PING                   -> Firmware connectivity and version");
    port->println("OK STAT                   -> Live machine status (Temp, SSR, Jacks, Heap)");
    port->println("OK MEM                    -> SRAM/PSRAM memory watermarks");
    port->println("OK SHOT                   -> Hardware framebuffer capture (Zero-lag PSRAM, ~3s)");
    port->println("OK SAFE                   -> Force SSR OFF and de-energize all jacks");
    port->println("OK NAV <id|name>          -> Navigate to screen (1..11, 111, MENU, WIFI, etc.)");
    port->println("OK LANG <1|2|3|fa|en|tr>  -> Switch language");
    port->println("OK TAP x y [ms]           -> Touch tap injection");
    port->println("OK PRESS x y              -> Continuous touch press");
    port->println("OK REL                    -> Touch release");
    port->println("OK TEMP                   -> Live thermocouple & setpoint");
    port->println("OK SEC_FRAME              -> $WELD,<SEQ>,<NONCE>,<CMD>,<PAYLOAD>*<HMAC-SHA256>");
}

static inline void agent_safe_stop(Stream *port)
{
    if (SSR_1 >= 0) digitalWrite(SSR_1, HIGH);
    if (TABLE >= 0) digitalWrite(TABLE, HIGH);
    if (PRESS_LEFT >= 0) digitalWrite(PRESS_LEFT, HIGH);
    if (PRESS_RIGHT >= 0) digitalWrite(PRESS_RIGHT, HIGH);
    if (ELEMENT >= 0) digitalWrite(ELEMENT, HIGH);
    if (SHABLON >= 0) digitalWrite(SHABLON, HIGH);
    if (ZARBEH1 >= 0) digitalWrite(ZARBEH1, HIGH);
    if (ZARBEH2 >= 0) digitalWrite(ZARBEH2, HIGH);

    if (port) port->println("OK SAFE_EXECUTED");
}

static inline int agent_resolve_screen_name(const char *name)
{
    if (!name || !*name) return -1;
    if (isdigit((unsigned char)name[0])) {
        return atoi(name);
    }
    if (strcasecmp(name, "SPLASH") == 0) return 1;
    if (strcasecmp(name, "MENU") == 0 || strcasecmp(name, "MAIN") == 0) return 2;
    if (strcasecmp(name, "GENERAL") == 0 || strcasecmp(name, "SETTING") == 0) return 3;
    if (strcasecmp(name, "JACK") == 0 || strcasecmp(name, "JACKS") == 0) return 4;
    if (strcasecmp(name, "ADMIN") == 0 || strcasecmp(name, "IO") == 0) return 5;
    if (strcasecmp(name, "ADMIN_PW") == 0 || strcasecmp(name, "ADMIN_PASS") == 0) return 6;
    if (strcasecmp(name, "LOADING") == 0) return 7;
    if (strcasecmp(name, "UPDATE") == 0 || strcasecmp(name, "INFO") == 0) return 8;
    if (strcasecmp(name, "FRAMWARE") == 0 || strcasecmp(name, "CARDS") == 0) return 9;
    if (strcasecmp(name, "PASSWORD") == 0 || strcasecmp(name, "PW") == 0) return 10;
    if (strcasecmp(name, "WIFI") == 0) return 11;
    if (strcasecmp(name, "WIFI_PASS") == 0 || strcasecmp(name, "WIFI_PASSWORD") == 0) return 111;
    return -1;
}

static inline void agent_execute_unwrapped_cmd(Stream *port, const char *cmd)
{
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    if (strncmp(cmd, "PING", 4) == 0) {
        port->printf("PONG welding_hmi_v%s\n", VERSION);
    }
    else if (strncmp(cmd, "HELP", 4) == 0) {
        agent_print_help(port);
    }
    else if (strncmp(cmd, "STAT", 4) == 0) {
        size_t free_int = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        extern int16_t eez_flow_get_current_screen();
        port->printf("STAT SCR=%d FREE_INT=%u FREE_PSRAM=%u SSR=%d ELEM=%d TBL=%d PL=%d PR=%d TEMP=%.1f TARGET=%.1f\n",
                     (int)eez_flow_get_current_screen(),
                     (unsigned)free_int, (unsigned)free_psram,
                     (SSR_1 >= 0 ? digitalRead(SSR_1) : -1),
                     (ELEMENT >= 0 ? digitalRead(ELEMENT) : -1),
                     (TABLE >= 0 ? digitalRead(TABLE) : -1),
                     (PRESS_LEFT >= 0 ? digitalRead(PRESS_LEFT) : -1),
                     (PRESS_RIGHT >= 0 ? digitalRead(PRESS_RIGHT) : -1),
                     (float)get_var_element_temperature(),
                     (float)get_var_element_temperature_target());
    }
    else if (strncmp(cmd, "MEM", 3) == 0) {
        size_t free_int = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t min_int = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        port->printf("MEM FREE_INT=%u MIN_INT=%u FREE_PSRAM=%u\n",
                     (unsigned)free_int, (unsigned)min_int, (unsigned)free_psram);
    }
    else if (strncmp(cmd, "SAFE", 4) == 0) {
        agent_safe_stop(port);
    }
    else if (strncmp(cmd, "SHOT", 4) == 0) {
        shot_request(port);
    }
    else if (strncmp(cmd, "TEST_MODAL", 10) == 0) {
        g_warranty_expired_dialog_request = true;
        port->println("OK TEST_MODAL");
    }
    else if (strncmp(cmd, "START", 5) == 0 || strncmp(cmd, "PEDAL", 5) == 0) {
        pedal_operation();
        port->printf("OK START step=%d proc=%d started=%d\n",
                     (int)get_var_step_number(), (int)get_var_step_process(), (int)get_var_cycel_started());
    }
    else if (strncmp(cmd, "CYCLE", 5) == 0) {
        port->printf("CYCLE step=%d proc=%d started=%d temp=%.1f target=%.1f sensor_en=%d templim=%d pedal3=%d\n",
                     (int)get_var_step_number(), (int)get_var_step_process(), (int)get_var_cycel_started(),
                     (float)get_var_element_temperature(), (float)get_var_element_temperature_target(),
                     (int)get_var_setting_admin_jack_sensor(),
                     (int)get_var_setting_general_temperature_limit(),
                     (int)get_var_setting_general_pedal_number3());
    }
    else if (strncmp(cmd, "TEST_CYCLE", 10) == 0) {
        set_var_progress_vlaue(1);
        set_var_step_number(0);
        set_var_step_process(true);
        set_var_cycel_started(true);
        port->println("OK TEST_CYCLE STARTED");
    }
    else if (strncmp(cmd, "WARR_MODAL", 10) == 0 || strncmp(cmd, "WARRANTY_MODAL", 14) == 0) {
        g_warranty_screen_request = true;
        port->println("OK WARR_MODAL");
    }
    else if (strncmp(cmd, "MFG_MODAL", 9) == 0) {
        g_mfg_setup_request = true;
        port->println("OK MFG_MODAL");
    }
    else if (strncmp(cmd, "CLOSE_MODAL", 11) == 0) {
        hide_warranty_expired_dialog();
        close_warranty_screen();
        port->println("OK CLOSE_MODAL");
    }
    else if (strncmp(cmd, "NAV", 3) == 0 || strncmp(cmd, "GOTO", 4) == 0) {
        const char *arg = strchr(cmd, ' ');
        while (arg && *arg == ' ') arg++;
        int target = agent_resolve_screen_name(arg);
        if (target == 111 || (target >= 1 && target <= 11)) {
            g_agent_nav_request = target;
            port->printf("OK NAV %d\n", target);
        } else {
            port->println("ERR INVALID_SCREEN");
        }
    }
    else if (strncmp(cmd, "LANG", 4) == 0 || strncmp(cmd, "SET_LANG", 8) == 0) {
        const char *arg = strchr(cmd, ' ');
        while (arg && *arg == ' ') arg++;
        int lang_id = -1;
        if (arg) {
            if (strcasecmp(arg, "FA") == 0 || strcmp(arg, "1") == 0) lang_id = 1;
            else if (strcasecmp(arg, "EN") == 0 || strcmp(arg, "2") == 0) lang_id = 2;
            else if (strcasecmp(arg, "TR") == 0 || strcmp(arg, "3") == 0) lang_id = 3;
        }
        if (lang_id >= 1 && lang_id <= 3) {
            apply_language_now(lang_id);
            port->printf("OK LANG %d\n", lang_id);
        } else {
            port->println("ERR INVALID_LANG");
        }
    }
    else if (strncmp(cmd, "PRESS", 5) == 0) {
        int x = agent_arg_int(cmd, 1, -1);
        int y = agent_arg_int(cmd, 2, -1);
        if (x >= 0 && x < SHOT_W && y >= 0 && y < SHOT_H) {
            g_agent_touch_x = (int16_t)x;
            g_agent_touch_y = (int16_t)y;
            g_agent_touch_active = true;
            g_agent_touch_release_ms = 0;
            port->printf("OK PRESS %d %d\n", x, y);
        } else {
            port->println("ERR INVALID_COORDS");
        }
    }
    else if (strncmp(cmd, "REL", 3) == 0) {
        g_agent_touch_active = false;
        g_agent_touch_release_ms = 0;
        port->println("OK REL");
    }
    else if (strncmp(cmd, "TAP", 3) == 0) {
        int x = agent_arg_int(cmd, 1, -1);
        int y = agent_arg_int(cmd, 2, -1);
        int hold_ms = agent_arg_int(cmd, 3, 120);
        if (x >= 0 && x < SHOT_W && y >= 0 && y < SHOT_H) {
            g_agent_touch_x = (int16_t)x;
            g_agent_touch_y = (int16_t)y;
            g_agent_touch_active = true;
            g_agent_touch_release_ms = millis() + (uint32_t)hold_ms;
            port->printf("OK TAP %d %d %d\n", x, y, hold_ms);
        } else {
            port->println("ERR INVALID_COORDS");
        }
    }
    else if (strncmp(cmd, "TEMP", 4) == 0) {
        port->printf("TEMP CUR=%.1f TARGET=%.1f\n",
                     (float)get_var_element_temperature(),
                     (float)get_var_element_temperature_target());
    }
    else if (strncmp(cmd, "TOUCH", 5) == 0) {
        uint8_t chip = 0, addr = 0;
        uint32_t reads = 0, errs = 0;
        display_port_touch_info(&chip, &addr, &reads, &errs);
        port->printf("TOUCH detected=%d chip=%d addr=0x%02X reads=%lu errs=%lu active=%d last_x=%d last_y=%d\n",
                     display_port_touch_detected() ? 1 : 0,
                     (int)chip, (unsigned)addr,
                     (unsigned long)reads, (unsigned long)errs,
                     g_agent_touch_active ? 1 : 0,
                     (int)g_agent_touch_x, (int)g_agent_touch_y);
    }
    else if (strncmp(cmd, "WIFI_TEST", 9) == 0) {
        char target_ssid[33] = {0};
        char target_pass[65] = {0};
        const char *p1 = cmd + 9;
        while (*p1 == ' ') p1++;
        const char *space = strchr(p1, ' ');
        if (space) {
            size_t slen = space - p1;
            if (slen > 32) slen = 32;
            strncpy(target_ssid, p1, slen);
            const char *p2 = space + 1;
            while (*p2 == ' ') p2++;
            strncpy(target_pass, p2, 64);
        } else {
            strncpy(target_ssid, p1, 32);
        }
        extern void agent_wifi_test(Stream *port, const char *ssid, const char *pass);
        agent_wifi_test(port, target_ssid, target_pass);
    }
    else if (strncmp(cmd, "WIFI_GET", 8) == 0) {
        extern void agent_cmd_wifi_get(Stream *port);
        agent_cmd_wifi_get(port);
    }
    else if (strncmp(cmd, "WIFI_SET", 8) == 0) {
        char target_ssid[33] = {0};
        char target_pass[65] = {0};
        const char *p1 = cmd + 8;
        while (*p1 == ' ') p1++;
        const char *space = strchr(p1, ' ');
        if (space) {
            size_t slen = space - p1;
            if (slen > 32) slen = 32;
            strncpy(target_ssid, p1, slen);
            const char *p2 = space + 1;
            while (*p2 == ' ') p2++;
            strncpy(target_pass, p2, 64);
        } else {
            strncpy(target_ssid, p1, 32);
        }
        extern void agent_cmd_wifi_set(Stream *port, const char *ssid, const char *pass);
        agent_cmd_wifi_set(port, target_ssid, target_pass);
    }
    else if (strncmp(cmd, "DO_UPDATE", 9) == 0) {
        extern void agent_cmd_do_update(Stream *port);
        agent_cmd_do_update(port);
    }
    else if (strncmp(cmd, "RESET_ONBOARDING", 16) == 0) {
        extern void agent_cmd_reset_onboarding(Stream *port);
        agent_cmd_reset_onboarding(port);
    }
    else {
        port->println("ERR UNKNOWN_COMMAND");
    }
}

static inline void agent_process_cmd(Stream *port, const char *cmd)
{
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    // پروتکل امنیتی رمزنگاری‌شده با HMAC-SHA256:
    // $WELD,<SEQ>,<NONCE>,<CMD>,<PAYLOAD>*<HMAC>\r\n
    if (strncmp(cmd, "$WELD,", 6) == 0) {
        const char *star = strrchr(cmd, '*');
        if (!star) {
            port->println("ERR SEC_MALFORMED");
            return;
        }
        const char *sig_hex = star + 1;
        if (strlen(sig_hex) < 64) {
            port->println("ERR SEC_INVALID_SIG_LEN");
            return;
        }
        const char *payload_start = cmd + 6;
        size_t payload_len = (size_t)(star - payload_start);
        if (!agent_verify_hmac(payload_start, payload_len, MASTER_SECRET_KEY, sig_hex)) {
            port->println("ERR SEC_AUTH_FAILED");
            return;
        }
        // بررسی محافظت در برابر بازپخش بسته (Anti-Replay Protection)
        static uint32_t s_last_seq = 0;
        uint32_t seq = (uint32_t)strtoul(payload_start, NULL, 16);
        if (seq <= s_last_seq && s_last_seq != 0) {
            port->println("ERR SEC_REPLAY_DETECTED");
            return;
        }
        s_last_seq = seq;

        char inner[128];
        if (payload_len >= sizeof(inner)) payload_len = sizeof(inner) - 1;
        memcpy(inner, payload_start, payload_len);
        inner[payload_len] = '\0';

        strtok(inner, ","); // seq
        strtok(NULL, ",");  // nonce
        char *cmd_token = strtok(NULL, ",");
        char *arg_token = strtok(NULL, "");

        if (!cmd_token) {
            port->println("ERR SEC_NO_CMD");
            return;
        }

        char sub_cmd[128];
        if (arg_token && *arg_token) {
            snprintf(sub_cmd, sizeof(sub_cmd), "%s %s", cmd_token, arg_token);
        } else {
            snprintf(sub_cmd, sizeof(sub_cmd), "%s", cmd_token);
        }
        agent_execute_unwrapped_cmd(port, sub_cmd);
        return;
    }

#if SERIAL_SEC_BYPASS
    agent_execute_unwrapped_cmd(port, cmd);
#else
    port->println("ERR SEC_UNAUTHORIZED");
#endif
}

static inline void agent_on_flush(const lv_area_t *area, const uint8_t *px_map, bool last)
{
    shot_on_flush(area, (const uint16_t *)px_map, last);
}

static inline void agent_shot_arm_before_render()
{
    shot_poll();
}

static inline void agent_hmi_tick(Stream *port)
{
    (void)port;
    if (g_agent_touch_release_ms > 0 && millis() >= g_agent_touch_release_ms) {
        g_agent_touch_active = false;
        g_agent_touch_release_ms = 0;
    }
}

static inline void agent_hmi_poll(Stream *port)
{
    if (!port) return;

    static char g_agent_cmd_buf[160];
    static size_t g_agent_cmd_len = 0;

    while (port->available() > 0) {
        char c = (char)port->read();
        if (c == '\r') continue;
        if (c == '\n') {
            g_agent_cmd_buf[g_agent_cmd_len] = '\0';
            if (g_agent_cmd_len > 0) {
                agent_process_cmd(port, g_agent_cmd_buf);
                g_agent_cmd_len = 0;
            }
        } else {
            if (g_agent_cmd_len < sizeof(g_agent_cmd_buf) - 1) {
                g_agent_cmd_buf[g_agent_cmd_len++] = c;
            }
        }
    }

    shot_poll();
    agent_hmi_tick(port);
}

static inline void agent_hmi_init()
{
    shot_init_worker();
}
