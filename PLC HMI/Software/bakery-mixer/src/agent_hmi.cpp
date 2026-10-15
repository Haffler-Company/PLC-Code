#include "agent_hmi.h"
#include "haffler_ui_builder.h"
#include "haffler_ui_lang.h"
#include "machine_fsm.h"
#include "screenshot.h"
#include "haffler_pins.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static volatile bool s_touch_injected = false;
static volatile int16_t s_touch_x = 0;
static volatile int16_t s_touch_y = 0;
static volatile uint32_t s_touch_until_ms = 0;

void agent_hmi_init(void) {
    s_touch_injected = false;
    s_touch_until_ms = 0;
}

void agent_hmi_inject_touch(int16_t x, int16_t y, uint32_t duration_ms) {
    s_touch_x = x;
    s_touch_y = y;
    s_touch_until_ms = millis() + (duration_ms > 0 ? duration_ms : 150);
    s_touch_injected = true;
}

bool agent_hmi_get_touch(int16_t *x, int16_t *y) {
    if (!s_touch_injected) return false;
    if (millis() < s_touch_until_ms) {
        if (x) *x = s_touch_x;
        if (y) *y = s_touch_y;
        return true;
    }
    s_touch_injected = false;
    return false;
}

static void print_status_json(Stream &p) {
    const bakery_live_t &live = g_machine_fsm.getLive();
    const bakery_config_t &cfg = g_machine_fsm.getConfig();

    p.printf("{\"state\":\"%s\",\"state_id\":%d,\"remaining_sec\":%u,\"mix_time\":%u,"
             "\"water_liters\":%.1f,\"water_target_l\":%.1f,\"water_temp_c\":%.1f,"
             "\"water_target_temp_c\":%.1f,\"water_ready\":%d,\"guard_ok\":%d,"
             "\"estop_ok\":%d,\"overload_ok\":%d,\"batches\":%u,\"daylock_stage\":%d,"
             "\"daylock_days\":%u,\"daylock_locked\":%d,\"screen\":%d}\n",
             g_machine_fsm.getStateName(),
             (int)g_machine_fsm.getState(),
             (unsigned)live.remaining_mix_sec,
             (unsigned)cfg.mix_time_sec,
             live.current_water_dosed_l,
             cfg.water_target_liters,
             live.water_temp_c,
             cfg.water_target_temp_c,
             live.water_temp_ready ? 1 : 0,
             live.guard_closed ? 1 : 0,
             live.estop_ok ? 1 : 0,
             live.overload_ok ? 1 : 0,
             (unsigned)live.batches_count,
             (int)live.daylock_stage,
             (unsigned)live.daylock_days_left,
             live.daylock_locked ? 1 : 0,
             (int)g_ui_ctx.current_screen
    );
}

