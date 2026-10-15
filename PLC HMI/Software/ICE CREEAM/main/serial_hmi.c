#include "serial_hmi.h"
#include "project_config.h"
#include "storage.h"
#include "lvgl_port.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui/actions.h"
#include "ui/ui_lang.h"
#include "wifi.h"
#include "adc.h"
#include "panel_nav.h"
#include "day_lock_core.h"
#include "max6675.h"
#include "extra_modes.h"
#include "analog_inputs.h"

extern int32_t icecream_counter, icecream_counter_daily, icecream_counter_total, cone_price;
extern int32_t s_comp_cooldown_left_s;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "mbedtls/md.h"

static const char *TAG = "serial_hmi";

typedef enum {
    PORT_UART0 = 0,
    PORT_USB_JTAG = 1,
    PORT_INTERNAL = 2,
} serial_port_type_t;

typedef enum {
    SHOT_STATE_IDLE = 0,
    SHOT_STATE_REQUESTED,
    SHOT_STATE_COPYING,
    SHOT_STATE_READY,
} shot_state_t;

typedef enum {
    SHOT_MODE_B64 = 0,
    SHOT_MODE_RAW = 1,
    SHOT_MODE_TURBO = 2,
} shot_mode_t;

static volatile shot_state_t s_shot_state = SHOT_STATE_IDLE;
static uint16_t *s_shot_buf = NULL;
static serial_port_type_t s_shot_port = PORT_UART0;
static shot_mode_t s_shot_mode = SHOT_MODE_TURBO;
static int64_t s_shot_t0 = 0;
static uint16_t s_shot_rows_copied = 0;
static uint8_t s_shot_row_seen[(SHOT_H + 7) / 8];
static TaskHandle_t s_serial_task_handle = NULL;

static size_t serial_write_bytes(serial_port_type_t port, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t written = 0;
    int idle_retries = 100;
    while (written < len && idle_retries > 0) {
        size_t to_write = len - written;
        if (port == PORT_USB_JTAG) {
            if (to_write > 64) to_write = 64;
            int w = usb_serial_jtag_write_bytes(p + written, to_write, pdMS_TO_TICKS(100));
            if (w > 0) {
                written += (size_t)w;
                idle_retries = 100;
            } else if (w < 0) {
                break;
            } else {
                idle_retries--;
                vTaskDelay(pdMS_TO_TICKS(2));
            }
        } else {
            if (to_write > 2048) to_write = 2048;
            int w = uart_write_bytes(UART_NUM_0, (const char *)(p + written), to_write);
            if (w > 0) {
                written += (size_t)w;
                idle_retries = 100;
            } else if (w < 0) {
                break;
            } else {
                idle_retries--;
                taskYIELD();
            }
        }
    }
    return written;
}

static void serial_write_str(serial_port_type_t port, const char *str)
{
    serial_write_bytes(port, str, strlen(str));
}

static void b64_encode_row(const uint8_t *src, char *dst, size_t len)
{
    static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint8_t b0 = src[i];
        uint8_t b1 = (i + 1 < len) ? src[i + 1] : 0;
        uint8_t b2 = (i + 2 < len) ? src[i + 2] : 0;
        uint32_t v = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | b2;
        dst[o++] = b64_table[(v >> 18) & 0x3F];
        dst[o++] = b64_table[(v >> 12) & 0x3F];
        dst[o++] = b64_table[(v >> 6)  & 0x3F];
        dst[o++] = b64_table[v         & 0x3F];
    }
    dst[o++] = '\n';
    dst[o] = '\0';
}

static bool verify_hmac(const char *body, size_t body_len, const char *sig_hex)
{
    if (strlen(sig_hex) != 64) {
        return false;
    }

    uint8_t hmac_res[32];
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md_info) return false;

    if (mbedtls_md_hmac(md_info,
                        (const unsigned char *)SERIAL_MASTER_SECRET,
                        strlen(SERIAL_MASTER_SECRET),
                        (const unsigned char *)body,
                        body_len,
                        hmac_res) != 0) {
        return false;
    }

    char calc_hex[65];
    for (int i = 0; i < 32; i++) {
        snprintf(&calc_hex[i * 2], (size_t)(sizeof(calc_hex) - (size_t)(i * 2)), "%02X", hmac_res[i]);
    }
    calc_hex[64] = '\0';

    return (strcasecmp(calc_hex, sig_hex) == 0);
}

static void serial_hmi_arm_shot(serial_port_type_t port, shot_mode_t mode)
{
    extern int ota_get_install_state(void);
    int ota_st = ota_get_install_state();
    if (ota_st == 1 || ota_st == 2 || ota_st == 3) { /* OTA_INST_CONNECTING, DOWNLOADING, VERIFYING */
        serial_write_str(port, "SHOT_ERR ota_active\r\n");
        return;
    }

    if (s_shot_state != SHOT_STATE_IDLE) {
        serial_write_str(port, "SHOT_ERR busy\r\n");
        return;
    }

    if (!s_shot_buf) {
        s_shot_buf = (uint16_t *)heap_caps_malloc(SHOT_W * SHOT_H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_shot_buf) {
            s_shot_buf = (uint16_t *)malloc(SHOT_W * SHOT_H * 2);
        }
    }
    if (!s_shot_buf) {
        serial_write_str(port, "SHOT_ERR alloc\r\n");
        return;
    }

    s_shot_port = port;
    s_shot_mode = mode;
    s_shot_rows_copied = 0;
    memset(s_shot_row_seen, 0, sizeof(s_shot_row_seen));
    s_shot_t0 = esp_timer_get_time() / 1000;
    s_shot_state = SHOT_STATE_COPYING;

    if (lvgl_lock(pdMS_TO_TICKS(100))) {
        lv_obj_t *act = lv_scr_act();
        if (act && lv_obj_is_valid(act)) lv_obj_invalidate(act);
        lv_obj_t *top = lv_layer_top();
        if (top && lv_obj_is_valid(top)) lv_obj_invalidate(top);
        lv_obj_t *sys = lv_layer_sys();
        if (sys && lv_obj_is_valid(sys)) lv_obj_invalidate(sys);
        lvgl_unlock();
    }
}

void serial_hmi_request_shot(void)
{
    serial_hmi_arm_shot(PORT_UART0, SHOT_MODE_B64);
}

void serial_hmi_request_raw_shot(void)
{
    serial_hmi_arm_shot(PORT_UART0, SHOT_MODE_RAW);
}

void serial_hmi_request_turbo_shot(void)
{
    serial_hmi_arm_shot(PORT_UART0, SHOT_MODE_TURBO);
}

