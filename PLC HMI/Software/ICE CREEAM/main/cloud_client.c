#include "cloud_client.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_app_format.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "esp_app_desc.h"

/* Project headers */
#include "project_config.h"
#include "storage.h"
#include "buzzer.h"
#include "fault_log.h"
#include "day_lock_core.h"
#include "max6675.h"
#include "extra_modes.h"
#include "ui/ui_lang.h"
#include "wifi.h"
#include "lvgl_port.h"
#include "warranty.h"
#include "ota_update.h"
#include "analog_inputs.h"
#include "serial_hmi.h"

static const char *TAG = "ASTIAJ_cloud";

/* بروکر پیش‌فرض باید با سرور وب یکی باشد — هر دو hivemq */
#define DEFAULT_CLOUD_BROKER_URI "mqtt://broker.hivemq.com:1883"
#define DEFAULT_TELEMETRY_PERIOD_MS 10000

/* Externs from main PLC control engine */
extern int32_t paddle_timer_1, paddle_timer_2, startup_timer, mid_eye_timer;
extern int32_t restart_time_1, restart_time_2, modbus_delay;
extern int32_t screensaver_timeout_s;
extern int32_t current_set_1a, current_set_1b, current_set_2a, current_set_2b;
extern int32_t icecream_counter, icecream_counter_daily, icecream_counter_total, cone_price;
extern bool night_scheduler_en;
extern int32_t night_start_h, night_start_m, night_end_h, night_end_m;
extern bool s_drain_mode_active;
extern int32_t s_drain_timer_s;
extern bool plc_is_ready(void);
extern bool plc_is_ending(void);
extern bool plc_is_dispense(void);
extern bool plc_is_fault(void);
extern bool micro_front_active(void);
extern bool micro_rear_active(void);
extern void plc_stop_all(void);
extern void plc_hw_start_make(void);
extern void plc_notify_mode_cleared(void);
extern void plc_set_big_compressor(bool on);
extern void plc_notify_ice_started(void);
extern void plc_notify_paddle_started(void);
extern void ui_apply_state(void);
extern void app_state_save_to_nvs(void);
extern void ui_update_icecream_counter(void);
extern void ota_request_scan(void);

/* State variables */
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_is_connected = false;
static bool s_wifi_ready = false;
static char s_device_id[32] = {0};
static char s_customer_name[64] = {0};
static char s_city[48] = {0};
static char s_serial_no[32] = {0};
static char s_topic_online[64] = {0};
static char s_topic_telemetry[64] = {0};
static char s_topic_cmd[64] = {0};
static char s_topic_ack[64] = {0};
static char s_topic_cmd_resp[64] = {0};
static char s_broker_uri[128] = {0};

static TaskHandle_t s_cloud_task_h = NULL;
static SemaphoreHandle_t s_trigger_sem = NULL;

/* ---------------- Forward Declarations ---------------- */
static void cloud_worker_task(void *pvParameters);
static void handle_cloud_command(const char *payload, int len);
static void publish_telemetry(void);
static void publish_online_state(bool online);

/* ---------------- Unique Device ID & Profile ---------------- */
static void init_device_identity(void)
{
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        snprintf(s_device_id, sizeof(s_device_id), "ASTIAJ-ICE-%02X%02X%02X", mac[3], mac[4], mac[5]);
    } else {
        snprintf(s_device_id, sizeof(s_device_id), "ASTIAJ-ICE-DEFAULT");
    }

    /* Load user-defined customer name, city, serial from NVS */
    storage_get_str("cloud_cust", "کارگاه آستیاج", s_customer_name, sizeof(s_customer_name));
    storage_get_str("cloud_city", "تهران", s_city, sizeof(s_city));
    storage_get_str("cloud_sn", s_device_id, s_serial_no, sizeof(s_serial_no));

    snprintf(s_topic_online, sizeof(s_topic_online), "ASTIAJ/ice_cream/%s/online", s_device_id);
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "ASTIAJ/ice_cream/%s/telemetry", s_device_id);
    snprintf(s_topic_cmd, sizeof(s_topic_cmd), "ASTIAJ/ice_cream/%s/cmd", s_device_id);
    snprintf(s_topic_ack, sizeof(s_topic_ack), "ASTIAJ/ice_cream/%s/ack", s_device_id);
    snprintf(s_topic_cmd_resp, sizeof(s_topic_cmd_resp), "ASTIAJ/ice_cream/%s/cmd_resp", s_device_id);

    /* Load broker URI from NVS if set, else use default */
    char saved_uri[128] = {0};
    if (storage_get_str("cloud_broker", "", saved_uri, sizeof(saved_uri)) && saved_uri[0] != '\0') {
        /* هر بروکری که سرور آن را نمی‌شناسد → بازنشانی به hivemq (بروکر مشترک با سرور وب) */
        bool is_bad = (strstr(saved_uri, "broker.emqx.io") != NULL)
                   || (strstr(saved_uri, "ASTIAJ-ai.ir")  != NULL);
        if (is_bad) {
            strncpy(s_broker_uri, DEFAULT_CLOUD_BROKER_URI, sizeof(s_broker_uri) - 1);
            storage_set_str("cloud_broker", DEFAULT_CLOUD_BROKER_URI);
        } else {
            strncpy(s_broker_uri, saved_uri, sizeof(s_broker_uri) - 1);
        }
    } else {
        strncpy(s_broker_uri, DEFAULT_CLOUD_BROKER_URI, sizeof(s_broker_uri) - 1);
    }

    ESP_LOGI(TAG, "Device Identity: ID=%s, Cust=%s, City=%s, SN=%s, Broker=%s",
             s_device_id, s_customer_name, s_city, s_serial_no, s_broker_uri);
}

