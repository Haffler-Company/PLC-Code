#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif
#include "lv_conf.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <bb_captouch.h>
#include <Wire.h>
#include "ui.h"
#include "actions.h"
#include "variables.h"
#include "a2_eeprom.h"
#include "agent_hmi.h"
#include "ui_vazir_fonts.h"
#include "ota_manager.h"
#include "day_lock.h"
#include "webportal.h"
#include "panel_keys.h"
#include "panel_nav.h"
#include "warranty_mgr.h"
#include "ui_toast.h"
#include "ui_resume.h"
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include <esp_arduino_version.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  #define BUZZER_LEDC_ATTACH()   ledcAttach(45, 7000, 8)
  #define BUZZER_LEDC_WRITE(val) ledcWrite(45, (val))
#else
  #define BUZZER_LEDC_ATTACH()   do { ledcSetup(0, 7000, 8); ledcAttachPin(45, 0); } while(0)
  #define BUZZER_LEDC_WRITE(val) ledcWrite(0, (val))
#endif

// #include "EEPROM.h"

#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TOUCH_INT 40
#define TOUCH_RST 39
static const uint16_t SCREEN_WIDTH = 480;
static const uint16_t SCREEN_HEIGHT = 320;
#define DISP_BUF_LINES 64
unsigned long lastTick = 0;
BBCapTouch bbct;
TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH,SCREEN_HEIGHT);
static uint16_t buf[SCREEN_WIDTH * DISP_BUF_LINES] __attribute__((aligned(4))); /* 64 lines (5 clean slices), 32-bit aligned for maximum SPI throughput */
static volatile bool g_disp_flush_ready = false;

#define I2C_TOUCH_ADDR  0x5D
bool i2c_touch_ok = false;
volatile int16_t g_touch_last_x = -1;
volatile int16_t g_touch_last_y = -1;
volatile uint8_t g_touch_last_count = 0;
volatile int g_touch_sensor = -1;
volatile uint32_t g_touch_ok_count = 0;
static uint8_t g_gt911_addr = 0x5D;
static uint8_t s_gt911_err_count = 0;

