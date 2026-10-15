#include "cloud_client.h"
#include "mqtt_client.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "vars.h"
#include "storage.h"

static const char *TAG = "WELD_CLOUD";

#define DEFAULT_CLOUD_BROKER_URI "mqtt://broker.hivemq.com:1883"
#define DEFAULT_TELEMETRY_PERIOD_MS 10000

/* Extern hardware pins defined in main.cpp / storage */
extern int SSR_1, TABLE, PRESS_LEFT, PRESS_RIGHT, ELEMENT, SHABLON, ZARBEH1, ZARBEH2;
extern int EMERGENCY, PEDAL;
extern Storage storage;

/* Extern UI and agent triggers */
extern volatile int g_agent_nav_request;
extern volatile bool g_agent_touch_active;
extern volatile int16_t g_agent_touch_x;
extern volatile int16_t g_agent_touch_y;
extern volatile uint32_t g_agent_touch_release_ms;
extern "C" void apply_language_now(int32_t lang);

/* State variables */
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_is_connected = false;
static bool s_wifi_ready = false;
static char s_device_id[32] = {0};
static char s_topic_online[64] = {0};
static char s_topic_telemetry[64] = {0};
static char s_topic_cmd[64] = {0};
static char s_topic_ack[64] = {0};
static char s_broker_uri[128] = {0};

static const char *const s_default_brokers[] = {
    "mqtt://broker.hivemq.com:1883",
    "mqtt://test.mosquitto.org:1883",
    "mqtt://broker.emqx.io:1883"
};
static int s_broker_idx = 0;
static int s_disc_retries = 0;
static bool s_reconnect_needed = false;
static int s_saved_target_temp = 240;

static TaskHandle_t s_cloud_task_h = NULL;
static SemaphoreHandle_t s_trigger_sem = NULL;

/* ---------------- Forward Declarations ---------------- */
static void cloud_worker_task(void *pvParameters);
static void handle_cloud_command(const char *payload, int len);
static void publish_telemetry(void);
static void publish_online_state(bool online);

static void cloud_safe_stop(void)
{
    if (SSR_1 >= 0) digitalWrite(SSR_1, HIGH);
    if (TABLE >= 0) digitalWrite(TABLE, HIGH);
    if (PRESS_LEFT >= 0) digitalWrite(PRESS_LEFT, HIGH);
    if (PRESS_RIGHT >= 0) digitalWrite(PRESS_RIGHT, HIGH);
    if (ELEMENT >= 0) digitalWrite(ELEMENT, HIGH);
    if (SHABLON >= 0) digitalWrite(SHABLON, HIGH);
    if (ZARBEH1 >= 0) digitalWrite(ZARBEH1, HIGH);
    if (ZARBEH2 >= 0) digitalWrite(ZARBEH2, HIGH);
    set_var_emergency_stop(true);
    set_var_step_process(false);
    set_var_josh_is_working(false);
    set_var_cool_is_working(false);
    set_var_cycel_started(false);
}

static void init_device_identity(void)
{
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        snprintf(s_device_id, sizeof(s_device_id), "HAFFLER-WELD-%02X%02X%02X", mac[3], mac[4], mac[5]);
    } else {
        snprintf(s_device_id, sizeof(s_device_id), "WELD-01");
    }

    snprintf(s_topic_online, sizeof(s_topic_online), "haffler/welding_single_head/%s/online", s_device_id);
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "haffler/welding_single_head/%s/telemetry", s_device_id);
    snprintf(s_topic_cmd, sizeof(s_topic_cmd), "haffler/welding_single_head/%s/cmd", s_device_id);
    snprintf(s_topic_ack, sizeof(s_topic_ack), "haffler/welding_single_head/%s/ack", s_device_id);

    strncpy(s_broker_uri, DEFAULT_CLOUD_BROKER_URI, sizeof(s_broker_uri) - 1);
}

