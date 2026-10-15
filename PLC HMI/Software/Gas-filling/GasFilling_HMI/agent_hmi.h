#pragma once

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include <Preferences.h>
#include <EEPROM.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "screens.h"
#include "ui.h"
#include "variables.h"
#include "version.h"
#include "actions.h"
#include "day_lock.h"
#include "panel_keys.h"
#include "ota_manager.h"
#include <bb_captouch.h>
#include "perf_overlay.h"

extern volatile int16_t g_touch_last_x;
extern volatile int16_t g_touch_last_y;
extern volatile uint8_t g_touch_last_count;
extern volatile int g_touch_sensor;
extern volatile uint32_t g_touch_ok_count;
extern bool i2c_touch_ok;
extern BBCapTouch bbct;
extern bool gt911_read_touch(int16_t *x, int16_t *y, uint8_t *count);
extern void gt911_debug_watch(Stream *port);

#ifndef AGENT_HMI_BAUD
#define AGENT_HMI_BAUD 921600
#endif

#define AGENT_SHOT_W 480
#define AGENT_SHOT_H 320

static uint16_t *agent_shot_buf = nullptr;
static volatile uint8_t agent_shot_state = 0;  // 0 idle, 1 requested, 2 capture, 3 ready for bg stream
static Stream *agent_shot_port = nullptr;
static uint32_t agent_shot_started_ms = 0;
static TaskHandle_t agent_shot_task_handle = nullptr;

static bool agent_touch_active = false;
static bool agent_touch_tap = false;
static int16_t agent_touch_x = 0;
static int16_t agent_touch_y = 0;
static uint32_t agent_touch_release_ms = 0;

static inline int agent_arg_int(const char *text, int index, int fallback)
{
    const char *p = text;
    for (int i = 0; i < index; i++) {
        p = strchr(p, ' ');
        if (!p) return fallback;
        while (*p == ' ') p++;
    }
    if (!*p) return fallback;
    return atoi(p);
}

#ifndef AGENT_SEC_KEY
#define AGENT_SEC_KEY "HAFFLER-SEC-2026-X9"
#endif

static bool agent_authenticated = false;
static uint32_t agent_auth_expiry_ms = 0;

static inline bool agent_is_authenticated()
{
    if (!agent_authenticated) return false;
    if ((int32_t)(millis() - agent_auth_expiry_ms) > 0) {
        agent_authenticated = false;
        return false;
    }
    return true;
}

static inline void agent_print_help(Stream *port)
{
    port->println("OK HELP GasFilling_HMI Serial Agent");
    port->println("PING");
    port->println("AUTH <key>");
    port->println("LOCK");
    port->println("STAT");
    port->println("SHOT (Async Zero-Lag 921600 baud)");
    port->println("NAV [MAIN|SET|TAB <0..3>|LANG <0..3>|MODAL <OTA|HELP>]");
    port->println("TAP x y [ms]");
    port->println("PRESS x y");
    port->println("REL");
    port->println("SAFE");
    port->println("PULSE1 ms");
    port->println("PULSE2 ms");
    port->println("BUZ ms");
    port->println("TIME");
    port->println("TIMESET YYYY MM DD HH MM SS");
    port->println("BAR pct [0|1|2]");
    port->println("TOUCH / TOUCHWATCH / TOUCHRAW");
}

static void agent_shot_worker_task(void *pvParameters)
{
    (void)pvParameters;
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static char line[AGENT_SHOT_W * 2 * 4 / 3 + 4];

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!agent_shot_buf || !agent_shot_port) {
            agent_shot_state = 0;
            continue;
        }

        Stream *p = agent_shot_port;
        p->printf("SHOT_BEGIN %d %d RGB565LE\n", AGENT_SHOT_W, AGENT_SHOT_H);
        p->flush();

        const uint8_t *raw = (const uint8_t *)agent_shot_buf;
        for (int y = 0; y < AGENT_SHOT_H; y++) {
            const uint8_t *row = &raw[y * AGENT_SHOT_W * 2];
            int o = 0;
            for (int i = 0; i < AGENT_SHOT_W * 2; i += 3) {
                uint32_t v = ((uint32_t)row[i] << 16) | ((uint32_t)row[i + 1] << 8) | row[i + 2];
                line[o++] = b64[(v >> 18) & 63];
                line[o++] = b64[(v >> 12) & 63];
                line[o++] = b64[(v >> 6) & 63];
                line[o++] = b64[v & 63];
            }
            line[o++] = '\n';
            p->write((const uint8_t *)line, o);
            // Yield every 4 rows for continuous sub-4s streaming with zero UI lag
            if ((y & 3) == 3) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        p->println("SHOT_END");
        p->flush();

        agent_shot_port = nullptr;
        agent_shot_state = 0;
    }
}