/* ترتیب بروکر: hivemq اول — سرور هم روی hivemq subscribe می‌کند → هر دو یک کانال */
static const char *const s_default_brokers[] = {
    "mqtt://broker.hivemq.com:1883",
    "mqtt://ASTIAJ-ai.ir:1883",
    "mqtt://test.mosquitto.org:1883",
    "mqtt://broker.emqx.io:1883"
};
static int s_broker_idx = 0;
static int s_disc_retries = 0;
static bool s_reconnect_needed = false;

/* ---------------- MQTT Event Handler ---------------- */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_is_connected = true;
        s_disc_retries = 0;
        s_reconnect_needed = false;
        ESP_LOGI(TAG, "Connected to ASTIAJ/Haffler cloud Broker (%s) -> Subscribing to command topics", s_broker_uri);
        esp_mqtt_client_subscribe(s_mqtt_client, s_topic_cmd, 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "ASTIAJ/ice_cream/broadcast/cmd", 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "ASTIAJ/ice_cream/all/cmd", 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "ASTIAJ/ice_creeam/broadcast/cmd", 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "ASTIAJ/ice_creeam/all/cmd", 1);
        char alt_topic[64];
        snprintf(alt_topic, sizeof(alt_topic), "ASTIAJ/ice_creeam/%s/cmd", s_device_id);
        esp_mqtt_client_subscribe(s_mqtt_client, alt_topic, 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "ASTIAJ/all/cmd", 1);

        /* Haffler standard namespaces */
        char haf_cmd[64];
        snprintf(haf_cmd, sizeof(haf_cmd), "haffler/ice_cream/%s/cmd", s_device_id);
        esp_mqtt_client_subscribe(s_mqtt_client, haf_cmd, 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "haffler/ice_cream/broadcast/cmd", 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "haffler/ice_cream/all/cmd", 1);
        esp_mqtt_client_subscribe(s_mqtt_client, "haffler/all/cmd", 1);

        publish_online_state(true);
        cloud_client_publish_telemetry_now();
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_is_connected = false;
        s_disc_retries++;
        ESP_LOGW(TAG, "Disconnected from ASTIAJ cloud Broker (attempt %d/4)", s_disc_retries);
        if (s_disc_retries >= 4) {
            s_disc_retries = 0;
            s_broker_idx = (s_broker_idx + 1) % 4;
            strncpy(s_broker_uri, s_default_brokers[s_broker_idx], sizeof(s_broker_uri) - 1);
            ESP_LOGW(TAG, "Rotating to fallback Cloud Broker: %s", s_broker_uri);
            s_reconnect_needed = true;
            if (s_trigger_sem) xSemaphoreGive(s_trigger_sem);
        }
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "Subscribed successfully, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        if (event->data_len > 0) {
            char *buf = malloc(event->data_len + 1);
            if (buf) {
                memcpy(buf, event->data, event->data_len);
                buf[event->data_len] = '\0';
                ESP_LOGI(TAG, "Received Cloud Command: %s", buf);
                handle_cloud_command(buf, event->data_len);
                free(buf);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error occurred");
        break;

    default:
        break;
    }
}

/* ---------------- Start / Stop Internal Client ---------------- */
static void start_mqtt_client(void)
{
    if (s_mqtt_client != NULL) {
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = s_broker_uri,
        .credentials.client_id = s_device_id,
        .session = {
            .last_will = {
                .topic = s_topic_online,
                .msg = "{\"online\":false}",
                .msg_len = 16,
                .qos = 1,
                .retain = 1,
            },
            .keepalive = 60,
        },
        .network = {
            .reconnect_timeout_ms = 5000,
            .timeout_ms = 10000,
        },
        .task = {
            .priority = 3,
            .stack_size = 4096,
        }
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client) {
        esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(s_mqtt_client);
        ESP_LOGI(TAG, "MQTT client started for URI: %s", s_broker_uri);
    } else {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
    }
}

