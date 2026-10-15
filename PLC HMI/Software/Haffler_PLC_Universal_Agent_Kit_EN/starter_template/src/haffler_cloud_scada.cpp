/**
 * @file haffler_cloud_scada.cpp
 * @brief Haffler IIoT Cloud SCADA & WebSerial Bridge Implementation
 * @version 1.0.0
 */

#include "haffler_cloud_scada.h"
#include <WiFi.h>
#include <mqtt_client.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static const char *TAG = "HAFFLER_CLOUD";

// Configuration Defaults
#define MQTT_BROKER_URI     "mqtt://haffler-ai.ir:1883"
#define TELEMETRY_PERIOD_MS 1000

// Internal State
static char s_slug[32] = "weld_single";
static char s_dev_id[32] = "WELD-01";
static haffler_cmd_callback_t s_cmd_callback = NULL;
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;

static QueueHandle_t s_telemetry_queue = NULL;
static haffler_telemetry_data_t s_last_telemetry;

// Topic Buffers
static char s_topic_telemetry[96];
static char s_topic_cmd[96];
static char s_topic_ack[96];
static char s_topic_online[96];

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✓ Connected to Haffler Cloud MQTT Broker");
            s_mqtt_connected = true;
            // Subscribe to remote command topic
            esp_mqtt_client_subscribe(s_mqtt_client, s_topic_cmd, 1);
            // Publish online status
            esp_mqtt_client_publish(s_mqtt_client, s_topic_online, "online", 6, 1, 1);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from Haffler Cloud MQTT Broker, retrying...");
            s_mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA: {
            char topic_buf[128] = {0};
            char data_buf[256] = {0};
            int tlen = (event->topic_len < (int)sizeof(topic_buf) - 1) ? event->topic_len : (int)sizeof(topic_buf) - 1;
            int dlen = (event->data_len < (int)sizeof(data_buf) - 1) ? event->data_len : (int)sizeof(data_buf) - 1;
            memcpy(topic_buf, event->topic, tlen);
            memcpy(data_buf, event->data, dlen);

            ESP_LOGI(TAG, "Remote Command Received: [%s] -> %s", topic_buf, data_buf);

            // Execute Callback
            if (s_cmd_callback) {
                // If data is JSON, extract action, or execute directly
                s_cmd_callback(data_buf, data_buf);
            }
            break;
        }

        default:
            break;
    }
}