static inline void agent_request_shot(Stream *port)
{
    if (agent_shot_state != 0) {
        port->println("BUSY SHOT");
        return;
    }
    agent_shot_port = port;
    agent_shot_state = 1;
}

static inline void agent_on_flush(const lv_area_t *area, const uint16_t *px, bool last)
{
    if (agent_shot_state != 2 || !agent_shot_buf) return;

    int x1 = area->x1 < 0 ? 0 : area->x1;
    int x2 = area->x2 >= AGENT_SHOT_W ? AGENT_SHOT_W - 1 : area->x2;
    int w_src = area->x2 - area->x1 + 1;
    if (x2 < x1 || w_src <= 0) return;

    for (int y = area->y1; y <= area->y2; y++) {
        if (y < 0 || y >= AGENT_SHOT_H) {
            px += w_src;
            continue;
        }
        uint16_t *dst = &agent_shot_buf[y * AGENT_SHOT_W + x1];
        const uint16_t *src = px + (x1 - area->x1);
        int copy_w = x2 - x1 + 1;
#if defined(LV_COLOR_16_SWAP) && LV_COLOR_16_SWAP
        for (int x = 0; x < copy_w; x++) {
            uint16_t c = src[x];
            dst[x] = (uint16_t)((c << 8) | (c >> 8));
        }
#else
        memcpy(dst, src, copy_w * sizeof(uint16_t));
#endif
        px += w_src;
    }

    if (last) {
        agent_shot_state = 3;
        if (agent_shot_task_handle) {
            xTaskNotifyGive(agent_shot_task_handle);
        }
    }
}

static inline bool agent_read_touch(lv_indev_data_t *data)
{
    if (!agent_touch_active) return false;

    if (!agent_touch_tap && agent_touch_release_ms != 0 && millis() >= agent_touch_release_ms) {
        agent_touch_active = false;
        agent_touch_tap = false;
        agent_touch_release_ms = 0;
        data->state = LV_INDEV_STATE_REL;
        data->point.x = agent_touch_x;
        data->point.y = agent_touch_y;
        return true;
    }

    if (agent_touch_tap && millis() >= agent_touch_release_ms) {
        agent_touch_active = false;
        agent_touch_tap = false;
        agent_touch_release_ms = 0;
        data->state = LV_INDEV_STATE_REL;
        data->point.x = agent_touch_x;
        data->point.y = agent_touch_y;
        return true;
    }

    data->state = LV_INDEV_STATE_PR;
    data->point.x = agent_touch_x;
    data->point.y = agent_touch_y;
    return true;
}

static inline void agent_safe_outputs()
{
    TimerValve1 = 0;
    TimerValve2 = 0;
    buzzerTimeVlaue = 0;
    digitalWrite(16, !vr_OutPutType);
    digitalWrite(42, !vr_OutPutType);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(45, 0);
#else
    ledcWrite(0, 0);
#endif
}