/* Industrial 9-Clock I2C bus unlock pattern to recover hung slave SDA lines */
static void i2c_bus_recover(void)
{
    Wire.end();
    pinMode(TOUCH_SCL, OUTPUT_OPEN_DRAIN);
    pinMode(TOUCH_SDA, INPUT_PULLUP);
    digitalWrite(TOUCH_SCL, HIGH);
    delayMicroseconds(5);
    for (int i = 0; i < 9; i++) {
        digitalWrite(TOUCH_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(TOUCH_SCL, HIGH);
        delayMicroseconds(5);
    }
    pinMode(TOUCH_SDA, OUTPUT_OPEN_DRAIN);
    digitalWrite(TOUCH_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(TOUCH_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(TOUCH_SDA, HIGH);
    delayMicroseconds(5);

    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(400000);
    Wire.setTimeOut(8);
}

/* GT911 must be read before its status register is acknowledged.  The generic
 * library clears the status first, which can discard the active touch frame. */
static bool gt911_read_reg(uint16_t reg, uint8_t *data, uint8_t length)
{
    Wire.beginTransmission(g_gt911_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)reg);
    if (Wire.endTransmission(false) != 0) {
        if (++s_gt911_err_count >= 5) {
            i2c_bus_recover();
            s_gt911_err_count = 0;
        }
        return false;
    }
    if (Wire.requestFrom((int)g_gt911_addr, (int)length) != length) {
        if (++s_gt911_err_count >= 5) {
            i2c_bus_recover();
            s_gt911_err_count = 0;
        }
        return false;
    }
    for (uint8_t i = 0; i < length; i++) data[i] = Wire.read();
    s_gt911_err_count = 0;
    return true;
}

static void gt911_ack_touch(void)
{
    Wire.beginTransmission(g_gt911_addr);
    Wire.write((uint8_t)0x81);
    Wire.write((uint8_t)0x4E);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission();
}

bool gt911_read_touch(int16_t *x, int16_t *y, uint8_t *count)
{
    uint8_t data[7];
    if (!gt911_read_reg(0x814E, data, sizeof(data))) return false;
    uint8_t status = data[0];
    if ((status & 0x80) == 0) return false;

    /* A ready frame with zero points is the GT911 release event. Always ACK immediately */
    uint8_t points = (uint8_t)(status & 0x0F);
    gt911_ack_touch();
    if (points == 0 || points > 5) {
        return false;
    }
    int16_t raw_x = (int16_t)(data[2] | ((uint16_t)data[3] << 8));
    int16_t raw_y = (int16_t)(data[4] | ((uint16_t)data[5] << 8));
    /* GT911 native portrait range is X=0..319, Y=0..479. Rotation 270 maps native Y onto the 480-pixel landscape X axis. */
    *x = (int16_t)(SCREEN_WIDTH - 1 - raw_y);
    *y = raw_x;
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
    if (*x >= (int16_t)SCREEN_WIDTH) *x = (int16_t)SCREEN_WIDTH - 1;
    if (*y >= (int16_t)SCREEN_HEIGHT) *y = (int16_t)SCREEN_HEIGHT - 1;
    *count = points;
    return true;
}

void gt911_debug_watch(Stream *port)
{
    uint8_t product[4] = {0};
    uint8_t config[5] = {0};
    uint8_t status = 0;
    uint8_t last_status = 0xff;
    uint32_t ready_frames = 0;
    uint32_t touch_frames = 0;
    uint32_t int_low = 0;
    int16_t last_x = -1;
    int16_t last_y = -1;

    bool product_ok = gt911_read_reg(0x8140, product, sizeof(product));
    bool config_ok = gt911_read_reg(0x8047, config, sizeof(config));
    port->printf("TOUCHRAW_BEGIN addr=0x%02X product_ok=%u id=%c%c%c%c config_ok=%u cfg=%u xres=%u yres=%u int=%d\n",
                 g_gt911_addr, (unsigned)product_ok,
                 product[0], product[1], product[2], product[3],
                 (unsigned)config_ok, config[0],
                 (unsigned)(config[1] | ((uint16_t)config[2] << 8)),
                 (unsigned)(config[3] | ((uint16_t)config[4] << 8)),
                 digitalRead(TOUCH_INT));

    uint32_t until = millis() + 8000;
    while ((int32_t)(millis() - until) < 0) {
        if (digitalRead(TOUCH_INT) == LOW) int_low++;
        if (gt911_read_reg(0x814E, &status, 1)) {
            if (status != last_status) {
                port->printf("TOUCHRAW_STATUS value=0x%02X int=%d\n", status, digitalRead(TOUCH_INT));
                last_status = status;
            }
            if (status & 0x80) {
                ready_frames++;
                uint8_t points = (uint8_t)(status & 0x0f);
                if (points > 0 && points <= 5) {
                    uint8_t point[8];
                    if (gt911_read_reg(0x814F, point, sizeof(point))) {
                        last_x = (int16_t)(point[1] | ((uint16_t)point[2] << 8));
                        last_y = (int16_t)(point[3] | ((uint16_t)point[4] << 8));
                        touch_frames++;
                    }
                }
                gt911_ack_touch();
            }
        }
        delay(10);
    }
    port->printf("TOUCHRAW_END ready=%u touches=%u raw=%d,%d int_low=%u last=0x%02X\n",
                 (unsigned)ready_frames, (unsigned)touch_frames,
                 (int)last_x, (int)last_y, (unsigned)int_low, last_status);
}

static uint32_t my_tick(void)
{
    return millis();
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (!g_disp_flush_ready || !area || !px_map) {
        lv_display_flush_ready(disp);
        return;
    }

    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= (int32_t)SCREEN_WIDTH) x2 = (int32_t)SCREEN_WIDTH - 1;
    if (y2 >= (int32_t)SCREEN_HEIGHT) y2 = (int32_t)SCREEN_HEIGHT - 1;
    if (x2 < x1 || y2 < y1) {
        lv_display_flush_ready(disp);
        return;
    }

    uint32_t w = (uint32_t)(x2 - x1 + 1);
    uint32_t h = (uint32_t)(y2 - y1 + 1);
    bool last_flush = lv_display_flush_is_last(disp);

    perf_overlay_note_flush(last_flush, w * h);
    agent_on_flush(area, (uint16_t *)px_map, last_flush);
    tft.startWrite();
    tft.setAddrWindow((int32_t)x1, (int32_t)y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}


void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    TOUCHINFO ti;

    if (agent_read_touch(data)) {
        return;
    }

    if (g_touch_sensor == CT_TYPE_GT911) {
        int16_t x, y;
        uint8_t count;
        if (!gt911_read_touch(&x, &y, &count)) {
            data->state = LV_INDEV_STATE_REL;
            data->point.x = g_touch_last_x >= 0 ? g_touch_last_x : 0;
            data->point.y = g_touch_last_y >= 0 ? g_touch_last_y : 0;
            g_touch_last_count = 0;
            return;
        }
        /* Zero Lag: Direct 1:1 reporting with no EMA smoothing delay for smartphone-like fluidity */
        g_touch_last_x = x;
        g_touch_last_y = y;
        g_touch_last_count = count;
        g_touch_ok_count++;
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
        panel_nav_notify_touch();
        return;
    }

    if (!bbct.getSamples(&ti) || ti.count < 1)
    {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    int16_t x = (int16_t)ti.x[0];
    int16_t y = (int16_t)ti.y[0];
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int16_t)SCREEN_WIDTH) x = (int16_t)SCREEN_WIDTH - 1;
    if (y >= (int16_t)SCREEN_HEIGHT) y = (int16_t)SCREEN_HEIGHT - 1;

    g_touch_last_x = x;
    g_touch_last_y = y;
    g_touch_last_count = (uint8_t)ti.count;
    g_touch_ok_count++;

    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
    panel_nav_notify_touch();
}
/*****************Timer*******************/
hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