static void stop_mqtt_client(void)
{
    if (s_mqtt_client) {
        publish_online_state(false);
        esp_mqtt_client_stop(s_mqtt_client);
        esp_mqtt_client_destroy(s_mqtt_client);
        s_mqtt_client = NULL;
        s_is_connected = false;
        ESP_LOGI(TAG, "MQTT client stopped");
    }
}

/* ---------------- Publish Online State ---------------- */
static void publish_online_state(bool online)
{
    if (!s_mqtt_client || !s_is_connected) return;

    const esp_app_desc_t *desc = esp_app_get_description();
    const char *ip = wifi_get_ip_str();

    wifi_ap_record_t ap_info;
    int rssi = 0;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        rssi = ap_info.rssi;
    }

    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"online\":%s,\"dev_id\":\"%s\",\"model\":\"ICE CREAM CM-700\","
             "\"cust\":\"%s\",\"city\":\"%s\",\"sn\":\"%s\","
             "\"ver\":\"%s\",\"ip\":\"%s\",\"rssi\":%d}",
             online ? "true" : "false", s_device_id,
             s_customer_name, s_city, s_serial_no,
             desc ? desc->version : "1.4.2", ip ? ip : "", rssi);

    esp_mqtt_client_publish(s_mqtt_client, s_topic_online, buf, strlen(buf), 1, 1);
    char haf_online[64];
    snprintf(haf_online, sizeof(haf_online), "haffler/ice_cream/%s/online", s_device_id);
    esp_mqtt_client_publish(s_mqtt_client, haf_online, buf, strlen(buf), 1, 1);
}