static inline void agent_handle_command(char *cmd, Stream *port)
{
    while (*cmd == ' ') cmd++;
    size_t len = strlen(cmd);
    while (len && (cmd[len - 1] == ' ' || cmd[len - 1] == '\t')) cmd[--len] = 0;
    if (!len) return;

    if (!strcasecmp(cmd, "HELP") || !strcmp(cmd, "?")) {
        agent_print_help(port);
        return;
    } else if (!strcasecmp(cmd, "PING")) {
        port->printf("PONG GasFilling_HMI AGENT=1 AUTH=%s VER=%s\n",
                     agent_is_authenticated() ? "UNLOCKED" : "LOCKED", FW_VERSION);
        return;
    } else if (!strncasecmp(cmd, "AUTH ", 5)) {
        const char *key = cmd + 5;
        while (*key == ' ') key++;
        if (!strcmp(key, AGENT_SEC_KEY)) {
            agent_authenticated = true;
            agent_auth_expiry_ms = millis() + 3600000;
            port->println("OK AUTH LEVEL=ADMIN TIMEOUT=3600s");
        } else {
            port->println("ERR AUTH INVALID_KEY");
        }
        return;
    } else if (!strcasecmp(cmd, "LOCK")) {
        agent_authenticated = false;
        agent_auth_expiry_ms = 0;
        port->println("OK LOCKED");
        return;
    }

    // Industrial Determinism Security Gate: reject unauthorized serial commands
    if (!agent_is_authenticated()) {
        port->println("ERR AUTH_REQUIRED (Session locked. Send AUTH <key> to unlock)");
        return;
    }
    // Refresh active session lease
    agent_auth_expiry_ms = millis() + 3600000;

    if (!strcasecmp(cmd, "STAT")) {
        port->printf("STAT ver=%s screen=%d t1=%u/%u t2=%u/%u logo=%u buz=%u buz_en=%u outtype=%d heap=%u psram=%u shot=%u touch_ok=%u sensor=%d last=%d,%d n=%u hits=%u inj=%u\n",
                     FW_VERSION, ui_current_screen_id(), TimerValve1, TimerValve1_Saved, TimerValve2, TimerValve2_Saved,
                     TimerLogo, buzzerTimeVlaue, buzzerEnable, (int)vr_OutPutType,
                     (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram(), agent_shot_state,
                     (unsigned)i2c_touch_ok, g_touch_sensor, (int)g_touch_last_x, (int)g_touch_last_y,
                     (unsigned)g_touch_last_count, (unsigned)g_touch_ok_count, (unsigned)agent_touch_active);
    } else if (!strcasecmp(cmd, "WIFI_TEST")) {
        port->printf("WIFI_TEST start mode=%d\n", (int)WiFi.getMode());
        int16_t n = WiFi.scanNetworks(false, false, false, 250, 0);
        port->printf("WIFI_TEST done res=%d\n", (int)n);
        for (int i = 0; i < n && i < 10; i++) {
            port->printf("  [%d] SSID='%s' RSSI=%d\n", i, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        }
        WiFi.scanDelete();
    } else if (!strncasecmp(cmd, "NAV ", 4)) {
        const char *sub = cmd + 4;
        while (*sub == ' ') sub++;
        if (!strcasecmp(sub, "MAIN") || !strcmp(sub, "1")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_MAIN);
            port->println("OK NAV MAIN");
        } else if (!strcasecmp(sub, "SET") || !strcasecmp(sub, "SETTING") || !strcmp(sub, "2")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            port->println("OK NAV SET");
        } else if (!strcasecmp(sub, "WIFI") || !strcmp(sub, "3")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_WIFI);
            port->println("OK NAV WIFI");
        } else if (!strcasecmp(sub, "FW") || !strcasecmp(sub, "FRAMWARE") || !strcasecmp(sub, "FIRMWARE") || !strcmp(sub, "4")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_FRAMWARE);
            port->println("OK NAV FW");
        } else if (!strcasecmp(sub, "WIRING") || !strcasecmp(sub, "PINOUT")) {
            ui_close_all_modals();
            ui_pinout_diagram_open(NULL);
            port->println("OK NAV WIRING");
        } else if (!strcasecmp(sub, "QS")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 0, LV_ANIM_OFF);
            action_select_as_key_board_earia_qs(NULL);
            port->println("OK NAV QS");
        } else if (!strncasecmp(sub, "TAB ", 4)) {
            int idx = agent_arg_int(sub, 1, 0);
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting && idx >= 0 && idx < 3) {
                lv_tabview_set_active(objects.tabview_setting, idx, LV_ANIM_OFF);
                port->printf("OK NAV TAB %d\n", idx);
            } else {
                port->println("OK NAV SET");
            }
        } else if (!strncasecmp(sub, "LANG ", 5)) {
            const char *lstr = sub + 5;
            while (*lstr == ' ') lstr++;
            int lid = 0;
            if (!strcasecmp(lstr, "fa") || !strcmp(lstr, "0")) lid = 0;
            else if (!strcasecmp(lstr, "en") || !strcmp(lstr, "1")) lid = 1;
            else if (!strcasecmp(lstr, "tr") || !strcmp(lstr, "2")) lid = 2;
            else if (!strcasecmp(lstr, "ru") || !strcmp(lstr, "3")) lid = 3;
            ChangeLanguage((char)lid);
            port->printf("OK NAV LANG %d\n", lid);
        } else if (!strcasecmp(sub, "ADMIN") || !strcasecmp(sub, "ADMIN_MENU") || !strcasecmp(sub, "ADMIN_OPTIONS")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 2, LV_ANIM_OFF);
            ui_admin_show_options();
            port->println("OK NAV ADMIN_MENU");
        } else if (!strcasecmp(sub, "MODAL DATETIME") || !strcasecmp(sub, "DATETIME")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 2, LV_ANIM_OFF);
            ui_admin_open_datetime_modal();
            port->println("OK NAV MODAL DATETIME");
        } else if (!strcasecmp(sub, "MODAL DAYLOCK") || !strcasecmp(sub, "DAYLOCK")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 2, LV_ANIM_OFF);
            ui_admin_open_daylock_modal();
            port->println("OK NAV MODAL DAYLOCK");
        } else if (!strcasecmp(sub, "MODAL NUMPAD") || !strcasecmp(sub, "NUMPAD")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 2, LV_ANIM_OFF);
            ui_daylock_open_numpad();
            port->println("OK NAV MODAL NUMPAD");
        } else if (!strcasecmp(sub, "MODAL STATS") || !strcasecmp(sub, "STATS")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_SETTING);
            if (objects.tabview_setting) lv_tabview_set_active(objects.tabview_setting, 2, LV_ANIM_OFF);
            ui_admin_open_stats_modal();
            port->println("OK NAV MODAL STATS");
        } else if (!strcasecmp(sub, "MODAL OTA")) {
            ui_close_all_modals();
            ui_open_ota_modal();
            port->println("OK NAV MODAL OTA");
        } else if (!strcasecmp(sub, "MODAL HELP")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_MAIN);
            ui_setting_help_open(NULL);
            port->println("OK NAV MODAL HELP");
        } else if (!strcasecmp(sub, "LOCK") || !strcasecmp(sub, "DEV_LOCK") || !strcasecmp(sub, "MODAL LOCK")) {
            ui_close_all_modals();
            dev_lock_ui_show();
            port->println("OK NAV DEV_LOCK");
        } else if (!strcasecmp(sub, "UNLOCK")) {
            day_lock_clear_after_unlock();
            dev_lock_ui_hide();
            ui_close_all_modals();
            loadScreen(SCREEN_ID_MAIN);
            port->println("OK NAV UNLOCK");
        } else if (!strcasecmp(sub, "LINK") || !strcasecmp(sub, "MODAL LINK")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_FRAMWARE);
            ui_open_link_entry_modal();
            port->println("OK NAV MODAL LINK");
        } else if (!strcasecmp(sub, "WIFI_PW") || !strcasecmp(sub, "MODAL WIFI_PW")) {
            ui_close_all_modals();
            loadScreen(SCREEN_ID_WIFI);
            ui_open_wifi_pw_modal();
            port->println("OK NAV MODAL WIFI_PW");
        } else if (!strcasecmp(sub, "MODAL CLOSE") || !strcasecmp(sub, "CLOSE")) {
            ui_close_all_modals();
            port->println("OK NAV MODAL CLOSE");
        } else {
            port->println("ERR NAV UNKNOWN_TARGET");
        }
    } else if (!strcasecmp(cmd, "TOUCH") || !strcasecmp(cmd, "TOUCHDIAG")) {
        agent_touch_active = false;
        agent_touch_tap = false;
        agent_touch_release_ms = 0;
        TOUCHINFO ti;
        memset(&ti, 0, sizeof(ti));
        int got = bbct.getSamples(&ti);
        port->printf("OK TOUCH sensor=%d init_ok=%u got=%d count=%d x=%d y=%d last=%d,%d hits=%u\n",
                     g_touch_sensor, (unsigned)i2c_touch_ok, got, (int)ti.count,
                     (got && ti.count > 0) ? (int)ti.x[0] : -1,
                     (got && ti.count > 0) ? (int)ti.y[0] : -1,
                     (int)g_touch_last_x, (int)g_touch_last_y, (unsigned)g_touch_ok_count);
    } else if (!strcasecmp(cmd, "TOUCHWATCH")) {
        uint32_t until = millis() + 8000;
        uint32_t hits = 0;
        int16_t last_x = -1, last_y = -1;
        uint8_t count = 0;
        agent_touch_active = false;
        while ((int32_t)(millis() - until) < 0) {
            int16_t x, y;
            uint8_t n;
            if (g_touch_sensor == CT_TYPE_GT911 && gt911_read_touch(&x, &y, &n)) {
                hits++;
                last_x = x;
                last_y = y;
                count = n;
            }
            delay(15);
        }
        port->printf("OK TOUCHWATCH sensor=%d hits=%u count=%u x=%d y=%d\n",
                     g_touch_sensor, (unsigned)hits, (unsigned)count, (int)last_x, (int)last_y);
    } else if (!strcasecmp(cmd, "TOUCHRAW")) {
        agent_touch_active = false;
        gt911_debug_watch(port);
    } else if (!strcasecmp(cmd, "SHOT")) {
        agent_request_shot(port);
    } else if (!strcasecmp(cmd, "SCR1") || !strcasecmp(cmd, "MAIN")) {
        loadScreen(SCREEN_ID_MAIN);
        port->println("OK SCR1");
    } else if (!strcasecmp(cmd, "SCR2") || !strcasecmp(cmd, "SET") || !strcasecmp(cmd, "SETTING")) {
        loadScreen(SCREEN_ID_SETTING);
        port->println("OK SCR2");
    } else if (!strncasecmp(cmd, "TAP ", 4)) {
        int x = agent_arg_int(cmd, 1, -1);
        int y = agent_arg_int(cmd, 2, -1);
        int ms = agent_arg_int(cmd, 3, 120);
        if (x < 0 || y < 0 || x >= AGENT_SHOT_W || y >= AGENT_SHOT_H) {
            port->println("ERR TAP range");
        } else {
            agent_touch_x = (int16_t)x;
            agent_touch_y = (int16_t)y;
            agent_touch_tap = true;
            agent_touch_active = true;
            agent_touch_release_ms = millis() + (uint32_t)(ms < 20 ? 20 : ms);
            port->printf("OK TAP %d %d %d\n", x, y, ms);
        }
    } else if (!strncasecmp(cmd, "PRESS ", 6)) {
        int x = agent_arg_int(cmd, 1, -1);
        int y = agent_arg_int(cmd, 2, -1);
        if (x < 0 || y < 0 || x >= AGENT_SHOT_W || y >= AGENT_SHOT_H) {
            port->println("ERR PRESS range");
        } else {
            agent_touch_x = (int16_t)x;
            agent_touch_y = (int16_t)y;
            agent_touch_tap = false;
            agent_touch_active = true;
            agent_touch_release_ms = millis() + 3000;
            port->printf("OK PRESS %d %d\n", x, y);
        }
    } else if (!strcasecmp(cmd, "REL")) {
        agent_touch_active = false;
        agent_touch_tap = false;
        agent_touch_release_ms = 0;
        port->println("OK REL");
    } else if (!strncasecmp(cmd, "KEY ", 4)) {
        const char *kname = cmd + 4;
        while (*kname == ' ') kname++;
        panel_key_id_t kid = KEY_NONE;
        if (!strcasecmp(kname, "F1")) kid = KEY_F1;
        else if (!strcasecmp(kname, "F2")) kid = KEY_F2;
        else if (!strcasecmp(kname, "F3")) kid = KEY_F3;
        else if (!strcasecmp(kname, "F4")) kid = KEY_F4;
        else if (!strcasecmp(kname, "F5")) kid = KEY_F5;
        else if (!strcasecmp(kname, "MODE")) kid = KEY_MODE;
        else if (!strcasecmp(kname, "START")) kid = KEY_START;
        else if (!strcasecmp(kname, "STOP")) kid = KEY_STOP;
        else if (!strcasecmp(kname, "CANCEL")) kid = KEY_CANCEL;
        else if (!strcasecmp(kname, "SAVE")) kid = KEY_SAVE;
        if (kid != KEY_NONE) {
            panel_keys_trigger_key(kid);
            port->printf("OK KEY %s\n", kname);
        } else {
            port->println("ERR KEY UNKNOWN");
        }
    } else if (!strcasecmp(cmd, "SAFE")) {
        agent_safe_outputs();
        port->println("OK SAFE");
    } else if (!strncasecmp(cmd, "PULSE1 ", 7)) {
        int ms = agent_arg_int(cmd, 1, 100);
        TimerValve1 = (uint32_t)(ms < 10 ? 10 : (ms > 30000 ? 30000 : ms));
        digitalWrite(16, vr_OutPutType);
        port->printf("OK PULSE1 %u\n", TimerValve1);
    } else if (!strncasecmp(cmd, "PULSE2 ", 7)) {
        int ms = agent_arg_int(cmd, 1, 100);
        TimerValve2 = (uint32_t)(ms < 10 ? 10 : (ms > 30000 ? 30000 : ms));
        digitalWrite(42, vr_OutPutType);
        port->printf("OK PULSE2 %u\n", TimerValve2);
    } else if (!strncasecmp(cmd, "BUZ ", 4)) {
        int ms = agent_arg_int(cmd, 1, 50);
        buzzerTimeVlaue = (uint32_t)(ms < 10 ? 10 : (ms > 5000 ? 5000 : ms));
        port->printf("OK BUZ %u\n", buzzerTimeVlaue);
    } else if (!strcasecmp(cmd, "TIME")) {
        time_t now = time(nullptr);
        struct tm t;
        localtime_r(&now, &t);
        port->printf("TIME %04d-%02d-%02d %02d:%02d:%02d\n",
                     t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                     t.tm_hour, t.tm_min, t.tm_sec);
    } else if (!strncasecmp(cmd, "TIMESET ", 8)) {
        int Y = agent_arg_int(cmd, 1, 2026);
        int M = agent_arg_int(cmd, 2, 1);
        int D = agent_arg_int(cmd, 3, 1);
        int h = agent_arg_int(cmd, 4, 12);
        int m = agent_arg_int(cmd, 5, 0);
        int s = agent_arg_int(cmd, 6, 0);
        struct tm t = {0};
        t.tm_year = Y - 1900;
        t.tm_mon = M - 1;
        t.tm_mday = D;
        t.tm_hour = h;
        t.tm_min = m;
        t.tm_sec = s;
        time_t epoch = mktime(&t);
        struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
        settimeofday(&tv, nullptr);
        port->println("OK TIMESET");
    } else if (!strcasecmp(cmd, "PERF")) {
        perf_qa_start(port, false);
    } else if (!strcasecmp(cmd, "PERF CONT") || !strcasecmp(cmd, "PERF ON")) {
        perf_qa_start(port, true);
    } else if (!strcasecmp(cmd, "PERF OFF") || !strcasecmp(cmd, "PERF STOP")) {
        perf_qa_stop("stopped_by_user");
    } else if (!strncasecmp(cmd, "BAR ", 4)) {
        int pct = agent_arg_int(cmd, 1, 50);
        int which = agent_arg_int(cmd, 2, 0);
        if (pct < 0) pct = 0; if (pct > 100) pct = 100;
        if (which == 0 || which == 1) ChangeBar((unsigned char)pct);
        if (which == 0 || which == 2) ChangeBar1((unsigned char)pct);
        port->printf("OK BAR %d %d\n", pct, which);
    } else if (!strcasecmp(cmd, "WIFI_INFO")) {
        char s[33] = {0}, p[65] = {0};
        ota_manager_load_wifi_credentials(s, sizeof(s), p, sizeof(p));
        Preferences pr;
        bool b_ok = pr.begin("haffler_cfg", true);
        String direct_s = pr.getString("wifi_ssid", "<NONE>");
        String direct_p = pr.getString("wifi_pass", "<NONE>");
        pr.end();
        port->printf("WIFI status=%d ip=%s ssid=\"%s\" saved_ssid=\"%s\" nvs_ok=%d direct_s=\"%s\" direct_p=\"%s\"\n",
                     (int)WiFi.status(), WiFi.localIP().toString().c_str(),
                     WiFi.SSID().c_str(), s, (int)b_ok, direct_s.c_str(), direct_p.c_str());
    } else if (!strncasecmp(cmd, "WIFI_SET ", 9)) {
        char s[33] = {0}, p[65] = {0};
        const char *arg = cmd + 9;
        while (*arg == ' ') arg++;
        const char *sp = strchr(arg, ' ');
        if (sp) {
            size_t s_len = sp - arg;
            if (s_len > sizeof(s) - 1) s_len = sizeof(s) - 1;
            strncpy(s, arg, s_len);
            s[s_len] = '\0';
            const char *pass_arg = sp + 1;
            while (*pass_arg == ' ') pass_arg++;
            strncpy(p, pass_arg, sizeof(p) - 1);
            p[sizeof(p) - 1] = '\0';
        } else {
            strncpy(s, arg, sizeof(s) - 1);
            s[sizeof(s) - 1] = '\0';
        }
        Preferences pr;
        bool b_ok = pr.begin("haffler_cfg", false);
        size_t n1 = pr.putString("wifi_ssid", s);
        size_t n2 = pr.putString("wifi_pass", p);
        pr.end();
        ota_manager_save_wifi_credentials(s, p);
        port->printf("OK WIFI_SET ssid=\"%s\" b_ok=%d n1=%u n2=%u\n", s, (int)b_ok, (unsigned)n1, (unsigned)n2);
    } else if (!strcasecmp(cmd, "NVS_DIAG")) {
        esp_err_t err_init = nvs_flash_init();
        nvs_handle_t h;
        esp_err_t err_cfg = nvs_open("haffler_cfg", NVS_READWRITE, &h);
        if (err_cfg == ESP_OK) nvs_close(h);
        esp_err_t err_lock = nvs_open("haffler_lock", NVS_READWRITE, &h);
        if (err_lock == ESP_OK) nvs_close(h);
        esp_err_t err_hw = nvs_open("haffler_hw", NVS_READWRITE, &h);
        if (err_hw == ESP_OK) nvs_close(h);
        nvs_stats_t stats;
        esp_err_t err_stats = nvs_get_stats("nvs", &stats);
        port->printf("NVS_DIAG init=%d(%s) cfg=%d(%s) lock=%d(%s) hw=%d(%s) stats=%d used=%u free=%u total=%u\n",
                     err_init, esp_err_to_name(err_init),
                     err_cfg, esp_err_to_name(err_cfg),
                     err_lock, esp_err_to_name(err_lock),
                     err_hw, esp_err_to_name(err_hw),
                     err_stats, (unsigned)stats.used_entries, (unsigned)stats.free_entries, (unsigned)stats.total_entries);
        nvs_iterator_t it = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY);
        int count = 0;
        while (it != NULL && count < 40) {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info);
            it = nvs_entry_next(it);
            port->printf("  [NVS] ns='%s' key='%s'\n", info.namespace_name, info.key);
            count++;
        }
        port->printf("NVS_DIAG_DONE count=%d\n", count);
    } else if (!strcasecmp(cmd, "NVS_NAMESPACES")) {
        nvs_iterator_t it = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY);
        char seen[32][16];
        int seen_count = 0;
        int total = 0;
        while (it != NULL) {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info);
            it = nvs_entry_next(it);
            total++;
            bool found = false;
            for (int i = 0; i < seen_count; i++) {
                if (!strcmp(seen[i], info.namespace_name)) {
                    found = true;
                    break;
                }
            }
            if (!found && seen_count < 32) {
                strncpy(seen[seen_count++], info.namespace_name, 15);
                seen[seen_count - 1][15] = 0;
            }
        }
        port->printf("TOTAL_ENTRIES=%d UNIQUE_NAMESPACES=%d\n", total, seen_count);
        for (int i = 0; i < seen_count; i++) {
            port->printf("  NS[%d] = '%s'\n", i, seen[i]);
        }
        port->println("OK NVS_NAMESPACES");
    } else if (!strcasecmp(cmd, "NVS_PURGE_FOREIGN")) {
        const char *foreign[] = {"AJOSH1", "app", "ice_cream", "welding", "cutter", "chone", "haffler_rcp", "bakery_cfg", "haf_lock", "wipe_ctl"};
        int purged = 0;
        for (int i = 0; i < 10; i++) {
            nvs_handle_t h;
            if (nvs_open(foreign[i], NVS_READWRITE, &h) == ESP_OK) {
                nvs_erase_all(h);
                nvs_commit(h);
                nvs_close(h);
                purged++;
                port->printf("PURGED '%s'\n", foreign[i]);
            }
        }
        port->printf("OK NVS_PURGE_FOREIGN count=%d\n", purged);
    } else if (!strcasecmp(cmd, "TEST_HW")) {
        Preferences p;
        bool b = p.begin("haffler_hw", false);
        size_t w = p.putString("test_wifi", "hello_world");
        String s = p.getString("test_wifi", "<NONE>");
        p.end();
        port->printf("TEST_HW b=%d w=%u s='%s'\n", (int)b, (unsigned)w, s.c_str());
    } else if (!strcasecmp(cmd, "TEST_EEPROM")) {
        EEPROM.writeUChar(200, 0x57);
        EEPROM.writeUChar(201, 0x46);
        const char *test_s = "HafflerOffice";
        for (int i = 0; i < 33; i++) EEPROM.writeUChar(202 + i, i < strlen(test_s) ? test_s[i] : 0);
        bool c_ok = EEPROM.commit();
        char read_s[33] = {0};
        for (int i = 0; i < 32; i++) read_s[i] = (char)EEPROM.readUChar(202 + i);
        port->printf("TEST_EEPROM commit=%d read='%s'\n", (int)c_ok, read_s);
    } else {
        port->println("ERR UNKNOWN_CMD");
    }
}

