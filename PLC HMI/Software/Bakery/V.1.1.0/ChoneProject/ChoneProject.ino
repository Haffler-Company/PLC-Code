#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif
#include "lv_conf.h"

#include "defines.h"
#include <lvgl.h>
#include <esp_system.h>
#include <TFT_eSPI.h>
#include <bb_captouch.h>
#include "ui.h"

#include <Wire.h>
#include <RTClib.h>
#include "day_lock.h"
#include "lang.h"
#include "storage.h"
#include "general.h"
#include "screenshot.h"
#include "agent_hmi.h"
#include "lvgl_function.h"
#include "tasks.h"
#include "ui_notification.h"
#include "ota_manager.h"
#include "panel_nav.h"

String tempKeyboardText = "";

ScreenMap currentScreen = WARNING;

bool processActive = false;
bool isAutoScreenActive = false;

bool cycleStarted = false;

bool isFunneleLeft = false;
bool isFunneleRight = false;

bool isPistonUp = false;
bool isPistonDown = false;

bool shouldBeep;

TaskHandle_t lvglTaskHandle;
TaskHandle_t appTaskHandle;
TaskHandle_t keypadTaskHandle;
TaskHandle_t buzzerTaskHandle;
TaskHandle_t webTaskHandle;
SemaphoreHandle_t lv_mutex;
QueueHandle_t soundQueue = NULL;
ShiftManager shiftManager;
WebPortal webPortal;
OtaManager otaManager;
bool rtc_available = false;

Storage storage;
BBCapTouch bbct;
RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);

double counter = 1;
uint8_t lastS = 0;
unsigned long lastMiliSec = 0;
unsigned long lastMiliSecCycle = 0;
String days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
ButtonState fPanel, mPanel;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10];

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    shot_on_flush(area, (const uint16_t *)&color_p->full, lv_disp_flush_is_last(disp));

    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (g_agent_touch_active)
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = g_agent_touch_x;
        data->point.y = g_agent_touch_y;
        panel_nav_notify_touch();
        if (millis() >= g_agent_touch_release_ms)
        {
            g_agent_touch_active = false;
        }
        return;
    }

    TOUCHINFO ti;

    if (!bbct.getSamples(&ti))
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = ti.x[0];
        data->point.y = ti.y[0];
        panel_nav_notify_touch();
    }
}

extern "C" void sound_play_c(uint8_t sound_type)
{
    sound_play((SoundType)sound_type);
}

extern "C" bool lv_lock_c(uint32_t timeout_ms)
{
    if (lv_mutex == NULL) return true;
    return (xSemaphoreTakeRecursive(lv_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
}

extern "C" void lv_unlock_c(void)
{
    if (lv_mutex != NULL)
    {
        xSemaphoreGiveRecursive(lv_mutex);
    }
}

void setup()
{
    //? 1. CRITICAL: Early latch MUST run before any peripheral / screen setup
    ui_resume_boot_latch_early();

    setupPin();
    checkBoot();

    Serial.begin(921600);
    shot_init_worker();
    agent_hmi_init();

    //? 2. Initialize global LVGL recursive mutex for dual-core thread safety
    lv_mutex = xSemaphoreCreateRecursiveMutex();

    storage.begin();
    day_lock_init();
    g_lang = (Lang)storage.getLanguage();
    sound_init();
    shiftManager.begin();
    if (storage.getWifiEnabled())
    {
        webPortal.begin();
    }
    otaManager.init();

    Wire.begin();
    delay(50);

    //? 3. Non-blocking RTC init: Do NOT brick or hang system if battery/RTC fails
    if (!rtc.begin())
    {
        Serial.println("WARN: RTC DS3231 not found - running with soft-clock fallback");
        rtc_available = false;
    }
    else
    {
        rtc_available = true;
        if (rtc.lostPower())
        {
            Serial.println("RTC lost power, setting compile time!");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }

    lv_init();
    tft.begin();

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
    bbct.setOrientation(TOUCH_ORIENTATION, SCREEN_HEIGHT, SCREEN_WIDTH);

    tft.setRotation(SCREEN_ROTATION);

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    indev_drv.long_press_time = 350;
    indev_drv.long_press_repeat_time = 80;
    lv_indev_drv_register(&indev_drv);

    //? 4. Initialize UI (Bypasses Splash/Warning if resuming from crash)
    custom_ui_init();
    if (ui_WarningPage_Button1 != NULL)
    {
        warning_screen_events_init();
    }
    ui_notification_init();
    panel_nav_init();

    if (!rtc_available)
    {
        ui_show_notification(NOTIF_WARNING, "ساعت سخت افزاری DS3231", "ساعت پشتیبان متصل نیست - ساعت موقت فعال شد");
    }
    else if (ui_resume_boot_get_latched(NULL))
    {
        ui_show_notification(NOTIF_INFO, "بازیابی اضطراری سیستم", "سیستم بازیابی شد - خروجی ها در حالت ایمن خاموش هستند");
    }
    else
    {
        ui_show_notification(NOTIF_SUCCESS, "سیستم آماده است", "کنترلر صنعتی و سنسورها در وضعیت نرمال قرار دارند");
    }

    //? 5. Right-sized FreeRTOS Tasks (Stacks in 4-byte words: 2048 words = 8 KB)
    xTaskCreatePinnedToCore(lv_tick_task, "LVGL Tick", 2048, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(lvgl_task, "LVGL", 4096, NULL, 2, &lvglTaskHandle, 1);
    xTaskCreatePinnedToCore(app_task, "App", 4096, NULL, 1, &appTaskHandle, 0);
    xTaskCreatePinnedToCore(keypad_task, "Keypad", 2048, NULL, 1, &keypadTaskHandle, 0);
    xTaskCreatePinnedToCore(buzzer_task, "Buzzer", 1024, NULL, 1, &buzzerTaskHandle, 0);
    xTaskCreatePinnedToCore(web_task, "WebPortal", 3072, NULL, 1, &webTaskHandle, 0);

    Serial.println("Setup done");
}

void loop() { vTaskDelete(NULL); }