/* ---------------- Publish Telemetry ---------------- */
static void publish_telemetry(void)
{
    if (!s_mqtt_client || !s_is_connected) return;

    bool out_comp = (gpio_get_level(Compressor) == 0);
    bool out_make = (gpio_get_level(Make_Icecream) == 0);
    bool out_pdl = (gpio_get_level(Paddle) == 0);
    bool out_cream = (gpio_get_level(CREAM_PUMP_GPIO) == 0);
    bool out_air = (gpio_get_level(AIR_SOLENOID_GPIO) == 0);
    bool out_rb = (gpio_get_level(RAINBOW_GPIO) == 0);

    bool in_front = micro_front_active();
    bool in_rear = micro_rear_active();
    bool in_eye = inductive_active();
    bool in_stop = stop_button_active();
    bool in_start = start_button_active();
    bool in_loadm = load_motor_tripped();
    bool in_loadc = load_comp_tripped();

    float temp_top = max6675_get_temp_top();
    float temp_cyl = max6675_get_temp_cylinder();
    bool temp_top_ok = max6675_is_top_valid();
    bool temp_cyl_ok = max6675_is_cylinder_valid();

    wifi_ap_record_t ap_info;
    int rssi = 0;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        rssi = ap_info.rssi;
    }
    uint32_t free_heap = esp_get_free_heap_size();
    int64_t uptime_sec = esp_timer_get_time() / 1000000;

    /* Get last recorded fault */
    int f_count = fault_log_get_count();
    int last_f_code = 0;
    uint32_t last_f_time = 0;
    if (f_count > 0) {
        fault_entry_t e;
        if (fault_log_get_entry(0, &e)) {
            last_f_code = e.code;
            last_f_time = e.timestamp;
        }
    }

    /* Get OTA State */
    ota_install_state_t ota_st = ota_get_install_state();
    uint8_t ota_pct = ota_get_install_pct();

    static char buf[1400];
    snprintf(buf, sizeof(buf),
        "{\"dev_id\":\"%s\",\"model\":\"ICE CREAM CM-700\",\"online\":true,"
        "\"cust\":\"%s\",\"city\":\"%s\",\"sn\":\"%s\","
        "\"t_top\":%.1f,\"t_cyl\":%.1f,\"t_top_ok\":%d,\"t_cyl_ok\":%d,"
        "\"make\":%d,\"hold\":%d,\"paddle\":%d,\"wash\":%d,\"edge\":%d,\"rainbow\":%d,\"drain\":%d,"
        "\"ready\":%d,\"dispense\":%d,\"ending\":%d,\"fault\":%d,"
        "\"in_front\":%d,\"in_rear\":%d,\"in_eye\":%d,\"in_stop\":%d,\"in_start\":%d,\"in_loadm\":%d,\"in_loadc\":%d,"
        "\"out_comp\":%d,\"out_make\":%d,\"out_pdl\":%d,\"out_cream\":%d,\"out_air\":%d,\"out_rb\":%d,"
        "\"scoops\":%ld,\"scoops_day\":%ld,\"scoops_tot\":%ld,\"cone_price\":%ld,"
        "\"lease_st\":%u,\"lease_days\":%d,\"lease_ch\":%lu,\"lease_locked\":%d,\"lease_soft\":%d,"
        "\"f_cnt\":%d,\"last_f\":%d,\"last_f_t\":%lu,"
        "\"ota_st\":%d,\"ota_pct\":%u,"
        "\"rssi\":%d,\"free_heap\":%" PRIu32 ",\"uptime\":%" PRId64 "}",
        s_device_id,
        s_customer_name, s_city, s_serial_no,
        temp_top, temp_cyl, temp_top_ok ? 1 : 0, temp_cyl_ok ? 1 : 0,
        make_start ? 1 : 0, ice_start ? 1 : 0, paddle_start ? 1 : 0, wash_is_active() ? 1 : 0,
        edge_is_active() ? 1 : 0, rainbow_is_active() ? 1 : 0, s_drain_mode_active ? 1 : 0,
        plc_is_ready() ? 1 : 0, plc_is_dispense() ? 1 : 0, plc_is_ending() ? 1 : 0, plc_is_fault() ? 1 : 0,
        in_front ? 1 : 0, in_rear ? 1 : 0, in_eye ? 1 : 0, in_stop ? 1 : 0, in_start ? 1 : 0, in_loadm ? 1 : 0, in_loadc ? 1 : 0,
        out_comp ? 1 : 0, out_make ? 1 : 0, out_pdl ? 1 : 0, out_cream ? 1 : 0, out_air ? 1 : 0, out_rb ? 1 : 0,
        (long)icecream_counter, (long)icecream_counter_daily, (long)icecream_counter_total, (long)cone_price,
        (unsigned)day_lock_get_active_stage(), (int)day_lock_get_remain_days(),
        (unsigned long)day_lock_get_active_challenge(),
        day_lock_is_locked() ? 1 : 0, day_lock_is_soft_locked() ? 1 : 0,
        f_count, last_f_code, (unsigned long)last_f_time,
        (int)ota_st, (unsigned)ota_pct,
        rssi, free_heap, uptime_sec);

    esp_mqtt_client_publish(s_mqtt_client, s_topic_telemetry, buf, strlen(buf), 0, 0);
    char haf_telem[64];
    snprintf(haf_telem, sizeof(haf_telem), "haffler/ice_cream/%s/telemetry", s_device_id);
    esp_mqtt_client_publish(s_mqtt_client, haf_telem, buf, strlen(buf), 0, 0);
}

/* ---------------- OTA Progress Callback for Cloud ---------------- */
static void ota_cloud_progress_cb(uint32_t dl_bytes, uint32_t total_bytes, uint8_t pct)
{
    static uint8_t s_last_pub_pct = 255;
    if (s_mqtt_client && s_is_connected) {
        if (pct == 0 || pct == 100 || (pct >= s_last_pub_pct + 10)) {
            s_last_pub_pct = pct;
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "{\"dev_id\":\"%s\",\"event\":\"ota_progress\",\"pct\":%u,\"dl_bytes\":%lu,\"total_bytes\":%lu}",
                     s_device_id, (unsigned)pct, (unsigned long)dl_bytes, (unsigned long)total_bytes);
            esp_mqtt_client_publish(s_mqtt_client, s_topic_cmd_resp, buf, strlen(buf), 1, 0);
        }
    }
}