void agent_hmi_poll(Stream &serial_port) {
    static char rx_buf[128];
    static uint8_t rx_len = 0;

    while (serial_port.available()) {
        char c = (char)serial_port.read();
        if (c == '\r' || c == '\n') {
            if (rx_len == 0) continue;
            rx_buf[rx_len] = '\0';

            // Command Processing
            if (strcmp(rx_buf, "PING") == 0) {
                serial_port.println("PONG");
            }
            else if (strcmp(rx_buf, "SHOT") == 0) {
                serial_port.println("[CMD] SHOT requested");
                shot_request(&serial_port);
            }
            else if (strcmp(rx_buf, "RAWSHOT") == 0) {
                serial_port.println("[CMD] RAWSHOT requested");
                shot_request_raw(&serial_port);
            }
            else if (strncmp(rx_buf, "NAV ", 4) == 0) {
                const char *target = rx_buf + 4;
                while (*target == ' ') target++;
                if (strcasecmp(target, "main") == 0 || strcasecmp(target, "dash") == 0) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_MAIN);
                    serial_port.println("OK NAV MAIN");
                } else if (strcasecmp(target, "settings") == 0 || strcasecmp(target, "cfg") == 0 || strcasecmp(target, "f1") == 0 || strcasecmp(target, "tab0") == 0) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
                    haffler_ui_set_settings_tab(0);
                    serial_port.println("OK NAV SETTINGS TAB0");
                } else if (strcasecmp(target, "schedule") == 0 || strcasecmp(target, "time") == 0 || strcasecmp(target, "f2") == 0 || strcasecmp(target, "tab1") == 0) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
                    haffler_ui_set_settings_tab(1);
                    serial_port.println("OK NAV SETTINGS TAB1");
                } else if (strcasecmp(target, "water") == 0 || strcasecmp(target, "f3") == 0 || strcasecmp(target, "tab2") == 0) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
                    haffler_ui_set_settings_tab(2);
                    serial_port.println("OK NAV SETTINGS TAB2");
                } else if (strcasecmp(target, "diag") == 0 || strcasecmp(target, "io") == 0 || strcasecmp(target, "f4") == 0 || strcasecmp(target, "tab3") == 0) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
                    haffler_ui_set_settings_tab(3);
                    serial_port.println("OK NAV SETTINGS TAB3");
                } else if (strcasecmp(target, "daylock") == 0 || strcasecmp(target, "lock") == 0 || strcasecmp(target, "f5") == 0 || strcasecmp(target, "tab4") == 0) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
                    haffler_ui_set_settings_tab(4);
                    serial_port.println("OK NAV SETTINGS TAB4");
                } else {
                    serial_port.printf("ERR NAV unknown screen: %s\n", target);
                }
            }
            else if (strncmp(rx_buf, "TOUCH ", 6) == 0) {
                int x = 0, y = 0;
                if (sscanf(rx_buf + 6, "%d %d", &x, &y) == 2) {
                    agent_hmi_inject_touch((int16_t)x, (int16_t)y, 180);
                    serial_port.printf("OK TOUCH %d %d\n", x, y);
                } else {
                    serial_port.println("ERR TOUCH syntax: TOUCH <x> <y>");
                }
            }
            else if (strcmp(rx_buf, "START") == 0) {
                g_machine_fsm.startCycle();
                serial_port.println("OK START");
            }
            else if (strcmp(rx_buf, "STOP") == 0) {
                g_machine_fsm.stopCycle();
                serial_port.println("OK STOP");
            }
            else if (strcmp(rx_buf, "OVERRIDE") == 0) {
                g_machine_fsm.overrideTempWait();
                serial_port.println("OK OVERRIDE");
            }
            else if (strncmp(rx_buf, "INP ", 4) == 0) {
                char name[32];
                int val = 0;
                if (sscanf(rx_buf + 4, "%31s %d", name, &val) == 2) {
                    uint8_t pin = 255;
                    if (strcasecmp(name, "estop") == 0) pin = PIN_INPUT_ESTOP;
                    else if (strcasecmp(name, "guard") == 0) pin = PIN_INPUT_GUARD;
                    else if (strcasecmp(name, "overload") == 0) pin = PIN_INPUT_OVERLOAD;
                    else if (strcasecmp(name, "start") == 0) pin = PIN_INPUT_START;
                    else pin = (uint8_t)atoi(name);

                    if (pin < 32) {
                        g_machine_fsm.setVirtualInput(pin, val != 0);
                        serial_port.printf("OK INP pin=%u val=%d\n", pin, val != 0 ? 1 : 0);
                    } else {
                        serial_port.printf("ERR INP invalid pin: %s\n", name);
                    }
                } else {
                    serial_port.println("ERR INP syntax: INP <pin|name> <0|1>");
                }
            }
            else if (strcmp(rx_buf, "CLEAR_INP") == 0) {
                g_machine_fsm.clearAllVirtualInputs();
                serial_port.println("OK CLEAR_INP");
            }
            else if (strncmp(rx_buf, "TEMP ", 5) == 0) {
                float t = atof(rx_buf + 5);
                g_machine_fsm.simulateWaterTemp(t);
                serial_port.printf("OK TEMP %.1f C\n", t);
            }
            else if (strcmp(rx_buf, "CLEAR_TEMP") == 0) {
                g_machine_fsm.clearSimulatedTemp();
                serial_port.println("OK CLEAR_TEMP");
            }
            else if (strncmp(rx_buf, "FLOW ", 5) == 0) {
                int p_count = atoi(rx_buf + 5);
                if (p_count > 0) {
                    g_machine_fsm.injectFlowPulses((uint32_t)p_count);
                    serial_port.printf("OK FLOW %d pulses\n", p_count);
                }
            }
            else if (strncmp(rx_buf, "SET ", 4) == 0) {
                char param[32];
                char sval[32];
                if (sscanf(rx_buf + 4, "%31s %31s", param, sval) == 2) {
                    if (strcasecmp(param, "mix") == 0) {
                        g_machine_fsm.setMixTimeSec((uint32_t)atoi(sval));
                        serial_port.printf("OK SET mix=%s\n", sval);
                    } else if (strcasecmp(param, "water") == 0) {
                        g_machine_fsm.setWaterTargetLiters((float)atof(sval));
                        serial_port.printf("OK SET water=%s\n", sval);
                    } else if (strcasecmp(param, "temp") == 0) {
                        g_machine_fsm.setWaterTargetTemp((float)atof(sval));
                        serial_port.printf("OK SET temp=%s\n", sval);
                    } else if (strcasecmp(param, "mode") == 0) {
                        g_machine_fsm.setDosingMode((uint8_t)atoi(sval));
                        serial_port.printf("OK SET mode=%s\n", sval);
                    } else {
                        serial_port.printf("ERR SET unknown param: %s\n", param);
                    }
                }
            }
            else if (strncmp(rx_buf, "LANG ", 5) == 0) {
                const char *l = rx_buf + 5;
                while (*l == ' ') l++;
                if (strcasecmp(l, "fa") == 0) {
                    ui_lang_set(LANG_FA);
                    serial_port.println("OK LANG FA");
                } else if (strcasecmp(l, "en") == 0) {
                    ui_lang_set(LANG_EN);
                    serial_port.println("OK LANG EN");
                } else if (strcasecmp(l, "tr") == 0) {
                    ui_lang_set(LANG_TR);
                    serial_port.println("OK LANG TR");
                } else {
                    serial_port.printf("ERR LANG unknown: %s\n", l);
                }
            }
            else if (strcmp(rx_buf, "STATUS") == 0) {
                print_status_json(serial_port);
            }
            else if (strncmp(rx_buf, "DAYLOCK ", 8) == 0) {
                const char *dl_cmd = rx_buf + 8;
                if (strcasecmp(dl_cmd, "challenge") == 0) {
                    uint16_t ch = g_machine_fsm.getDayLockChallenge();
                    serial_port.printf("DAYLOCK CHALLENGE: %04u\n", ch);
                } else if (strncmp(dl_cmd, "unlock ", 7) == 0) {
                    uint16_t resp = (uint16_t)atoi(dl_cmd + 7);
                    bool ok = g_machine_fsm.verifyDayLockUnlock(resp);
                    serial_port.printf("DAYLOCK UNLOCK %s\n", ok ? "SUCCESS" : "FAILED");
                } else if (strncmp(dl_cmd, "bypass ", 7) == 0) {
                    bool ok = g_machine_fsm.applyDayLockAdminBypass(dl_cmd + 7);
                    serial_port.printf("DAYLOCK BYPASS %s\n", ok ? "SUCCESS" : "FAILED");
                }
            }
            else {
                serial_port.printf("ERR UNKNOWN CMD: %s\n", rx_buf);
            }

            rx_len = 0;
        } else if (rx_len < sizeof(rx_buf) - 1) {
            if (rx_len == 0 && ((unsigned char)c <= 32 || (unsigned char)c > 126)) {
                continue;
            }
            rx_buf[rx_len++] = c;
        }
    }
}