/* ---------------- MQTT Event Callback ---------------- */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        s_is_connected = true;
        s_disc_retries = 0;
        s_reconnect_needed = false;
        Serial.printf("[CLOUD] Connected to Haffler Cloud Broker (%s)\n", s_broker_uri);

        /* Subscribe to own unique cmd topic, generic WELD-01 topic, and broadcasts */
        esp_mqtt_client_subscribe(s_mqtt_client, s_topic_cmd, 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "haffler/welding_single_head/WELD-01/cmd", 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "haffler/welding_single_head/broadcast/cmd", 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "haffler/all/cmd", 1);

        publish_online_state(true);
        publish_telemetry();
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_is_connected = false;
        s_disc_retries++;
        Serial.printf("[CLOUD] Disconnected from Haffler Cloud Broker (attempt %d/4)\n", s_disc_retries);
        if (s_disc_retries >= 4) {
            s_disc_retries = 0;
            s_broker_idx = (s_broker_idx + 1) % 3;
            strncpy(s_broker_uri, s_default_brokers[s_broker_idx], sizeof(s_broker_uri) - 1);
            Serial.printf("[CLOUD] Rotating to fallback Cloud Broker: %s\n", s_broker_uri);
            s_reconnect_needed = true;
            if (s_trigger_sem) xSemaphoreGive(s_trigger_sem);
        }
        break;

    case MQTT_EVENT_SUBSCRIBED:
        Serial.printf("[CLOUD] Subscribed successfully, msg_id=%d\n", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        if (event->data_len > 0) {
            char *buf = (char *)malloc(event->data_len + 1);
            if (buf) {
                memcpy(buf, event->data, event->data_len);
                buf[event->data_len] = '\0';
                Serial.printf("[CLOUD] Received Command: %s\n", buf);
                handle_cloud_command(buf, event->data_len);
                free(buf);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        Serial.println("[CLOUD] MQTT error event");
        break;

    default:
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
}

/* ---------------- Client Lifecycle ---------------- */
static void start_mqtt_client(void)
{
    if (s_mqtt_client != NULL) return;

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.uri = s_broker_uri;
    mqtt_cfg.client_id = s_device_id;
    mqtt_cfg.lwt_topic = s_topic_online;
    mqtt_cfg.lwt_msg = "{\"online\":false}";
    mqtt_cfg.lwt_qos = 1;
    mqtt_cfg.lwt_retain = 1;
    mqtt_cfg.keepalive = 60;
    mqtt_cfg.task_prio = 3;
    mqtt_cfg.task_stack = 4096;
    mqtt_cfg.buffer_size = 1024;
    mqtt_cfg.out_buffer_size = 1024;

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client != NULL) {
        esp_mqtt_client_register_event(s_mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, s_mqtt_client);
        esp_err_t err = esp_mqtt_client_start(s_mqtt_client);
        if (err != ESP_OK) {
            Serial.printf("[CLOUD] esp_mqtt_client_start failed: 0x%x\n", err);
        } else {
            Serial.println("[CLOUD] esp_mqtt_client_start OK");
        }
    } else {
        Serial.println("[CLOUD] esp_mqtt_client_init failed (NULL handle)");
    }
}

static void stop_mqtt_client(void)
{
    if (s_mqtt_client != NULL) {
        publish_online_state(false);
        esp_mqtt_client_stop(s_mqtt_client);
        esp_mqtt_client_destroy(s_mqtt_client);
        s_mqtt_client = NULL;
        s_is_connected = false;
        Serial.println("[CLOUD] MQTT client stopped cleanly");
    }
}

/* ---------------- Publish Online State ---------------- */
static void publish_online_state(bool online)
{
    if (!s_mqtt_client || !s_is_connected) return;

    String ip = WiFi.localIP().toString();
    int rssi = WiFi.RSSI();

    char buf[384];
    snprintf(buf, sizeof(buf),
             "{\"online\":%s,\"dev_id\":\"%s\",\"model\":\"WELDING SINGLE HEAD\","
             "\"ver\":\"%s\",\"ip\":\"%s\",\"rssi\":%d}",
             online ? "true" : "false", s_device_id,
             VERSION, ip.c_str(), rssi);

    esp_mqtt_client_publish(s_mqtt_client, s_topic_online, buf, strlen(buf), 1, 1);
    esp_mqtt_client_publish(s_mqtt_client, "haffler/welding_single_head/WELD-01/online", buf, strlen(buf), 1, 1);
}

/* ---------------- Publish Telemetry ---------------- */
static void publish_telemetry(void)
{
    if (!s_mqtt_client || !s_is_connected) return;

    bool ssr_on   = (SSR_1 >= 0) ? (digitalRead(SSR_1) == ON) : false;
    bool tbl_on   = (TABLE >= 0) ? (digitalRead(TABLE) == ON) : false;
    bool pl_on    = (PRESS_LEFT >= 0) ? (digitalRead(PRESS_LEFT) == ON) : false;
    bool pr_on    = (PRESS_RIGHT >= 0) ? (digitalRead(PRESS_RIGHT) == ON) : false;
    bool elem_on  = (ELEMENT >= 0) ? (digitalRead(ELEMENT) == ON) : false;
    bool shab_on  = (SHABLON >= 0) ? (digitalRead(SHABLON) == ON) : false;
    bool em_on    = (EMERGENCY >= 0) ? (digitalRead(EMERGENCY) == LOW) : false;
    bool ped_on   = (PEDAL >= 0) ? (digitalRead(PEDAL) == LOW) : false;

    float t_act   = (float)get_var_element_temperature();
    float t_tgt   = (float)get_var_element_temperature_target();
    int m_time    = (int)get_var_welding_time();
    int c_time    = (int)get_var_cooling_time();
    long w_cnt    = (long)get_var_total_frame_numbers();
    long d_cnt    = (long)get_var_frame_numbers();

    bool is_working = get_var_josh_is_working() || get_var_step_process();
    const char *fsm = is_working ? "welding" : (em_on ? "emergency" : "ready");
    const char *fsm_fa = is_working ? "در حال چرخه جوشکاری" : (em_on ? "توقف اضطراری E-STOP" : "دمای اتو آماده جوش");

    int rssi = WiFi.RSSI();
    uint32_t free_heap = (uint32_t)ESP.getFreeHeap();
    int64_t uptime_sec = esp_timer_get_time() / 1000000;

    static char buf[1024];
    snprintf(buf, sizeof(buf),
        "{\"dev_id\":\"%s\",\"model\":\"WELDING SINGLE HEAD\",\"online\":true,"
        "\"temp_actual\":%.1f,\"temp_target\":%.1f,\"melt_time\":%d,\"weld_time\":%d,"
        "\"ssr\":%d,\"clamp_left\":%d,\"clamp_right\":%d,\"table\":%d,\"heater_jack\":%d,\"shablon\":%d,"
        "\"weld_count\":%ld,\"daily_frames\":%ld,"
        "\"fsm_state\":\"%s\",\"fsm_state_fa\":\"%s\","
        "\"emergency\":%d,\"pedal\":%d,\"rssi\":%d,\"free_heap\":%u,\"uptime\":%lld}",
        s_device_id,
        t_act, t_tgt, m_time, c_time,
        ssr_on ? 1 : 0, pl_on ? 1 : 0, pr_on ? 1 : 0, tbl_on ? 1 : 0, elem_on ? 1 : 0, shab_on ? 1 : 0,
        w_cnt, d_cnt,
        fsm, fsm_fa,
        em_on ? 1 : 0, ped_on ? 1 : 0, rssi, free_heap, uptime_sec);

    esp_mqtt_client_publish(s_mqtt_client, s_topic_telemetry, buf, strlen(buf), 0, 0);
    esp_mqtt_client_publish(s_mqtt_client, "haffler/welding_single_head/WELD-01/telemetry", buf, strlen(buf), 0, 0);
}

/* ---------------- Command Execution ---------------- */
static void handle_cloud_command(const char *payload, int len)
{
    (void)len;
    bool success = true;
    const char *msg = "OK";
    char action_name[48] = {0};
    char req_id[32] = {0};

    /* Extract req_id */
    const char *p_req = strstr(payload, "\"req_id\":");
    if (p_req) {
        p_req = strchr(p_req, ':');
        if (p_req) {
            p_req++;
            while (*p_req == ' ' || *p_req == '\"') p_req++;
            size_t idx = 0;
            while (*p_req && *p_req != '\"' && *p_req != ',' && *p_req != '}' && idx + 1 < sizeof(req_id)) {
                req_id[idx++] = *p_req++;
            }
            req_id[idx] = '\0';
        }
    }

    /* Extract action */
    const char *p_act = strstr(payload, "\"action\":");
    if (p_act) {
        p_act = strchr(p_act, ':');
        if (p_act) {
            p_act++;
            while (*p_act == ' ' || *p_act == '\"') p_act++;
            size_t idx = 0;
            while (*p_act && *p_act != '\"' && *p_act != ',' && *p_act != '}' && idx + 1 < sizeof(action_name)) {
                action_name[idx++] = *p_act++;
            }
            action_name[idx] = '\0';
        }
    }

    if (strcmp(action_name, "estop") == 0 || strcmp(action_name, "safe_stop") == 0) {
        cloud_safe_stop();
        msg = "Emergency Stop Executed (توقف اضطراری کامل اجرا شد)";
    }
    else if (strcmp(action_name, "start_weld") == 0 || strcmp(action_name, "weld_cycle") == 0) {
        if (get_var_thermocouple_error()) {
            success = false;
            msg = "خطا: سنسور ترموکوپل قطع است — شروع سیکل جوشکاری متوقف شد";
        } else if (get_var_emergency_stop()) {
            success = false;
            msg = "خطا: شستی استپ اضطراری فعال است";
        } else {
            set_var_step_process(true);
            msg = "سیکل جوشکاری با موفقیت آغاز شد";
        }
    }
    else if (strcmp(action_name, "heater_toggle") == 0 || strcmp(action_name, "heater_on") == 0 || strcmp(action_name, "heater_off") == 0) {
        int cur_target = get_var_element_temperature_target();
        bool turn_on = false;
        if (strcmp(action_name, "heater_on") == 0) {
            turn_on = true;
        } else if (strcmp(action_name, "heater_off") == 0) {
            turn_on = false;
        } else {
            turn_on = (cur_target <= 0);
        }

        if (turn_on) {
            int target_to_restore = (s_saved_target_temp > 0) ? s_saved_target_temp : storage.getTempTarget(storage.getSelectedUser());
            if (target_to_restore <= 0) target_to_restore = 240;
            set_var_element_temperature_target(target_to_restore);
            storage.setTempTarget(target_to_restore, storage.getSelectedUser());
            msg = "المنت هیتر فعال شد (تنظیم دمای هدف)";
        } else {
            if (cur_target > 0) s_saved_target_temp = cur_target;
            set_var_element_temperature_target(0);
            storage.setTempTarget(0, storage.getSelectedUser());
            if (SSR_1 >= 0) digitalWrite(SSR_1, HIGH); // Active-LOW OFF
            msg = "المنت هیتر خاموش شد (دمای هدف صفر شد)";
        }
    }
    else if (strcmp(action_name, "element_toggle") == 0) {
        if (ELEMENT >= 0) {
            int cur = digitalRead(ELEMENT);
            if (cur == HIGH) {
                // قصد پیشروی جک اتو (Active-LOW: ON = LOW)
                int pl = (PRESS_LEFT >= 0) ? digitalRead(PRESS_LEFT) : HIGH;
                int pr = (PRESS_RIGHT >= 0) ? digitalRead(PRESS_RIGHT) : HIGH;
                if (pl != LOW || pr != LOW) {
                    success = false;
                    msg = "خطای ایمنی اینترلاک زیمنس: جک اتو فقط در صورت قفل بودن هر دو فک کلمپ مجاز به پیشروی است";
                } else {
                    digitalWrite(ELEMENT, LOW);
                    msg = "جک اتو پیشروی کرد (ON)";
                }
            } else {
                digitalWrite(ELEMENT, HIGH);
                msg = "جک اتو عقب رفت (OFF)";
            }
        } else {
            success = false;
            msg = "پایه جک اتو تعریف نشده است";
        }
    }
    else if (strcmp(action_name, "clamp_toggle") == 0) {
        int cur_cl = (PRESS_LEFT >= 0) ? digitalRead(PRESS_LEFT) : HIGH;
        int next_cl = (cur_cl == LOW) ? HIGH : LOW;
        if (PRESS_LEFT >= 0) digitalWrite(PRESS_LEFT, next_cl);
        if (PRESS_RIGHT >= 0) digitalWrite(PRESS_RIGHT, next_cl);
        msg = (next_cl == LOW) ? "فک‌های گیره قفل شدند (ON)" : "فک‌های گیره باز شدند (OFF)";
    }
    else if (strcmp(action_name, "table_toggle") == 0) {
        if (TABLE >= 0) {
            int cur = digitalRead(TABLE);
            int next_v = (cur == LOW) ? HIGH : LOW;
            digitalWrite(TABLE, next_v);
            msg = (next_v == LOW) ? "جک صفحه پیشروی کرد (ON)" : "جک صفحه عقب رفت (OFF)";
        } else {
            success = false;
            msg = "پایه جک صفحه تعریف نشده است";
        }
    }
    else if (strcmp(action_name, "shablon_toggle") == 0) {
        if (SHABLON >= 0) {
            int cur = digitalRead(SHABLON);
            int next_v = (cur == LOW) ? HIGH : LOW;
            digitalWrite(SHABLON, next_v);
            msg = (next_v == LOW) ? "جک شابلون جلو آمد (ON)" : "جک شابلون عقب رفت (OFF)";
        } else {
            success = false;
            msg = "پایه جک شابلون تعریف نشده است";
        }
    }
    else if (strcmp(action_name, "set_temp") == 0) {
        const char *p_val = strstr(payload, "\"val\":");
        if (!p_val) p_val = strstr(payload, "\"temp\":");
        if (p_val) {
            p_val = strchr(p_val, ':');
            if (p_val) {
                int t = atoi(p_val + 1);
                if (t >= 0 && t <= 350) {
                    set_var_element_temperature_target(t);
                    storage.setTempTarget(t, storage.getSelectedUser());
                    msg = "دمای هدف اتو تنظیم و در NVS ذخیره شد";
                } else {
                    success = false;
                    msg = "مقدار دمای درخواستی خارج از محدوده مجاز (۰ تا ۳۵۰)";
                }
            }
        }
    }
    else if (strcmp(action_name, "nav") == 0) {
        const char *p_scr = strstr(payload, "\"target\":");
        if (!p_scr) p_scr = strstr(payload, "\"screen\":");
        if (p_scr) {
            p_scr = strchr(p_scr, ':');
            if (p_scr) {
                int scr = atoi(p_scr + 1);
                if (scr >= 1 && scr <= 11) {
                    g_agent_nav_request = scr;
                    msg = "دستور انتقال صفحه صادر شد";
                } else {
                    success = false;
                    msg = "شماره صفحه نامعتبر است";
                }
            }
        }
    }
    else if (strcmp(action_name, "lang") == 0) {
        const char *p_lng = strstr(payload, "\"lang\":");
        if (p_lng) {
            p_lng = strchr(p_lng, ':');
            if (p_lng) {
                int l = atoi(p_lng + 1);
                if (l >= 1 && l <= 3) {
                    apply_language_now(l);
                    msg = "زبان رابط کاربری دستگاه تغییر یافت";
                }
            }
        }
    }
    else if (strcmp(action_name, "tap") == 0) {
        const char *px = strstr(payload, "\"x\":");
        const char *py = strstr(payload, "\"y\":");
        if (px && py) {
            int x = atoi(strchr(px, ':') + 1);
            int y = atoi(strchr(py, ':') + 1);
            if (x >= 0 && x < 480 && y >= 0 && y < 320) {
                g_agent_touch_x = (int16_t)x;
                g_agent_touch_y = (int16_t)y;
                g_agent_touch_active = true;
                g_agent_touch_release_ms = millis() + 120;
                msg = "تزریق لمسی مختصات انجام شد";
            }
        }
    }
    else {
        success = false;
        msg = "فرمان ناشناخته برای دستگاه جوشکاری";
    }

    /* Publish ACK response */
    char ack_buf[384];
    snprintf(ack_buf, sizeof(ack_buf),
             "{\"req_id\":\"%s\",\"action\":\"%s\",\"status\":\"%s\",\"msg\":\"%s\"}",
             req_id, action_name, success ? "ok" : "error", msg);

    if (s_mqtt_client && s_is_connected) {
        esp_mqtt_client_publish(s_mqtt_client, s_topic_ack, ack_buf, strlen(ack_buf), 1, 0);
        esp_mqtt_client_publish(s_mqtt_client, "haffler/welding_single_head/WELD-01/ack", ack_buf, strlen(ack_buf), 1, 0);
    }

    /* Immediate telemetry trigger after any command */
    cloud_client_trigger_telemetry();
}

/* ---------------- FreeRTOS Worker Task on Core 0 ---------------- */
static void cloud_worker_task(void *pvParameters)
{
    (void)pvParameters;
    Serial.println("[CLOUD] Worker task started on Core 0");

    uint32_t last_telemetry_ms = 0;

    while (1) {
        if (s_wifi_ready) {
            if (s_reconnect_needed) {
                s_reconnect_needed = false;
                stop_mqtt_client();
                start_mqtt_client();
            } else if (s_mqtt_client == NULL) {
                start_mqtt_client();
            }
        } else {
            if (s_mqtt_client != NULL) {
                stop_mqtt_client();
            }
        }

        /* Wait up to 10 seconds or until triggered */
        if (s_trigger_sem) {
            xSemaphoreTake(s_trigger_sem, pdMS_TO_TICKS(1000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        uint32_t now = millis();
        if (s_is_connected && (now - last_telemetry_ms >= DEFAULT_TELEMETRY_PERIOD_MS)) {
            last_telemetry_ms = now;
            publish_telemetry();
        }
    }
}

/* ---------------- Public Interface Implementation ---------------- */
extern "C" void cloud_client_init(void)
{
    if (s_cloud_task_h != NULL) return;

    init_device_identity();

    if (!s_trigger_sem) {
        s_trigger_sem = xSemaphoreCreateBinary();
    }

    BaseType_t res = xTaskCreatePinnedToCore(
        cloud_worker_task,
        "CloudWorker",
        3072,
        NULL,
        2,
        &s_cloud_task_h,
        0 // Core 0 (System Core)
    );

    if (res != pdPASS) {
        res = xTaskCreatePinnedToCore(
            cloud_worker_task,
            "CloudWorker",
            2048,
            NULL,
            2,
            &s_cloud_task_h,
            0
        );
    }

    if (res != pdPASS) {
        Serial.println("[CLOUD] Error creating CloudWorker task");
    } else {
        Serial.printf("[CLOUD] Initialized for device %s\n", s_device_id);
    }
}

extern "C" void cloud_client_on_wifi_connected(void)
{
    s_wifi_ready = true;
    if (s_trigger_sem) {
        xSemaphoreGive(s_trigger_sem);
    }
}

extern "C" void cloud_client_on_wifi_disconnected(void)
{
    s_wifi_ready = false;
    if (s_trigger_sem) {
        xSemaphoreGive(s_trigger_sem);
    }
}

extern "C" bool cloud_client_is_connected(void)
{
    return s_is_connected;
}

extern "C" void cloud_client_trigger_telemetry(void)
{
    if (s_is_connected) {
        publish_telemetry();
    }
    if (s_trigger_sem) {
        xSemaphoreGive(s_trigger_sem);
    }
}

extern "C" const char *cloud_client_get_device_id(void)
{
    return s_device_id;
}