volatile bool g_hw_override_valve1 = false;
volatile bool g_hw_override_valve2 = false;
volatile bool g_hw_override_buzzer = false;

void ARDUINO_ISR_ATTR onTimer() {
  if (TimerLogo < 25) {
    TimerLogo++;
  } else {
    ui_logo_hide_req = 1;
  }

  if (stop_input_is_active()) {
    TimerValve1 = 0;
    TimerValve2 = 0;
    g_hw_override_valve1 = false;
    g_hw_override_valve2 = false;
    digitalWrite(16, !vr_OutPutType);
    digitalWrite(42, !vr_OutPutType);
    buzzerTimeVlaue = 0;
    BUZZER_LEDC_WRITE(0);
    return;
  }

  if (TimerValve1 != 0) {
    if (TimerValve1 > 1800) TimerValve1 = 1800; /* Runaway watchdog: 180s max */
    digitalWrite(16, vr_OutPutType);
    TimerValve1--;
    if (TimerValve1 == 0) ui_complete1_pending = 1;
    if (TimerValve1_Saved != 0) {
      uint32_t pct = (TimerValve1 * 100u) / TimerValve1_Saved;
      ui_bar1_value = (uint8_t)(100u - pct);
      ui_bar1_pending = 1;
    }
  } else if (!g_hw_override_valve1) {
    digitalWrite(16, !vr_OutPutType);
  }

  if (TimerValve2 != 0) {
    if (TimerValve2 > 1800) TimerValve2 = 1800; /* Runaway watchdog: 180s max */
    digitalWrite(42, vr_OutPutType);
    TimerValve2--;
    if (TimerValve2 == 0) ui_complete2_pending = 1;
    if (TimerValve2_Saved != 0) {
      uint32_t pct = (TimerValve2 * 100u) / TimerValve2_Saved;
      ui_bar2_value = (uint8_t)(100u - pct);
      ui_bar2_pending = 1;
    }
  } else if (!g_hw_override_valve2) {
    digitalWrite(42, !vr_OutPutType);
  }

  if (buzzerTimeVlaue && buzzerEnable && EEprom3_Buzzer) {
    buzzerTimeVlaue--;
    BUZZER_LEDC_WRITE(200);
  } else if (!g_hw_override_buzzer) {
    buzzerTimeVlaue = 0;
    BUZZER_LEDC_WRITE(0);
  }
}
/*****************************************/

static void emergency_valve_shutdown(void)
{
    g_hw_override_valve1 = false;
    g_hw_override_valve2 = false;
    g_hw_override_buzzer = false;
    digitalWrite(16, !vr_OutPutType);
    digitalWrite(42, !vr_OutPutType);
    BUZZER_LEDC_WRITE(0);
}

static void nvs_purge_foreign_namespaces_at_boot(void)
{
    const char *foreign[] = {"AJOSH1", "app", "ice_cream", "welding", "cutter", "chone", "haffler_rcp", "bakery_cfg", "haf_lock", "wipe_ctl"};
    int purged = 0;
    for (int i = 0; i < 10; i++) {
        nvs_handle_t h;
        if (nvs_open(foreign[i], NVS_READWRITE, &h) == ESP_OK) {
            nvs_erase_all(h);
            nvs_commit(h);
            nvs_close(h);
            purged++;
            Serial.printf("[NVS] Boot purged foreign namespace '%s'\n", foreign[i]);
        }
    }
    if (purged > 0) {
        Serial.printf("[NVS] Freed %d foreign namespaces\n", purged);
    }
}