void serial_hmi_on_flush(const lv_area_t *area, const uint16_t *px)
{
    if (s_shot_state != SHOT_STATE_COPYING || !s_shot_buf || !area || !px) {
        return;
    }

    int x1 = area->x1 < 0 ? 0 : area->x1;
    int x2 = area->x2 >= SHOT_W ? SHOT_W - 1 : area->x2;
    int w_src = area->x2 - area->x1 + 1;
    if (x2 < x1 || w_src <= 0) return;

    for (int y = area->y1; y <= area->y2; y++) {
        if (y < 0 || y >= SHOT_H) {
            px += w_src;
            continue;
        }
        uint16_t *dst = &s_shot_buf[y * SHOT_W + x1];
        const uint16_t *src = px + (x1 - area->x1);
        int copy_w = x2 - x1 + 1;
        memcpy(dst, src, copy_w * sizeof(uint16_t));
        px += w_src;

        uint8_t mask = (uint8_t)(1u << (y & 7));
        uint16_t idx = (uint16_t)(y >> 3);
        if (!(s_shot_row_seen[idx] & mask)) {
            s_shot_row_seen[idx] |= mask;
            s_shot_rows_copied++;
        }
    }

    if (s_shot_rows_copied >= SHOT_H) {
        s_shot_state = SHOT_STATE_READY;
        if (s_serial_task_handle) {
            xTaskNotifyGive(s_serial_task_handle);
        }
    }
}

static void stream_screenshot(void)
{
    if (!s_shot_buf || s_shot_state != SHOT_STATE_READY) {
        return;
    }

    if (s_shot_port == PORT_INTERNAL) {
        /* Capture completed for internal HTTP / MQTT buffer */
        return;
    }

    serial_port_type_t p = s_shot_port;
    esp_log_level_set("*", ESP_LOG_NONE);

    if (s_shot_mode == SHOT_MODE_TURBO) {
        /* جریان فوق‌سریع فشرده‌سازی RLE (ارسال کل صفحه در کمتر از ۰.۵ ثانیه بدون افت کیفیت) */
        char header[64];
        snprintf(header, sizeof(header), "\r\nTURBO_BEGIN %d %d\n", SHOT_W, SHOT_H);
        serial_write_str(p, header);

        const uint16_t *pixels = s_shot_buf;
        size_t total_px = SHOT_W * SHOT_H;
        size_t i = 0;
        uint8_t out_buf[1536];
        size_t out_len = 0;

        while (i < total_px) {
            uint16_t px = pixels[i];
            size_t run = 1;
            while (i + run < total_px && pixels[i + run] == px && run < 255) {
                run++;
            }
            out_buf[out_len++] = (uint8_t)run;
            out_buf[out_len++] = (uint8_t)(px & 0xFF);
            out_buf[out_len++] = (uint8_t)((px >> 8) & 0xFF);
            i += run;

            if (out_len >= sizeof(out_buf) - 6) {
                serial_write_bytes(p, out_buf, out_len);
                out_len = 0;
            }
        }
        if (out_len > 0) {
            serial_write_bytes(p, out_buf, out_len);
        }
        serial_write_str(p, "\nTURBO_END\n");
    } else if (s_shot_mode == SHOT_MODE_RAW) {
        /* جریان دودویی مستقیم بدون سربار Base64 */
        char header[64];
        snprintf(header, sizeof(header), "\r\nRAW_BEGIN %d %d %u\n", SHOT_W, SHOT_H, (unsigned)(SHOT_W * SHOT_H * 2));
        serial_write_str(p, header);

        const uint8_t *raw = (const uint8_t *)s_shot_buf;
        size_t total_bytes = SHOT_W * SHOT_H * 2;
        size_t sent = 0;
        while (sent < total_bytes) {
            size_t chunk = (total_bytes - sent > 4096) ? 4096 : (total_bytes - sent);
            size_t w = serial_write_bytes(p, raw + sent, chunk);
            if (w == 0) break;
            sent += w;
            taskYIELD();
        }
        serial_write_str(p, "\nRAW_END\n");
    } else {
        char header[64];
        snprintf(header, sizeof(header), "\r\nSHOT_BEGIN %d %d RGB565LE\n", SHOT_W, SHOT_H);
        serial_write_str(p, header);
        vTaskDelay(pdMS_TO_TICKS(10));

        /* stack-local — thread-safe در مقابل دو port همزمان */
        char line_b64[SHOT_W * 2 * 4 / 3 + 4];
        const uint8_t *raw = (const uint8_t *)s_shot_buf;

        for (int y = 0; y < SHOT_H; y++) {
            const uint8_t *row = &raw[y * SHOT_W * 2];
            b64_encode_row(row, line_b64, SHOT_W * 2);
            serial_write_str(p, line_b64);
            if ((y & 15) == 0) {
                taskYIELD();
            }
        }
        serial_write_str(p, "SHOT_END\n");
    }

    esp_log_level_set("*", ESP_LOG_INFO);

    s_shot_state = SHOT_STATE_IDLE;
}