static inline void agent_poll_stream(Stream &port, char *buf, size_t &idx, size_t max_len)
{
    while (port.available() > 0) {
        char c = (char)port.read();
        if (c == '\r') continue;
        if (c == '\n') {
            buf[idx] = 0;
            if (idx > 0) {
                agent_handle_command(buf, &port);
                idx = 0;
            }
        } else if (idx < max_len - 1) {
            buf[idx++] = c;
        } else {
            idx = 0;
        }
    }
}

static inline void agent_begin()
{
    agent_touch_active = false;
    agent_touch_tap = false;
    agent_touch_release_ms = 0;

    if (!agent_shot_buf) {
        agent_shot_buf = (uint16_t *)heap_caps_malloc(AGENT_SHOT_W * AGENT_SHOT_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!agent_shot_buf) {
            agent_shot_buf = (uint16_t *)malloc(AGENT_SHOT_W * AGENT_SHOT_H * sizeof(uint16_t));
        }
    }

    if (!agent_shot_task_handle) {
        xTaskCreatePinnedToCore(agent_shot_worker_task, "ShotWorker", 4096, NULL, 1, &agent_shot_task_handle, 0);
    }

    Serial.begin(AGENT_HMI_BAUD);
    Serial0.begin(AGENT_HMI_BAUD);
    delay(100);
    Serial.println("AGENT_READY GasFilling_HMI");
    Serial0.println("AGENT_READY GasFilling_HMI");
    Serial.flush();
    Serial0.flush();
}