void setup() {
    /* Zero-Glitch Boot Failsafe: Clamping relay valve outputs (Active-LOW: HIGH=OFF) and buzzer */
    digitalWrite(16, HIGH);
    digitalWrite(42, HIGH);
    pinMode(16, INPUT_PULLUP);
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);
    pinMode(42, INPUT_PULLUP);
    pinMode(42, OUTPUT);
    digitalWrite(42, HIGH);
    pinMode(45, OUTPUT);
    digitalWrite(45, LOW);

    /* Latch crash/hang reset reason and target screen immediately before anything else */
    ui_resume_boot_latch_early();
    /* Serial first so boot breadcrumbs survive later hangs. */
    agent_begin();
    nvs_purge_foreign_namespaces_at_boot();
    Serial.println("SETUP 1 wifi_off");
    Serial0.println("SETUP 1 wifi_off");
    WiFi.mode(WIFI_OFF);
    setenv("TZ", "IRST-3:30", 1);
    tzset();

    /* Initialize/reset the capacitive controller before display/UI work.
       This is the project's previously verified touch-safe boot order. */
    Serial.println("SETUP 2 touch_early");
    Serial0.println("SETUP 2 touch_early");
    int touch_init_rc = bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
    Wire.setClock(400000); /* 400kHz Fast Mode for GT911 touch bus */
    Wire.setTimeOut(8);    /* 8ms max timeout to prevent bus lockup */
    bbct.setOrientation(270, SCREEN_HEIGHT, SCREEN_WIDTH);
    g_touch_sensor = bbct.sensorType();
    Wire.beginTransmission(g_gt911_addr);
    if (Wire.endTransmission() != 0) g_gt911_addr = 0x14;
    Wire.beginTransmission(g_gt911_addr);
    i2c_touch_ok = (touch_init_rc == 0 && g_touch_sensor > 0 && Wire.endTransmission() == 0);
    Serial.printf("TOUCH_INIT rc=%d sensor=%d addr=0x%02X ok=%d\n",
                  touch_init_rc, g_touch_sensor, g_gt911_addr, (int)i2c_touch_ok);
    Serial0.printf("TOUCH_INIT rc=%d sensor=%d addr=0x%02X ok=%d\n",
                   touch_init_rc, g_touch_sensor, g_gt911_addr, (int)i2c_touch_ok);

    Serial.println("SETUP 3 lvgl_tft");
    Serial0.println("SETUP 3 lvgl_tft");
    lv_init();
    lv_tick_set_cb(my_tick);
    pinMode(45, OUTPUT);
    BUZZER_LEDC_ATTACH();
    tft.begin();
    tft.setSwapBytes(true);
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    Serial.println("SETUP 3 display");
    Serial0.println("SETUP 3 display");
    Serial.flush();
    Serial0.flush();
    lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    Serial.println("SETUP 3a created");
    Serial.flush();
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    Serial.println("SETUP 3a1 format");
    Serial.flush();
    /* Register buffers BEFORE flush_cb so LVGL cannot call into TFT mid-init. */
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.println("SETUP 3b buffers");
    Serial.flush();
    g_disp_flush_ready = true;
    lv_display_set_flush_cb(disp, my_disp_flush);
    Serial.println("SETUP 3c flush_cb");
    Serial.flush();

    Serial.println("SETUP 4 ui");
    Serial0.println("SETUP 4 ui");
    Serial.flush();
    Serial0.flush();
    ui_vazir_fonts_init(); /* TinyTTF disabled — was stalling first refresh */
    Serial.println("SETUP 4a fonts_ok");
    Serial.flush();
    ui_init();
    Serial.println("SETUP 4b ui_init_ok");
    Serial0.println("SETUP 4b ui_init_ok");
    Serial.flush();
    Serial0.flush();
    ui_clock_init();
    HideLogo();
    TimerLogo = 25;
    lv_obj_t *act_scr = lv_screen_active();
    if (act_scr) {
        lv_obj_invalidate(act_scr);
    }
    lv_refr_now(disp);
    Serial.println("SETUP 5 ui_painted");
    Serial0.println("SETUP 5 ui_painted");
    Serial.flush();
    Serial0.flush();

    /* Touch/indev AFTER first paint so I2C cannot block a half-drawn frame. */
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_indev_set_scroll_limit(indev, 4);
    lv_indev_set_scroll_throw(indev, 20);
    lv_timer_t *indev_timer = lv_indev_get_read_timer(indev);
    if (indev_timer) {
        lv_timer_set_period(indev_timer, 15); /* 66 Hz touch polling for smartphone-like fluidity */
    }
    Serial.println("SETUP 5b indev");
    Serial0.println("SETUP 5b indev");
    Serial.flush();
    Serial0.flush();
    a2_eeprom_init();
    a2_eeprom_read_all();
    a2_eeprom_load_to_ui();
    ChangeBar(0);
    ChangeBar1(0);
    vr_OutPutType = (char)EEprom7_OutputType;
    ChangeBuzzerAvailble(EEprom3_Buzzer);
    ChangeTouchFeedback(EEprom4_TouchAvailable);
    ChangeLanguage(EEprom2_Language);
    /* Deterministic Siemens Pin Setup: Pre-load inactive state before driving pins */
    digitalWrite(16, !vr_OutPutType);
    digitalWrite(42, !vr_OutPutType);
    pinMode(16, OUTPUT);
    pinMode(42, OUTPUT);
    pinMode(3, INPUT_PULLUP);
    pinMode(7, INPUT_PULLUP);
    warranty_mgr_init();
    esp_register_shutdown_handler(emergency_valve_shutdown);
    Set_SWTypeOutPut(vr_OutPutType);
    day_lock_init();
    webportal_init();
    ota_manager_init();
    ota_manager_auto_connect();
    panel_keys_init();
    panel_nav_init();
    ui_resume_apply_if_pending();

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    timer = timerBegin(1000000);
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, 100000, true, 0);
#else
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 100000, true);
    timerAlarmEnable(timer);