/* ---------------- Command Execution ---------------- */
static void handle_cloud_command(const char *payload, int len)
{
    (void)len;
    bool success = true;
    const char *msg = "OK";
    char req_id[32] = {0};
    char action_name[64] = "unknown";

    /* Extract optional request ID for tracking */
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

    /* Extract action name */
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

    if (strstr(payload, "\"action\":\"estop\"")) {
        plc_stop_all();
        buzzer_beep_alarm();
        msg = "Emergency Stop Executed";
    }
    else if (strstr(payload, "\"action\":\"make_start\"")) {
        plc_hw_start_make();
        msg = "Production cycle started";
    }
    else if (strstr(payload, "\"action\":\"make_stop\"")) {
        if (make_start) {
            plc_notify_mode_cleared();
            pin_write(Paddle, 1);
            plc_set_big_compressor(false);
            pin_write(Compressor, 1);
            make_start = false;
            ui_apply_state();
            msg = "Production stopped";
        }
    }
    else if (strstr(payload, "\"action\":\"make_toggle\"")) {
        if (make_start) {
            plc_notify_mode_cleared();
            pin_write(Paddle, 1);
            plc_set_big_compressor(false);
            pin_write(Compressor, 1);
            make_start = false;
            ui_apply_state();
            msg = "Production toggled OFF";
        } else {
            plc_hw_start_make();
            msg = "Production toggled ON";
        }
    }
    else if (strstr(payload, "\"action\":\"hold_toggle\"")) {
        if (ice_start) {
            ice_start = false;
            pin_write(Compressor, 1);
            ui_apply_state();
            msg = "Night hold toggled OFF";
        } else {
            if (!day_lock_is_soft_locked() && !plc_is_fault() && !wash_is_active() && !make_start && !warranty_is_expired_and_locked()) {
                ice_start = true;
                plc_notify_ice_started();
                ui_apply_state();
                msg = "Night hold toggled ON";
            } else {
                success = false;
                msg = "Hold blocked by interlock";
            }
        }
    }
    else if (strstr(payload, "\"action\":\"paddle_toggle\"")) {
        if (paddle_start) {
            paddle_start = false;
            pin_write(Paddle, 1);
            ui_apply_state();
            msg = "Paddle mixer toggled OFF";
        } else {
            if (!day_lock_is_soft_locked() && !plc_is_fault() && !wash_is_active() && !make_start && !warranty_is_expired_and_locked()) {
                paddle_start = true;
                plc_notify_paddle_started();
                pin_write(Paddle, 0);
                ui_apply_state();
                msg = "Paddle mixer toggled ON";
            } else {
                success = false;
                msg = "Paddle blocked by interlock";
            }
        }
    }
    else if (strstr(payload, "\"action\":\"wash_toggle\"")) {
        wash_toggle();
        ui_apply_state();
        msg = "Wash mode toggled";
    }
    else if (strstr(payload, "\"action\":\"edge_toggle\"") || strstr(payload, "\"action\":\"edge_start\"")) {
        edge_toggle_produce();
        ui_apply_state();
        msg = "Color edge toggled";
    }
    else if (strstr(payload, "\"action\":\"milk_mixer_toggle\"") || strstr(payload, "\"action\":\"milk_mixer_start\"") ||
             strstr(payload, "\"action\":\"rainbow_toggle\"") || strstr(payload, "\"action\":\"rainbow_start\"")) {
        rainbow_toggle_rp();
        ui_apply_state();
        msg = "Rainbow valve toggled";
    }
    else if (strstr(payload, "\"action\":\"take_screenshot\"") || strstr(payload, "\"action\":\"screenshot\"")) {
        bool shot_ok = serial_hmi_capture_sync(800);
        success = shot_ok;
        msg = shot_ok ? "Screenshot captured in memory" : "Screenshot capture timeout";
    }
    else if (strstr(payload, "\"action\":\"toggle_compressor\"") || strstr(payload, "\"action\":\"relay_compressor\"")) {
        bool cur = (gpio_get_level(Compressor) == 0);
        pin_write(Compressor, cur ? 1 : 0);
        msg = cur ? "Compressor Relay OFF" : "Compressor Relay ON";
    }
    else if (strstr(payload, "\"action\":\"toggle_beater\"") || strstr(payload, "\"action\":\"relay_beater\"")) {
        bool cur = (paddle_start != false);
        pin_write(Paddle, cur ? 1 : 0);
        paddle_start = !cur;
        msg = cur ? "Beater Mixer Relay OFF" : "Beater Mixer Relay ON";
    }
    else if (strstr(payload, "\"action\":\"toggle_air_pump\"") || strstr(payload, "\"action\":\"relay_air_pump\"")) {
        edge_toggle_produce();
        msg = "Air Injection Pump Toggled";
    }
    else if (strstr(payload, "\"action\":\"drain_toggle\"")) {
        if (s_drain_mode_active) {
            drain_mode_stop();
            msg = "Drain stopped";
        } else {
            drain_mode_start();
            msg = "Drain started (180s)";
        }
    }
    else if (strstr(payload, "\"action\":\"reset_counter\"") || strstr(payload, "\"action\":\"reset_daily_counter\"")) {
        icecream_counter_daily = 0;
        icecream_counter = 0;
        storage_set_i32(KEY_ICE_DAILY, 0);
        storage_set_i32(KEY_ICE_COUNTER, 0);
        app_state_save_to_nvs();
        ui_update_icecream_counter();
        buzzer_beep_confirm();
        msg = "Daily scoop counter reset";
    }
    else if (strstr(payload, "\"action\":\"buzzer_test\"")) {
        buzzer_beep_confirm();
        msg = "Buzzer beep tested";
    }
    else if (strstr(payload, "\"action\":\"daylock_unlock\"")) {
        char pin_str[32] = {0};
        const char *p = strstr(payload, "\"pin\":");
        if (!p) p = strstr(payload, "\"code\":");
        if (p) {
            p = strchr(p, ':');
            if (p) {
                p++;
                while (*p == ' ' || *p == '\"') p++;
                size_t idx = 0;
                while (*p && *p != '\"' && *p != ',' && *p != '}' && idx + 1 < sizeof(pin_str)) {
                    pin_str[idx++] = *p++;
                }
                pin_str[idx] = '\0';
            }
        }
        bool ok = day_lock_validate_pin(pin_str);
        success = ok;
        msg = ok ? "Lease installment unlocked successfully" : "Invalid daylock PIN";
    }
    else if (strstr(payload, "\"action\":\"daylock_extend\"")) {
        int days = 30;
        const char *p = strstr(payload, "\"days\":");
        if (p) days = atoi(p + 7);
        if (days > 0 && days <= 365) {
            day_lock_extend_days(days);
            msg = "Lease days extended";
        } else {
            success = false;
            msg = "Invalid days value";
        }
    }
    else if (strstr(payload, "\"action\":\"clear_faults\"") || strstr(payload, "\"action\":\"reset_fault\"")) {
        extern bool panel_nav_dismiss_active_modal(void);
        plc_request_control_reset();
        fault_log_clear();
        if (lvgl_lock(pdMS_TO_TICKS(100))) {
            panel_nav_dismiss_active_modal();
            lvgl_unlock();
        }
        buzzer_beep_confirm();
        msg = "Faults reset and log cleared";
    }
    else if (strstr(payload, "\"action\":\"ota_check\"")) {
        ota_request_scan();
        msg = "OTA scan requested";
    }
    else if (strstr(payload, "\"action\":\"ota_install\"") || strstr(payload, "\"action\":\"ota_update\"") || strstr(payload, "\"action\":\"firmware_update\"")) {
        char custom_url[256] = {0};
        const char *p_url = strstr(payload, "\"url\":");
        if (p_url) {
            p_url = strchr(p_url, ':');
            if (p_url) {
                p_url++; while (*p_url == ' ' || *p_url == '\"') p_url++;
                size_t idx = 0;
                while (*p_url && *p_url != '\"' && *p_url != ',' && *p_url != '}' && idx + 1 < sizeof(custom_url)) {
                    custom_url[idx++] = *p_url++;
                }
                custom_url[idx] = '\0';
            }
        }
        int model = 0;
        const char *p_mod = strstr(payload, "\"model\":");
        if (p_mod) model = atoi(p_mod + 8);

        /* قطع فوری و ایمن کلیه فرآیندهای تولید پیش از آغاز دانلود و فلش فریمور */
        if (make_start || paddle_start || ice_start || plc_is_dispense() || plc_is_ending() ||
            wash_is_active() || edge_is_active() || rainbow_is_active() || s_drain_mode_active) {
            plc_stop_all();
            buzzer_beep_alarm();
        }

        if (custom_url[0] != '\0') {
            ota_set_custom_url(custom_url);
            model = -1;
        }

        esp_err_t ota_res = ota_start_install(model, ota_cloud_progress_cb);
        if (ota_res == ESP_OK) {
            msg = "Remote OTA Download & Flash Started";
            success = true;
            buzzer_beep_confirm();
        } else {
            msg = "OTA already in progress or invalid URL";
            success = false;
        }
    }
    else if (strstr(payload, "\"action\":\"ota_cancel\"")) {
        ota_cancel_install();
        msg = "OTA Installation Canceled";
        buzzer_beep_confirm();
    }
    else if (strstr(payload, "\"action\":\"set_settings\"")) {
        const char *p;
        if ((p = strstr(payload, "\"pdl1\":"))) { paddle_timer_1 = atoi(p + 7); storage_set_i32(KEY_PDL_T1, paddle_timer_1); }
        if ((p = strstr(payload, "\"pdl2\":"))) { paddle_timer_2 = atoi(p + 7); storage_set_i32(KEY_PDL_T2, paddle_timer_2); }
        if ((p = strstr(payload, "\"start\":"))) { startup_timer = atoi(p + 8); storage_set_i32(KEY_START_T, startup_timer); }
        if ((p = strstr(payload, "\"eye\":"))) { mid_eye_timer = atoi(p + 6); storage_set_i32(KEY_EYE_T, mid_eye_timer); }
        if ((p = strstr(payload, "\"modbus\":"))) { modbus_delay = atoi(p + 9); storage_set_i32(KEY_MB_DLY, modbus_delay); }
        if ((p = strstr(payload, "\"rst1\":"))) { restart_time_1 = atoi(p + 7); storage_set_i32(KEY_RST_T1, restart_time_1); }
        if ((p = strstr(payload, "\"rst2\":"))) { restart_time_2 = atoi(p + 7); storage_set_i32(KEY_RST_T2, restart_time_2); }
        if ((p = strstr(payload, "\"ss_timeout\":"))) { screensaver_timeout_s = atoi(p + 13); storage_set_i32(KEY_SS_TIMEOUT, screensaver_timeout_s); }
        if ((p = strstr(payload, "\"price\":"))) { cone_price = (int32_t)atol(p + 8); storage_set_i32(KEY_CONE_PRICE, cone_price); }

        extern void nvs_request_lazy_flush(void);
        nvs_request_lazy_flush(); /* lazy: یک commit پس از ۳s به جای چند commit پشت هم */
        buzzer_beep_confirm();
        msg = "Parameters saved to NVS";
    }
    else if (strstr(payload, "\"action\":\"set_profile\"") || strstr(payload, "\"action\":\"set_identity\"")) {
        const char *p;
        if ((p = strstr(payload, "\"cust\":"))) {
            p = strchr(p, ':');
            if (p) {
                p++; while (*p == ' ' || *p == '\"') p++;
                size_t i = 0;
                while (*p && *p != '\"' && *p != ',' && *p != '}' && i + 1 < sizeof(s_customer_name)) {
                    s_customer_name[i++] = *p++;
                }
                s_customer_name[i] = '\0';
                storage_set_str("cloud_cust", s_customer_name);
            }
        }
        if ((p = strstr(payload, "\"city\":"))) {
            p = strchr(p, ':');
            if (p) {
                p++; while (*p == ' ' || *p == '\"') p++;
                size_t i = 0;
                while (*p && *p != '\"' && *p != ',' && *p != '}' && i + 1 < sizeof(s_city)) {
                    s_city[i++] = *p++;
                }
                s_city[i] = '\0';
                storage_set_str("cloud_city", s_city);
            }
        }
        if ((p = strstr(payload, "\"sn\":"))) {
            p = strchr(p, ':');
            if (p) {
                p++; while (*p == ' ' || *p == '\"') p++;
                size_t i = 0;
                while (*p && *p != '\"' && *p != ',' && *p != '}' && i + 1 < sizeof(s_serial_no)) {
                    s_serial_no[i++] = *p++;
                }
                s_serial_no[i] = '\0';
                storage_set_str("cloud_sn", s_serial_no);
            }
        }
        buzzer_beep_confirm();
        publish_online_state(true);
        msg = "Device Profile Updated in NVS";
    }
    else if (strstr(payload, "\"action\":\"get_telemetry\"")) {
        msg = "Telemetry refreshed";
    }
    else if (strstr(payload, "\"action\":\"ping\"")) {
        msg = "pong";
    }
    else if (strstr(payload, "\"action\":\"reboot\"")) {
        char pin_str[16] = {0};
        const char *p = strstr(payload, "\"pin\":");
        if (p) {
            p = strchr(p, ':');
            if (p) {
                p++; while (*p == ' ' || *p == '\"') p++;
                size_t idx = 0;
                while (*p && *p != '\"' && *p != ',' && *p != '}' && idx + 1 < sizeof(pin_str)) pin_str[idx++] = *p++;
            }
        }
        if (strcmp(pin_str, "1234") == 0 || strcmp(pin_str, "7476") == 0) {
            msg = "Reboot acknowledged";
            buzzer_beep_confirm();
        } else {
            success = false;
            msg = "Invalid admin PIN for reboot";
        }
    }
    else {
        success = false;
        msg = "Unknown action";
    }

    /* Send standard response packet back to cloud (both /ack and /cmd_resp) */
    if (s_mqtt_client && s_is_connected) {
        char resp[512];
        snprintf(resp, sizeof(resp),
                 "{\"req_id\":\"%s\",\"dev_id\":\"%s\",\"status\":\"%s\",\"ok\":%s,"
                 "\"executed_action\":\"%s\",\"code\":%d,\"message\":\"%s\"}",
                 req_id, s_device_id, success ? "SUCCESS" : "ERROR",
                 success ? "true" : "false", action_name, success ? 0 : -1, msg);
        esp_mqtt_client_publish(s_mqtt_client, s_topic_ack, resp, strlen(resp), 1, 0);
        esp_mqtt_client_publish(s_mqtt_client, s_topic_cmd_resp, resp, strlen(resp), 1, 0);
    }

    /* Trigger immediate telemetry update */
    cloud_client_publish_telemetry_now();
}