static void task_cloud_scada_worker(void *param) {
    (void)param;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(TELEMETRY_PERIOD_MS);

    while (1) {
        // 1. Drain fresh telemetry from queue
        haffler_telemetry_data_t fresh;
        if (xQueueReceive(s_telemetry_queue, &fresh, 0) == pdPASS) {
            s_last_telemetry = fresh;
        }

        // 2. Stream to WebSerial / USB CDC
        char serial_buf[256];
        snprintf(serial_buf, sizeof(serial_buf),
            "STAT:slug=%s,dev=%s,state=%s,val=%.1f,tgt=%.1f,cyc=%lu,relays=0x%03X,in=0x%04X,err=%d",
            s_slug,
            s_dev_id,
            s_last_telemetry.state_name ? s_last_telemetry.state_name : "IDLE",
            s_last_telemetry.hero_val,
            s_last_telemetry.target_val,
            (unsigned long)s_last_telemetry.cycle_count,
            s_last_telemetry.active_relays,
            s_last_telemetry.active_inputs,
            s_last_telemetry.error_flag ? 1 : 0
        );
        Serial.println(serial_buf);

        // 3. Publish to Cloud MQTT if connected
        if (s_mqtt_connected && s_mqtt_client) {
            char json_payload[384];
            snprintf(json_payload, sizeof(json_payload),
                "{\"machine_id\":\"%s\",\"device_id\":\"%s\",\"status\":\"%s\",\"hero_val\":%.2f,\"target\":%.2f,\"cycles\":%lu,\"relays\":%u,\"inputs\":%u,\"error\":%s,\"ts\":%lu}",
                s_slug,
                s_dev_id,
                s_last_telemetry.state_name ? s_last_telemetry.state_name : "IDLE",
                s_last_telemetry.hero_val,
                s_last_telemetry.target_val,
                (unsigned long)s_last_telemetry.cycle_count,
                s_last_telemetry.active_relays,
                s_last_telemetry.active_inputs,
                s_last_telemetry.error_flag ? "true" : "false",
                (unsigned long)millis()
            );
            esp_mqtt_client_publish(s_mqtt_client, s_topic_telemetry, json_payload, 0, 0, 0);
        }

        // 4. Handle incoming WebSerial from USB
        if (Serial.available()) {
            String incoming = Serial.readStringUntil('\n');
            incoming.trim();
            if (incoming.length() > 0) {
                haffler_cloud_scada_handle_serial_input(incoming.c_str());
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void haffler_cloud_scada_init(const char *machine_slug, const char *device_id, haffler_cmd_callback_t cmd_cb) {
    if (machine_slug) strncpy(s_slug, machine_slug, sizeof(s_slug) - 1);
    if (device_id) strncpy(s_dev_id, device_id, sizeof(s_dev_id) - 1);
    s_cmd_callback = cmd_cb;

    // Build standard topics
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "haffler/%s/%s/telemetry", s_slug, s_dev_id);
    snprintf(s_topic_cmd, sizeof(s_topic_cmd), "haffler/%s/%s/cmd", s_slug, s_dev_id);
    snprintf(s_topic_ack, sizeof(s_topic_ack), "haffler/%s/%s/ack", s_slug, s_dev_id);
    snprintf(s_topic_online, sizeof(s_topic_online), "haffler/%s/%s/online", s_slug, s_dev_id);

    // Create telemetry queue (single item overwrite)
    if (!s_telemetry_queue) {
        s_telemetry_queue = xQueueCreate(1, sizeof(haffler_telemetry_data_t));
    }

    // Configure ESP-IDF MQTT Client
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;
    mqtt_cfg.session.last_will.topic = s_topic_online;
    mqtt_cfg.session.last_will.msg = "offline";
    mqtt_cfg.session.last_will.msg_len = 7;
    mqtt_cfg.session.last_will.qos = 1;
    mqtt_cfg.session.last_will.retain = 1;
    mqtt_cfg.network.reconnect_timeout_ms = 5000;

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client) {
        esp_mqtt_client_register_event(s_mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(s_mqtt_client);
    }

    // Launch worker task on CORE 0 with 4096 words (16KB) stack
    xTaskCreatePinnedToCore(
        task_cloud_scada_worker,
        "CloudSCADA_Task",
        4096,
        NULL,
        3,
        NULL,
        0 // Strictly Core 0
    );

    ESP_LOGI(TAG, "Haffler Cloud SCADA initialized for %s (%s)", s_slug, s_dev_id);
}

void haffler_cloud_scada_update_telemetry(const haffler_telemetry_data_t *data) {
    if (!data || !s_telemetry_queue) return;
    xQueueOverwrite(s_telemetry_queue, data);
}

void haffler_cloud_scada_send_ack(const char *action, const char *status) {
    if (!s_mqtt_connected || !s_mqtt_client || !action) return;
    char ack_buf[128];
    snprintf(ack_buf, sizeof(ack_buf), "{\"action\":\"%s\",\"status\":\"%s\",\"ts\":%lu}", action, status ? status : "ok", (unsigned long)millis());
    esp_mqtt_client_publish(s_mqtt_client, s_topic_ack, ack_buf, 0, 1, 0);
}

bool haffler_cloud_scada_is_connected(void) {
    return s_mqtt_connected;
}

void haffler_cloud_scada_handle_serial_input(const char *line) {
    if (!line) return;
    if (strcmp(line, "PING") == 0) {
        Serial.println("PONG");
    } else if (strcmp(line, "STATUS") == 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "STAT:slug=%s,dev=%s,state=%s,val=%.1f,relays=0x%03X,in=0x%04X",
            s_slug, s_dev_id, s_last_telemetry.state_name ? s_last_telemetry.state_name : "IDLE",
            s_last_telemetry.hero_val, s_last_telemetry.active_relays, s_last_telemetry.active_inputs);
        Serial.println(buf);
    } else if (strcmp(line, "RESET") == 0 || strcmp(line, "ESP_RESET") == 0) {
        Serial.println("[RESET] Rebooting ESP32-S3...");
        delay(100);
        esp_restart();
    } else {
        if (s_cmd_callback) {
            s_cmd_callback(line, line);
        }
        Serial.printf("[ACK] %s_RECEIVED\n", line);
    }
}