#endif

    Serial.printf("SETUP OK sensor=%d addr=0x%02X ok=%d\n",
                  g_touch_sensor, g_gt911_addr, (int)i2c_touch_ok);
    Serial0.printf("SETUP OK sensor=%d addr=0x%02X ok=%d\n",
                   g_touch_sensor, g_gt911_addr, (int)i2c_touch_ok);
    Serial0.flush();
}

void loop() { 
  static bool s_first_loop = true;
  if (s_first_loop) {
    s_first_loop = false;
    Serial0.println("LOOP_STARTED_RUNNING");
    Serial0.flush();
  }
  uint32_t loop_start_us = micros();
  agent_poll();
  agent_before_lvgl();
  ui_process_isr_requests();
  panel_keys_poll();
  panel_nav_poll_tick();
  ui_toast_poll();
  ota_manager_poll();
  ui_check_and_show_update_prompt();
  day_lock_tick_1s();
  webportal_poll();
  ui_resume_poll();
  uint32_t next_run = lv_timer_handler();
  ui_tick();
  agent_after_lvgl();
  perf_overlay_add_busy(micros() - loop_start_us);
  perf_qa_tick();
  yield();
}

void ApplyOutputTypeSafe(char type)
{
  g_hw_override_valve1 = false;
  g_hw_override_valve2 = false;
  g_hw_override_buzzer = false;
  vr_OutPutType = type ? 1 : 0;
  TimerValve1 = 0;
  TimerValve2 = 0;
  digitalWrite(16, !vr_OutPutType);
  digitalWrite(42, !vr_OutPutType);
  ChangeBar(0);
  ChangeBar1(0);
  Set_SWTypeOutPut(vr_OutPutType);
}

void HwSetValve1(bool active)
{
  if (active && stop_input_is_active()) {
    active = false;
  }
  g_hw_override_valve1 = active;
  digitalWrite(16, active ? vr_OutPutType : !vr_OutPutType);
}

void HwSetValve2(bool active)
{
  if (active && stop_input_is_active()) {
    active = false;
  }
  g_hw_override_valve2 = active;
  digitalWrite(42, active ? vr_OutPutType : !vr_OutPutType);
}

void HwSetBuzzer(bool active)
{
  g_hw_override_buzzer = active;
  if (active) {
    BUZZER_LEDC_WRITE(200);
  } else {
    buzzerTimeVlaue = 0;
    BUZZER_LEDC_WRITE(0);
  }
}