/* ---------------- Periodic Telemetry Task (Adaptive Rate) ---------------- */
static void cloud_worker_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        /* Adaptive rate: 1000ms if actively producing/dispensing/washing, 5000ms if idle */
        bool is_active_proc = (make_start || plc_is_dispense() || wash_is_active() || s_drain_mode_active);
        uint32_t wait_ms = is_active_proc ? 1000 : 5000;

        /* Wait for periodic interval or immediate trigger */
        if (xSemaphoreTake(s_trigger_sem, pdMS_TO_TICKS(wait_ms)) == pdTRUE) {
            /* Triggered immediately by state change or command */
        }

        extern volatile bool s_cloud_paused;
        if (s_cloud_paused) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (s_wifi_ready) {
            if (s_reconnect_needed) {
                s_reconnect_needed = false;
                ESP_LOGW(TAG, "Reconnecting MQTT client to %s...", s_broker_uri);
                stop_mqtt_client();
                vTaskDelay(pdMS_TO_TICKS(1000));
                start_mqtt_client();
            } else if (!s_mqtt_client) {
                start_mqtt_client();
            } else if (s_is_connected) {
                publish_telemetry();
            }
        }
    }
}

/* ---------------- Public Interface ---------------- */
void cloud_client_init(void)
{
    if (s_cloud_task_h != NULL) return;

    init_device_identity();

    s_trigger_sem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(cloud_worker_task, "cloud_task", 4096, NULL, 3, &s_cloud_task_h, 0);
    ESP_LOGI(TAG, "Cloud client initialized on Core 0");
}