static void execute_command(serial_port_type_t port, const char *cmd, const char *payload)
{
    screensaver_notify_activity();
    if (screensaver_is_active()) {
        if (lvgl_lock(pdMS_TO_TICKS(100))) {
            hide_screensaver();
            lvgl_unlock();
        }
    }

    if (strcasecmp(cmd, "NAV") == 0) {
        int scr_num = SCREEN_ID_MAIN_PG;
        int tab_idx = -1;

        if (strcasecmp(payload, "main") == 0 || strcasecmp(payload, "home") == 0) {
            scr_num = SCREEN_ID_MAIN_PG;
        } else if (strcasecmp(payload, "splash") == 0) {
            scr_num = SCREEN_ID_SPLASH_SCREEN;
        } else if (strcasecmp(payload, "settings") == 0 || strcasecmp(payload, "set") == 0) {
            scr_num = SCREEN_ID_SETTINGS_PG;
        } else if (strcasecmp(payload, "framware") == 0 || strcasecmp(payload, "firmware") == 0 || strcasecmp(payload, "ota") == 0) {
            scr_num = SCREEN_ID_SCREEN_FRAMWARE;
        } else if (strcasecmp(payload, "boot_wifi") == 0 || strcasecmp(payload, "wifi") == 0 || strcasecmp(payload, "5") == 0) {
            scr_num = SCREEN_ID_BOOT_WIFI;
        } else if (strcasecmp(payload, "warranty") == 0 || strcasecmp(payload, "warr") == 0 || strcasecmp(payload, "6") == 0) {
            scr_num = SCREEN_ID_WARRANTY;
        } else if (strcasecmp(payload, "expired") == 0) {
            scr_num = 99;
        } else if (strncasecmp(payload, "F", 1) == 0 && isdigit((unsigned char)payload[1])) {
            int f_num = atoi(payload + 1);
            if (f_num >= 1 && f_num <= 6) {
                scr_num = SCREEN_ID_SETTINGS_PG;
                tab_idx = f_num - 1;
            }
        } else if (strchr(payload, ':')) {
            sscanf(payload, "%d:%d", &scr_num, &tab_idx);
        } else if (isdigit((unsigned char)payload[0])) {
            int n = atoi(payload);
            if (n == 0) scr_num = SCREEN_ID_SPLASH_SCREEN;
            else if (n == 1) scr_num = SCREEN_ID_MAIN_PG;
            else if (n == 2 || n == 3) scr_num = SCREEN_ID_SETTINGS_PG;
            else if (n == 4) scr_num = SCREEN_ID_SCREEN_FRAMWARE;
            else if (n == 5) scr_num = SCREEN_ID_BOOT_WIFI;
            else if (n == 6) scr_num = SCREEN_ID_WARRANTY;
        }

        if (lvgl_lock(pdMS_TO_TICKS(100))) {
            panel_nav_dismiss_active_modal();
            if (scr_num == SCREEN_ID_MAIN_PG) {
                loadScreen(SCREEN_ID_MAIN_PG);
            } else if (scr_num == SCREEN_ID_SETTINGS_PG) {
                if (tab_idx >= 0 && tab_idx <= 5) {
                    open_settings_tab((uint32_t)tab_idx);
                } else {
                    open_settings_tab(0);
                }
            } else if (scr_num == SCREEN_ID_SPLASH_SCREEN) {
                loadScreen(SCREEN_ID_SPLASH_SCREEN);
            } else if (scr_num == SCREEN_ID_SCREEN_FRAMWARE) {
                loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
            } else if (scr_num == SCREEN_ID_BOOT_WIFI) {
                loadScreen(SCREEN_ID_BOOT_WIFI);
            } else if (scr_num == SCREEN_ID_WARRANTY) {
                open_warranty_screen();
            } else if (scr_num == 99) {
                extern void open_warranty_expired_dialog(void);
                open_warranty_expired_dialog();
            }
            lvgl_unlock();

            char resp[64];
            snprintf(resp, sizeof(resp), "OK NAV %s\r\n", payload);
            serial_write_str(port, resp);
        } else {
            serial_write_str(port, "ERR LVGL_BUSY\r\n");
        }
    } else if (strcasecmp(cmd, "SHOT") == 0) {
        serial_hmi_arm_shot(port, SHOT_MODE_B64);
    } else if (strcasecmp(cmd, "RAWSHOT") == 0) {
        serial_hmi_arm_shot(port, SHOT_MODE_RAW);
    } else if (strcasecmp(cmd, "TURBOSHOT") == 0 || strcasecmp(cmd, "FASTSHOT") == 0) {
        serial_hmi_arm_shot(port, SHOT_MODE_TURBO);
    } else if (strcasecmp(cmd, "LANG") == 0) {
        int l = 0;
        if (strcasecmp(payload, "fa") == 0 || strcmp(payload, "0") == 0) {
            l = UI_LANG_FA;
        } else if (strcasecmp(payload, "en") == 0 || strcmp(payload, "1") == 0) {
            l = UI_LANG_EN;
        } else if (strcasecmp(payload, "tr") == 0 || strcmp(payload, "2") == 0) {
            l = UI_LANG_TR;
        } else if (strcasecmp(payload, "cycle") == 0 || strcasecmp(payload, "next") == 0) {
            l = (ui_lang_get() + 1) % 3;
        } else {
            l = atoi(payload);
        }
        if (l < 0 || l > 2) l = 0;
        if (lvgl_lock(pdMS_TO_TICKS(100))) {
            ui_lang_set((ui_lang_t)l);
            ui_screens_refresh_language();
            lvgl_unlock();
        }
        char resp[64];
        snprintf(resp, sizeof(resp), "OK LANG %d\r\n", l);
        serial_write_str(port, resp);
    } else if (strcasecmp(cmd, "STATUS") == 0) {
        char resp[160];
        snprintf(resp, sizeof(resp), "OK STATUS heap=%u psram=%u screen=%d ip=%s\r\n",
                 (unsigned)esp_get_free_heap_size(),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (int)ui_get_screen_id(),
                 wifi_get_ip_str() ? wifi_get_ip_str() : "");
        serial_write_str(port, resp);
    } else if (strcasecmp(cmd, "WIFI") == 0) {
        if (strcasecmp(payload, "ON") == 0 || strcasecmp(payload, "ENABLE") == 0 || strcasecmp(payload, "1") == 0) {
            wifi_set_enabled(true);
            serial_write_str(port, "OK WIFI ENABLED\r\n");
        } else if (strcasecmp(payload, "OFF") == 0 || strcasecmp(payload, "DISABLE") == 0 || strcasecmp(payload, "0") == 0) {
            wifi_set_enabled(false);
            serial_write_str(port, "OK WIFI DISABLED\r\n");
        } else {
            extern bool webportal_is_running(void);
            char resp[256];
            snprintf(resp, sizeof(resp),
                     "OK WIFI en=%d conn=%d ap_ssid=\"%s\" ap_pass=\"%s\" ip=\"%s\" ap_clients=%u web=%d\r\n",
                     wifi_is_enabled() ? 1 : 0,
                     wifi_is_connected() ? 1 : 0,
                     wifi_get_ap_ssid() ? wifi_get_ap_ssid() : "",
                     wifi_get_ap_pass() ? wifi_get_ap_pass() : "",
                     wifi_get_ip_str() ? wifi_get_ip_str() : "",
                     (unsigned)wifi_get_ap_client_count(),
                     webportal_is_running() ? 1 : 0);
            serial_write_str(port, resp);
        }
    } else if (strcasecmp(cmd, "METRICS") == 0) {
        int front = micro_front_active() ? 0 : 1;
        int rear = micro_rear_active() ? 0 : 1;
        int eye = inductive_active() ? 0 : 1;
        int load_m = load_motor_tripped() ? 0 : 1;
        int load_c = load_comp_tripped() ? 0 : 1;
        int comp = gpio_get_level(Compressor);
        int make = gpio_get_level(Make_Icecream);
        int paddle = gpio_get_level(Paddle);
        int reset_rel = gpio_get_level(RESET_RELAY_GPIO);
        int bz = gpio_get_level(BUZZER_GPIO);
        int tab_idx = ui_get_settings_tab();
        float temp_top = max6675_get_temp_top();
        float temp_cyl = max6675_get_temp_cylinder();
        bool temp_top_ok = max6675_is_top_valid();
        bool temp_cyl_ok = max6675_is_cylinder_valid();

        int lvl4 = gpio_get_level(GPIO_NUM_4);
        int lvl5 = gpio_get_level(GPIO_NUM_5);
        int lvl6 = gpio_get_level(GPIO_NUM_6);
        int adc3 = adc_get_gpio4();
        int adc4 = adc_get_gpio5();
        int adc5 = adc_get_gpio6();

        const char *ip_str = wifi_get_ip_str();
        if (!ip_str) ip_str = "";

        static char resp[1152];
        snprintf(resp, sizeof(resp),
            "OK METRICS {\"fps\":%u,\"render_us\":%u,\"flush_us\":%u,\"plc_scan_us\":%u,\"heap_free\":%u,\"heap_min\":%u,\"psram_free\":%u,\"screen\":%d,\"tab\":%d,\"inputs\":{\"front\":%d,\"rear\":%d,\"eye\":%d,\"load_m\":%d,\"load_c\":%d},\"relays\":{\"comp\":%d,\"make\":%d,\"paddle\":%d,\"reset\":%d,\"bz\":%d},\"adc\":{\"ch0\":%d,\"ch1\":%d,\"ch3\":%d,\"ch4\":%d,\"ch5\":%d,\"lvl4\":%d,\"lvl5\":%d,\"lvl6\":%d},\"fault\":%d,\"temp_top\":%.1f,\"temp_cyl\":%.1f,\"temp_top_ok\":%d,\"temp_cyl_ok\":%d,\"scoops\":%ld,\"scoops_day\":%ld,\"cone_price\":%ld,\"stage\":%u,\"remain_days\":%d,\"lease_locked\":%d,\"cooldown_s\":%ld,\"ip\":\"%s\"}\r\n",
            (unsigned)lvgl_port_get_fps(),
            (unsigned)lvgl_port_get_render_us(),
            (unsigned)lvgl_port_get_flush_us(),
            (unsigned)plc_get_scan_us(),
            (unsigned)esp_get_free_heap_size(),
            (unsigned)esp_get_minimum_free_heap_size(),
            (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
            (int)ui_get_screen_id(),
            tab_idx,
            front, rear, eye, load_m, load_c,
            comp, make, paddle, reset_rel, bz,
            adc_get_io1(),
            adc_get_io2(),
            adc3, adc4, adc5,
            lvl4, lvl5, lvl6,
            plc_is_fault() ? 1 : 0,
            temp_top, temp_cyl, temp_top_ok ? 1 : 0, temp_cyl_ok ? 1 : 0,
            (long)icecream_counter, (long)icecream_counter_daily, (long)cone_price,
            (unsigned)day_lock_get_active_stage(), (int)day_lock_get_remain_days(),
            day_lock_is_locked() ? 1 : 0, (long)s_comp_cooldown_left_s,
            ip_str
        );
        serial_write_str(port, resp);
    } else if (strcasecmp(cmd, "HOPPER_SENSOR") == 0) {
        bool en = (strcmp(payload, "1") == 0 || strcasecmp(payload, "true") == 0 || strcasecmp(payload, "on") == 0);
        max6675_set_top_enabled(en);
        char resp[64];
        snprintf(resp, sizeof(resp), "OK HOPPER_SENSOR %d\r\n", en ? 1 : 0);
        serial_write_str(port, resp);
    } else if (strcasecmp(cmd, "EVENT") == 0) {
        int tx = 0, ty = 0;
        /* بلاک inject لمس اگر ماشین در حال تولید فعال باشد — جلوگیری از کور شدن actions_task */
        bool machine_running = (make_start || paddle_start || ice_start);
        if (strncasecmp(payload, "TAP:", 4) == 0 && sscanf(payload + 4, "%d,%d", &tx, &ty) == 2) {
            if (!machine_running) {
                lvgl_port_inject_touch((uint16_t)tx, (uint16_t)ty, true);
                vTaskDelay(pdMS_TO_TICKS(20)); /* کاهش از 60→20ms */
                lvgl_port_inject_touch((uint16_t)tx, (uint16_t)ty, false);
                vTaskDelay(pdMS_TO_TICKS(20));
                lvgl_port_clear_inject_touch();
            }
        } else if (strncasecmp(payload, "HOLD:", 5) == 0) {
            int hold_ms = 2200;
            if (sscanf(payload + 5, "%d,%d,%d", &tx, &ty, &hold_ms) >= 2) {
                if (hold_ms < 10) hold_ms = 10;
                if (hold_ms > 3000) hold_ms = 3000; /* کران بالا: جلوگیری از DoS روی Core 0 */
                lvgl_port_inject_touch((uint16_t)tx, (uint16_t)ty, true);
                vTaskDelay(pdMS_TO_TICKS(hold_ms));
                lvgl_port_inject_touch((uint16_t)tx, (uint16_t)ty, false);
                vTaskDelay(pdMS_TO_TICKS(40));
                lvgl_port_clear_inject_touch();
            }
        } else if (strncasecmp(payload, "KEY:F", 5) == 0 && isdigit((unsigned char)payload[5])) {
            int f_num = atoi(payload + 5);
            if (f_num >= 1 && f_num <= 6) {
                panel_nav_queue_key(1, (uint8_t)f_num);
            }
        } else if (strcasecmp(payload, "KEY:START") == 0) {
            panel_nav_queue_key(2, 4);
        } else if (strcasecmp(payload, "KEY:STOP") == 0) {
            panel_nav_queue_key(2, 5);
        } else if (strcasecmp(payload, "MAKE:START") == 0) {
            plc_hw_start_make();
        } else if (strcasecmp(payload, "MAKE:STOP") == 0 || strcasecmp(payload, "ESTOP") == 0) {
            plc_stop_all();
        } else if (strcasecmp(payload, "MAKE:TOGGLE") == 0) {
            if (make_start) {
                plc_stop_all();
            } else {
                plc_hw_start_make();
            }
        } else if (strcasecmp(payload, "WASH:START") == 0 || strcasecmp(payload, "WASH:TOGGLE") == 0) {
            extern void wash_toggle(void);
            wash_toggle();
        } else if (strcasecmp(payload, "PADDLE:TOGGLE") == 0 || strcasecmp(payload, "PADDLE:START") == 0) {
            if (paddle_start) {
                plc_stop_all();
            } else {
                extern void action_go_paddle_on(lv_event_t *e);
                action_go_paddle_on(NULL);
            }
        } else if (strcasecmp(payload, "HOLD:TOGGLE") == 0 || strcasecmp(payload, "HOLD:START") == 0) {
            if (ice_start) {
                plc_stop_all();
            } else {
                extern void action_go_compressor_on(lv_event_t *e);
                action_go_compressor_on(NULL);
            }
        } else if (strcasecmp(payload, "MODAL:CLOSE") == 0 || strcasecmp(payload, "CLEAR:FAULTS") == 0 || strcasecmp(payload, "FAULT:CLEAR") == 0 || strcasecmp(payload, "FAULT:RESET") == 0) {
            extern bool panel_nav_dismiss_active_modal(void);
            extern void fault_log_clear(void);
            extern bool plc_try_clear_fault(void);
            plc_try_clear_fault();
            plc_request_control_reset();
            fault_log_clear();
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                panel_nav_dismiss_active_modal();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "DAYLOCK:CLEAR") == 0 || strcasecmp(payload, "DAYLOCK:UNLOCK") == 0) {
            extern void day_lock_clear_after_unlock(void);
            day_lock_clear_after_unlock();
        } else if (strcasecmp(payload, "DAYLOCK:RESET_ERRORS") == 0) {
            day_lock_reset_failed_attempts();
        } else if (strcasecmp(payload, "DAYLOCK:SHOW") == 0) {
            extern void day_lock_ui_show(void);
            day_lock_ui_show();
        } else if (strcasecmp(payload, "DAYLOCK:SHOW_5TIMERS") == 0) {
            extern void day_lock_ui_show_5timers_dialog(void);
            day_lock_ui_show_5timers_dialog();
        } else if (strcasecmp(payload, "KEY:SAVE") == 0) {
            panel_nav_queue_key(2, 1);
        } else if (strcasecmp(payload, "KEY:CANCEL") == 0) {
            panel_nav_queue_key(2, 2);
        } else if (strcasecmp(payload, "KEY:MODE") == 0) {
            panel_nav_queue_key(2, 3);
        } else if (strcasecmp(payload, "PADDLE:START") == 0 || strcasecmp(payload, "PADDLE:TOGGLE") == 0 || strcasecmp(payload, "KEY:F4") == 0) {
            panel_nav_queue_key(1, 4);
        } else if (strcasecmp(payload, "WASH:START") == 0 || strcasecmp(payload, "WASH:TOGGLE") == 0 || strcasecmp(payload, "KEY:F2") == 0) {
            panel_nav_queue_key(1, 2);
        } else if (strcasecmp(payload, "HOLD:START") == 0 || strcasecmp(payload, "HOLD:TOGGLE") == 0 || strcasecmp(payload, "KEY:F3") == 0) {
            panel_nav_queue_key(1, 3);
        } else if (strcasecmp(payload, "RAINBOW:TOGGLE") == 0) {
            rainbow_toggle_rp();
        } else if (strcasecmp(payload, "EDGE:TOGGLE") == 0) {
            edge_toggle_produce();
        } else if (strcasecmp(payload, "COUNTER:RESET") == 0 || strcasecmp(payload, "SCOOPS:CLEAR") == 0 || strcasecmp(payload, "CLEAR:SCOOPS") == 0) {
            extern void ui_update_icecream_counter(void);
            extern void app_state_save_to_nvs(void);
            icecream_counter_daily = 0;
            icecream_counter = 0;
            app_state_save_to_nvs();
            ui_update_icecream_counter();
        } else if (strncasecmp(payload, "LEASE:UNLOCK:", 13) == 0) {
            lease_try_unlock_str(payload + 13);
        } else if (strcasecmp(payload, "SHOW:PIN") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                show_manager_pin_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "BOOT:F5") == 0 || strcasecmp(payload, "SIM:BOOT_F5") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                g_boot_f5_service_mode = true;
                s_pin_for_boot_wifi = true;
                loadScreen(SCREEN_ID_BOOT_WIFI);
                show_manager_pin_dialog();
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "PIN:ENTER:", 10) == 0) {
            const char *pin_str = payload + 10;
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                extern void action_manager_pin_submit_str(const char *pin_str);
                action_manager_pin_submit_str(pin_str);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "OTA:START") == 0 || strcasecmp(payload, "OTA:INSTALL") == 0) {
            extern void ota_ui_start_install_model(int idx);
            if (lvgl_lock(pdMS_TO_TICKS(1000))) {
                loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
                ota_ui_start_install_model(0);
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "TOUCH:", 6) == 0) {
            int tx = 0, ty = 0;
            if (sscanf(payload + 6, "%d,%d", &tx, &ty) == 2) {
                extern void lvgl_port_inject_touch(uint16_t x, uint16_t y, bool pressed);
                extern void lvgl_port_clear_inject_touch(void);
                lvgl_port_inject_touch((uint16_t)tx, (uint16_t)ty, true);
                vTaskDelay(pdMS_TO_TICKS(80));
                lvgl_port_inject_touch((uint16_t)tx, (uint16_t)ty, false);
                vTaskDelay(pdMS_TO_TICKS(40));
                lvgl_port_clear_inject_touch();
            }
        } else if (strncasecmp(payload, "SWIPE:", 6) == 0) {
            int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
            if (sscanf(payload + 6, "%d,%d,%d,%d", &x1, &y1, &x2, &y2) == 4) {
                extern void lvgl_port_inject_touch(uint16_t x, uint16_t y, bool pressed);
                extern void lvgl_port_clear_inject_touch(void);
                int steps = 10;
                for (int s = 0; s <= steps; s++) {
                    int cx = x1 + (x2 - x1) * s / steps;
                    int cy = y1 + (y2 - y1) * s / steps;
                    lvgl_port_inject_touch((uint16_t)cx, (uint16_t)cy, true);
                    vTaskDelay(pdMS_TO_TICKS(25));
                }
                lvgl_port_inject_touch((uint16_t)x2, (uint16_t)y2, false);
                vTaskDelay(pdMS_TO_TICKS(40));
                lvgl_port_clear_inject_touch();
            }
        } else if (strcasecmp(payload, "SHOW:FAULTS") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                show_fault_history_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:DASH") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                show_manager_dashboard_dialog(false);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:MASTER") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                show_manager_dashboard_dialog(true);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:HELP") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                extern void ui_show_smart_guide(guide_id_t id);
                ui_show_smart_guide(GUIDE_ID_SETTINGS);
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "SHOW:GUIDE:", 11) == 0) {
            int gid = atoi(payload + 11);
            if (gid >= 1 && gid <= 20) {
                if (lvgl_lock(pdMS_TO_TICKS(100))) {
                    extern void ui_show_smart_guide(guide_id_t id);
                    ui_show_smart_guide((guide_id_t)gid);
                    lvgl_unlock();
                }
            }
        } else if (strncasecmp(payload, "SHOW:FAULT:", 11) == 0) {
            int fcode = atoi(payload + 11);
            extern void plc_trigger_fault_interactive(uint8_t code);
            plc_trigger_fault_interactive((uint8_t)fcode);
        } else if (strncasecmp(payload, "HOLD:", 5) == 0) {
            int hx = 0, hy = 0, hms = 1500;
            if (sscanf(payload + 5, "%d,%d,%d", &hx, &hy, &hms) >= 2) {
                if (hms < 10) hms = 10;
                if (hms > 3000) hms = 3000;
                extern void lvgl_port_inject_touch(uint16_t x, uint16_t y, bool pressed);
                extern void lvgl_port_clear_inject_touch(void);
                lvgl_port_inject_touch((uint16_t)hx, (uint16_t)hy, true);
                vTaskDelay(pdMS_TO_TICKS(hms));
                lvgl_port_inject_touch((uint16_t)hx, (uint16_t)hy, false);
                vTaskDelay(pdMS_TO_TICKS(40));
                lvgl_port_clear_inject_touch();
            }
        } else if (strcasecmp(payload, "SHOW:LEASE") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                show_lease_unlock_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:WARRANTY") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                extern void open_warranty_details_dialog(void);
                open_warranty_details_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "HIDE:WARRANTY") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                extern void hide_warranty_details_dialog(void);
                hide_warranty_details_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:RESET") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                show_factory_reset_confirm_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:COUNTERS") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                show_counter_details_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:FW") == 0 || strcasecmp(payload, "SHOW:OTA") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                extern void ota_request_scan(void);
                ota_request_scan();
                loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "START:OTA") == 0 || strcasecmp(payload, "OTA:START") == 0 || strcasecmp(payload, "OTA:INSTALL") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(1000))) {
                extern void hide_boot_ota_prompt_dialog(void);
                extern void ota_ui_start_install_model(int idx);
                hide_boot_ota_prompt_dialog();
                loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
                ota_ui_start_install_model(0);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:URL") == 0 || strncasecmp(payload, "SHOW:URL:", 9) == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                extern void open_custom_url_dialog(void);
                extern void ota_set_custom_url(const char *url);
                if (strncasecmp(payload, "SHOW:URL:", 9) == 0) {
                    ota_set_custom_url(payload + 9);
                }
                if (ui_get_screen_id() != SCREEN_ID_SCREEN_FRAMWARE) {
                    loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
                }
                open_custom_url_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:SPECS") == 0 || strcasecmp(payload, "SHOW:PINMAP") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                if (ui_get_screen_id() != SCREEN_ID_SCREEN_FRAMWARE) {
                    loadScreen(SCREEN_ID_SCREEN_FRAMWARE);
                }
                show_ota_pinmap_dialog(0);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:CONGRATS") == 0 || strcasecmp(payload, "SHOW:WHATSNEW") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                char prev_v[32] = {0};
                char new_v[32] = {0};
                storage_get_str("ota_prev_ver", "", prev_v, sizeof(prev_v));
                storage_get_str("ota_new_ver", FIRMWARE_VERSION_STR, new_v, sizeof(new_v));
                const char *p_arg = (strcasecmp(payload, "SHOW:WHATSNEW") == 0) ? FIRMWARE_VERSION_CODE : prev_v;
                show_ota_update_congrats_dialog(p_arg, new_v[0] ? new_v : FIRMWARE_VERSION_STR);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SHOW:WIFI_KB") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                wifi_key_act_show();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "HIDE:WIFI_KB") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                wifi_key_act_hide();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "HIDE:PROMPT") == 0 || strcasecmp(payload, "HIDE:BOOT_WIFI") == 0 || strcasecmp(payload, "DISMISS") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                extern void hide_boot_wifi_prompt_dialog(void);
                hide_boot_wifi_prompt_dialog();
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "SCROLL:SCAN_DOWN") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                if (objects.ui_scan_list) {
                    lv_obj_scroll_by_bounded(objects.ui_scan_list, 0, -80, LV_ANIM_OFF);
                }
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "SELECT:AP:", 10) == 0) {
            int ap_idx = atoi(payload + 10);
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                if (objects.ui_scan_list && ap_idx >= 0 && ap_idx < (int)lv_obj_get_child_count(objects.ui_scan_list)) {
                    lv_obj_t *child = lv_obj_get_child(objects.ui_scan_list, ap_idx);
                    if (child) {
                        lv_obj_scroll_to_view(child, LV_ANIM_OFF);
                    }
                }
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "WIFI:ERR", 8) == 0) {
            const char *msg = (strlen(payload) > 9) ? payload + 9 : "رمز عبور نادرست است (تست خطا)";
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                wifi_status_msgbox_show_error(msg);
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "WIFI:OK", 7) == 0) {
            const char *msg = (strlen(payload) > 8) ? payload + 8 : "اتصال با موفقیت برقرار شد";
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                wifi_status_msgbox_show_ok(msg);
                lvgl_unlock();
            }
        } else if (strcasecmp(payload, "MODAL:CLOSE") == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                panel_nav_dismiss_active_modal();
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "TOAST:", 6) == 0) {
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                panel_nav_toast_info(payload + 6);
                lvgl_unlock();
            }
        } else if (strncasecmp(payload, "FAULT:", 6) == 0) {
            uint8_t f_code = (uint8_t)atoi(payload + 6);
            extern void plc_trigger_fault_interactive(uint8_t code);
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                plc_trigger_fault_interactive(f_code);
                lvgl_unlock();
            }
        }
        char resp[64];
        snprintf(resp, sizeof(resp), "OK EVENT %s\r\n", payload);
        serial_write_str(port, resp);

    /* ================================================================
     * دستورات شبیه‌سازی صنعتی (SIM) — برای تست بدون سخت‌افزار
     * نمونه: SIM FAULT:2    → شبیه‌سازی خطای E02 اضافه‌بار موتور
     *        SIM INPUT:front → شبیه‌سازی میکروسوئیچ جلو
     *        SIM INPUT:rear  → شبیه‌سازی میکروسوئیچ عقب
     *        SIM INPUT:stop  → شبیه‌سازی استپ اضطراری
     *        SIM INPUT:start → شبیه‌سازی استارت
     *        SIM GRACE:0     → اعمال رمز گریس ۱ ("11")
     *        SIM GRACE:1     → اعمال رمز گریس ۲ ("22")
     *        SIM GRACE:2     → اعمال رمز گریس ۳ ("33")
     *        SIM STATUS      → گزارش کامل وضعیت FSM و Day-Lock
     * ================================================================ */
    } else if (strcasecmp(cmd, "SIM") == 0) {
        char resp[256] = {0};

        if (strncasecmp(payload, "FAULT:", 6) == 0) {
            /* شبیه‌سازی خطا — نمایش دیالوگ تعاملی خطا */
            uint8_t f_code = (uint8_t)atoi(payload + 6);
            extern void plc_trigger_fault_interactive(uint8_t code);
            if (lvgl_lock(pdMS_TO_TICKS(100))) {
                plc_trigger_fault_interactive(f_code);
                lvgl_unlock();
            }
            snprintf(resp, sizeof(resp), "OK SIM FAULT code=%u\r\n", (unsigned)f_code);

        } else if (strncasecmp(payload, "INPUT:", 6) == 0) {
            /* شبیه‌سازی ورودی سنسوری — برای تست FSM بدون سخت‌افزار */
            const char *inp = payload + 6;
            extern void plc_sim_inject_input(const char *name, bool active);
            plc_sim_inject_input(inp, true);
            vTaskDelay(pdMS_TO_TICKS(150));
            plc_sim_inject_input(inp, false);
            snprintf(resp, sizeof(resp), "OK SIM INPUT:%s pulse sent\r\n", inp);

        } else if (strncasecmp(payload, "GRACE:", 6) == 0) {
            /* شبیه‌سازی رمز اضطراری یکبار مصرف */
            uint8_t gi = (uint8_t)atoi(payload + 6);
            bool ok_g = day_lock_apply_grace10_indexed(gi);
            snprintf(resp, sizeof(resp), "OK SIM GRACE idx=%u result=%s remain_days=%d\r\n",
                     (unsigned)gi, ok_g ? "APPLIED" : "ALREADY_USED",
                     (int)day_lock_get_remain_days());

        } else if (strcasecmp(payload, "STATUS") == 0) {
            /* گزارش جامع وضعیت سیستم */
            extern int32_t s_comp_cooldown_left_s;
            snprintf(resp, sizeof(resp),
                "OK SIM STATUS {\"lock_stage\":%u,\"remain_days\":%d,"
                "\"make\":%d,\"ice\":%d,\"paddle\":%d,"
                "\"cooldown_s\":%ld,\"fault\":%d,"
                "\"grace_used\":[%d,%d,%d]}\r\n",
                (unsigned)day_lock_get_active_stage(),
                (int)day_lock_get_remain_days(),
                make_start ? 1 : 0,
                ice_start ? 1 : 0,
                paddle_start ? 1 : 0,
                (long)s_comp_cooldown_left_s,
                plc_is_fault() ? 1 : 0,
                day_lock_is_grace_used() ? 1 : 0,   /* legacy 112233 */
                0, 0                                  /* grace_used[1],[2] — extendable */
            );

        } else if (strcasecmp(payload, "MAKE:START") == 0) {
            plc_hw_start_make();
            snprintf(resp, sizeof(resp), "OK SIM MAKE:START\r\n");

        } else if (strcasecmp(payload, "MAKE:STOP") == 0) {
            plc_stop_all();
            snprintf(resp, sizeof(resp), "OK SIM MAKE:STOP\r\n");

        } else if (strcasecmp(payload, "COOLDOWN:CLEAR") == 0) {
            extern int32_t s_comp_cooldown_left_s;
            s_comp_cooldown_left_s = 0;
            snprintf(resp, sizeof(resp), "OK SIM COOLDOWN:CLEAR (anti-short-cycle reset)\r\n");

        } else {
            snprintf(resp, sizeof(resp), "ERR SIM UNKNOWN_PAYLOAD %s\r\n", payload);
        }

        serial_write_str(port, resp);
    } else {
        char resp[64];
        snprintf(resp, sizeof(resp), "ERR UNKNOWN_CMD %s\r\n", cmd);
        serial_write_str(port, resp);
    }
}