static inline void agent_poll()
{
    static char s_buf_serial[160];
    static size_t s_idx_serial = 0;
    static char s_buf_serial0[160];
    static size_t s_idx_serial0 = 0;

    agent_poll_stream(Serial, s_buf_serial, s_idx_serial, sizeof(s_buf_serial));
    agent_poll_stream(Serial0, s_buf_serial0, s_idx_serial0, sizeof(s_buf_serial0));
}

static inline void agent_before_lvgl()
{
    if (agent_shot_state != 1) return;
    if (!agent_shot_buf) {
        agent_shot_buf = (uint16_t *)heap_caps_malloc(AGENT_SHOT_W * AGENT_SHOT_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!agent_shot_buf) {
            agent_shot_buf = (uint16_t *)malloc(AGENT_SHOT_W * AGENT_SHOT_H * sizeof(uint16_t));
        }
    }
    if (!agent_shot_buf) {
        if (agent_shot_port) agent_shot_port->println("SHOT_ERR alloc_psram");
        agent_shot_state = 0;
        return;
    }

    lv_obj_invalidate(lv_screen_active());
    lv_obj_invalidate(lv_layer_top());
    lv_obj_invalidate(lv_layer_sys());
    agent_shot_started_ms = millis();
    agent_shot_state = 2;
}

static inline void agent_after_lvgl()
{
    if (agent_shot_state == 2) {
        if (millis() - agent_shot_started_ms > 3000) {
            if (agent_shot_port) agent_shot_port->println("SHOT_ERR timeout");
            agent_shot_state = 0;
        }
    }
}