void cloud_client_on_wifi_connected(void)
{
    s_wifi_ready = true;
    ESP_LOGI(TAG, "WiFi ready signal received -> triggering Cloud connect");
    if (s_trigger_sem) {
        xSemaphoreGive(s_trigger_sem);
    }
}

void cloud_client_on_wifi_disconnected(void)
{
    s_wifi_ready = false;
    s_is_connected = false;
    ESP_LOGW(TAG, "WiFi lost -> Cloud connection paused");
}

bool cloud_client_is_connected(void)
{
    return s_is_connected;
}

const char *cloud_client_get_device_id(void)
{
    return s_device_id;
}

const char *cloud_client_get_customer_name(void)
{
    return s_customer_name;
}

const char *cloud_client_get_city(void)
{
    return s_city;
}

const char *cloud_client_get_serial_no(void)
{
    return s_serial_no;
}

void cloud_client_set_profile(const char *cust, const char *city, const char *sn)
{
    if (cust && *cust) {
        strncpy(s_customer_name, cust, sizeof(s_customer_name) - 1);
        storage_set_str("cloud_cust", s_customer_name);
    }
    if (city && *city) {
        strncpy(s_city, city, sizeof(s_city) - 1);
        storage_set_str("cloud_city", s_city);
    }
    if (sn && *sn) {
        strncpy(s_serial_no, sn, sizeof(s_serial_no) - 1);
        storage_set_str("cloud_sn", s_serial_no);
    }
    if (s_mqtt_client && s_is_connected) {
        publish_online_state(true);
        cloud_client_publish_telemetry_now();
    }
}

void cloud_client_publish_telemetry_now(void)
{
    if (s_trigger_sem) {
        xSemaphoreGive(s_trigger_sem);
    }
}

void cloud_client_set_broker_uri(const char *broker_uri)
{
    if (!broker_uri || strlen(broker_uri) == 0) return;
    strncpy(s_broker_uri, broker_uri, sizeof(s_broker_uri) - 1);
    storage_set_str("cloud_broker", s_broker_uri);
    ESP_LOGI(TAG, "New broker URI saved: %s -> reconnecting...", s_broker_uri);

    stop_mqtt_client();
    if (s_wifi_ready) {
        start_mqtt_client();
    }
}

void cloud_client_stop(void)
{
    s_wifi_ready = false;
    stop_mqtt_client();
}

volatile bool s_cloud_paused = false;

void cloud_client_pause(void)
{
    s_cloud_paused = true;
    stop_mqtt_client();
    ESP_LOGI(TAG, "Cloud client paused for OTA (MQTT stopped)");
}

void cloud_client_resume(void)
{
    s_cloud_paused = false;
    if (s_trigger_sem) {
        xSemaphoreGive(s_trigger_sem);
    }
    ESP_LOGI(TAG, "Cloud client resumed");
}