static void process_line(serial_port_type_t port, char *line)
{
    while (*line == ' ' || *line == '\t' || *line == '\n' || *line == '\r') {
        line++;
    }
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t' ||
                       line[len - 1] == '\r' || line[len - 1] == '\n')) {
        line[--len] = '\0';
    }
    if (len == 0) return;

    /* 1. Legacy / Unauthenticated SHOT, RAWSHOT, TURBOSHOT and STANDBY command support */
    if (strcasecmp(line, "SHOT") == 0 || strncasecmp(line, "SHOT ", 5) == 0) {
        serial_hmi_arm_shot(port, SHOT_MODE_B64);
        return;
    }
    if (strcasecmp(line, "RAWSHOT") == 0 || strncasecmp(line, "RAWSHOT ", 8) == 0) {
        serial_hmi_arm_shot(port, SHOT_MODE_RAW);
        return;
    }
    if (strcasecmp(line, "TURBOSHOT") == 0 || strncasecmp(line, "TURBOSHOT ", 10) == 0 ||
        strcasecmp(line, "FASTSHOT") == 0 || strncasecmp(line, "FASTSHOT ", 9) == 0) {
        serial_hmi_arm_shot(port, SHOT_MODE_TURBO);
        return;
    }
    if (strcasecmp(line, "STANDBY") == 0 || strcasecmp(line, "SS") == 0) {
        if (lvgl_lock(pdMS_TO_TICKS(100))) {
            show_screensaver();
            lvgl_unlock();
            serial_write_str(port, "OK STANDBY\r\n");
        } else {
            serial_write_str(port, "ERR LVGL_BUSY\r\n");
        }
        return;
    }

    /* 2. Authenticated $ICE frame */
    if (strncmp(line, "$ICE,", 5) == 0) {
        char *star = strchr(line + 5, '*');
        if (!star) {
            serial_write_str(port, "ERR INVALID_FRAME_NO_STAR\r\n");
            return;
        }
        *star = '\0';
        const char *body = line + 5;
        const char *sig = star + 1;

#if !SERIAL_SEC_BYPASS
        if (!verify_hmac(body, strlen(body), sig)) {
            serial_write_str(port, "ERR HMAC_FAILED\r\n");
            return;
        }
#endif

        char body_copy[128];
        strncpy(body_copy, body, sizeof(body_copy) - 1);
        body_copy[sizeof(body_copy) - 1] = '\0';

        char *saveptr = NULL;
        char *seq = strtok_r(body_copy, ",", &saveptr);
        char *nonce = strtok_r(NULL, ",", &saveptr);
        char *cmd = strtok_r(NULL, ",", &saveptr);
        char *payload = strtok_r(NULL, "", &saveptr);

        (void)seq;
        (void)nonce;

        if (!cmd) {
            serial_write_str(port, "ERR MISSING_CMD\r\n");
            return;
        }

        execute_command(port, cmd, payload ? payload : "");
        return;
    }

    /* 3. Direct unauthenticated commands if allowed */
#if SERIAL_SEC_BYPASS
    char line_copy[128];
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';
    char *saveptr = NULL;
    char *cmd = strtok_r(line_copy, " ", &saveptr);
    char *payload = strtok_r(NULL, "", &saveptr);
    if (cmd) {
        execute_command(port, cmd, payload ? payload : "");
    }
#endif
}

static void serial_hmi_worker_task(void *pv)
{
    (void)pv;
    static uint8_t uart_rx_buf[256];
    static char line_buf_u0[256];
    static size_t line_len_u0 = 0;
    static char line_buf_usb[256];
    static size_t line_len_usb = 0;

    while (1) {
        if (s_shot_state == SHOT_STATE_READY) {
            stream_screenshot();
        }

        if (s_shot_state == SHOT_STATE_COPYING) {
            int64_t now_ms = esp_timer_get_time() / 1000;
            if (now_ms - s_shot_t0 > SHOT_TIMEOUT_MS) {
                if (s_shot_rows_copied >= (SHOT_H * 3 / 4)) {
                    s_shot_state = SHOT_STATE_READY;
                    if (s_serial_task_handle) {
                        xTaskNotifyGive(s_serial_task_handle);
                    }
                } else {
                    serial_write_str(s_shot_port, "SHOT_ERR timeout\r\n");
                    s_shot_state = SHOT_STATE_IDLE;
                }
            }
        }

        /* Read from UART0 (CP210x / Hardware UART) */
        int rx = uart_read_bytes(UART_NUM_0, uart_rx_buf, sizeof(uart_rx_buf) - 1, pdMS_TO_TICKS(10));
        if (rx > 0) {
            for (int i = 0; i < rx; i++) {
                char c = (char)uart_rx_buf[i];
                if (c == '\r' || c == '\n') {
                    if (line_len_u0 > 0) {
                        line_buf_u0[line_len_u0] = '\0';
                        process_line(PORT_UART0, line_buf_u0);
                        line_len_u0 = 0;
                    }
                } else if (line_len_u0 < sizeof(line_buf_u0) - 1) {
                    line_buf_u0[line_len_u0++] = c;
                }
            }
        }

        /* Read from USB-Serial-JTAG */
        rx = usb_serial_jtag_read_bytes(uart_rx_buf, sizeof(uart_rx_buf) - 1, 0);
        if (rx > 0) {
            for (int i = 0; i < rx; i++) {
                char c = (char)uart_rx_buf[i];
                if (c == '\r' || c == '\n') {
                    if (line_len_usb > 0) {
                        line_buf_usb[line_len_usb] = '\0';
                        process_line(PORT_USB_JTAG, line_buf_usb);
                        line_len_usb = 0;
                    }
                } else if (line_len_usb < sizeof(line_buf_usb) - 1) {
                    line_buf_usb[line_len_usb++] = c;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#define SERIAL_HMI_STACK_WORDS 4096 /* 16384 bytes static stack */
static StaticTask_t s_serial_hmi_tcb;
static StackType_t s_serial_hmi_stack[SERIAL_HMI_STACK_WORDS] __attribute__((aligned(16)));

esp_err_t serial_hmi_init(void)
{
    /* 1. Install & configure UART0 at 921,600 baud */
    if (!uart_is_driver_installed(UART_NUM_0)) {
        uart_config_t uart_config = {
            .baud_rate = SERIAL_HMI_BAUD,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        esp_err_t err = uart_param_config(UART_NUM_0, &uart_config);
        if (err == ESP_OK) {
            uart_driver_install(UART_NUM_0, SERIAL_HMI_RX_BUF_SIZE, SERIAL_HMI_TX_BUF_SIZE, 0, NULL, 0);
            uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            /* Do NOT call uart_vfs_dev_use_driver(UART_NUM_0); it hijacks console stdout and breaks multicore locks */
        }
    } else {
        uart_set_baudrate(UART_NUM_0, SERIAL_HMI_BAUD);
    }

    /* 2. Install USB-Serial-JTAG driver if available */
    usb_serial_jtag_driver_config_t jtag_cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    jtag_cfg.rx_buffer_size = 1024;
    jtag_cfg.tx_buffer_size = 4096;
    esp_err_t jerr = usb_serial_jtag_driver_install(&jtag_cfg);
    if (jerr != ESP_OK && jerr != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "USB-Serial-JTAG driver notice: %s", esp_err_to_name(jerr));
    }

    /* 3. Spawn background worker task on Core 0 statically */
    s_serial_task_handle = xTaskCreateStaticPinnedToCore(
        serial_hmi_worker_task,
        "serial_hmi",
        SERIAL_HMI_STACK_WORDS,
        NULL,
        4,
        s_serial_hmi_stack,
        &s_serial_hmi_tcb,
        0
    );

    if (!s_serial_task_handle) {
        ESP_LOGE(TAG, "Failed to create serial_hmi task statically");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Serial HMI & Snapshot worker started on Core 0 (baud %d)", SERIAL_HMI_BAUD);
    return ESP_OK;
}

bool serial_hmi_capture_sync(uint32_t timeout_ms)
{
    serial_hmi_arm_shot(PORT_INTERNAL, SHOT_MODE_RAW);
    int64_t t0 = esp_timer_get_time() / 1000;
    while ((esp_timer_get_time() / 1000 - t0) < timeout_ms) {
        if (s_shot_state == SHOT_STATE_READY) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return (s_shot_state == SHOT_STATE_READY || (s_shot_buf && s_shot_rows_copied >= (SHOT_H * 3 / 4)));
}

const uint16_t* serial_hmi_get_buffer(void)
{
    return s_shot_buf;
}

void serial_hmi_release_shot(void)
{
    /* اجازه می‌دهد درخواست بعدی /api/screenshot یک کادر تازه را ضبط کند */
    s_shot_state = SHOT_STATE_IDLE;
}
