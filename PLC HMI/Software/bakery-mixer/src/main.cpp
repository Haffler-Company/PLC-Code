/**
 * @file main.cpp
 * @brief Haffler Industrial Commercial Bakery Spiral Dough Mixer Firmware
 * @version 1.0.0
 * 
 * Hardware: HAFFLER ETV3A (ESP32-S3 WROOM-1 N16R8, 480x320 IPS Screen, GT911, DS3231 RTC, MAX6675)
 * Architecture: Deterministic Siemens 50ms State Machine (Core 1) + Apple HIG HMI (Core 0)
 */

#include <Arduino.h>
#include "haffler_pins.h"
#include "splash_screen.h"
#include "machine_fsm.h"
#include "haffler_recipes.h"
#include "haffler_ui_builder.h"
#include "haffler_ui_lang.h"
#include "haffler_day_lock.h"
#include "haffler_web_portal.h"
#include <esp_task_wdt.h>
#include <driver/gpio.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <bb_captouch.h>
#include <lvgl.h>
#include "screenshot.h"
#include "agent_hmi.h"
#include "haffler_touchless_nav.h"

#define SCREEN_WIDTH        480
#define SCREEN_HEIGHT       320
#define SCREEN_ROTATION     3

#define TOUCH_SDA           8
#define TOUCH_SCL           9
#define TOUCH_INT           40
#define TOUCH_RST           -1
#define TOUCH_ORIENTATION   270

#define TFT_BL              38

static TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);
static BBCapTouch bbct;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10];

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    shot_on_flush(area, (const uint16_t *)&color_p->full, lv_disp_flush_is_last(disp));

    lv_disp_flush_ready(disp);
}

static void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    int16_t vx = 0, vy = 0;
    if (agent_hmi_get_touch(&vx, &vy)) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = vx;
        data->point.y = vy;
        return;
    }

    TOUCHINFO ti;
    if (!bbct.getSamples(&ti)) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = ti.x[0];
        data->point.y = ti.y[0];
    }
}

// Software Status Mask to solve the ESP32 Output Read Bug
static volatile uint16_t s_active_relay_mask = 0;

// Dual-Core FreeRTOS Task Handles
TaskHandle_t h_task_ui = NULL;
TaskHandle_t h_task_plc = NULL;

/**
 * @brief Boot Zero-Glitch Clamping
 * Guaranteed Active-LOW relay de-energizing in the very first nanoseconds of boot.
 */
void hal_relays_boot_clamp(void) {
    const uint8_t relay_pins[] = {
        PIN_RELAY_Q0, PIN_RELAY_Q1, PIN_RELAY_Q2, PIN_RELAY_Q3,
        PIN_RELAY_Q4, PIN_RELAY_Q5, PIN_RELAY_Q6, PIN_RELAY_Q7,
        PIN_RELAY_Q8, PIN_RELAY_Q9
    };
    for (size_t i = 0; i < sizeof(relay_pins); ++i) {
        gpio_set_level((gpio_num_t)relay_pins[i], 1); // 1 = OFF (Active-LOW)
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << relay_pins[i]),
            .mode = GPIO_MODE_INPUT_OUTPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
}

/**
 * @brief Thread-Safe Relay Control using Software Status Mask
 */
void relay_set(uint8_t idx, bool active) {
    if (idx >= 10) return;
    const uint8_t pins[] = {
        PIN_RELAY_Q0, PIN_RELAY_Q1, PIN_RELAY_Q2, PIN_RELAY_Q3,
        PIN_RELAY_Q4, PIN_RELAY_Q5, PIN_RELAY_Q6, PIN_RELAY_Q7,
        PIN_RELAY_Q8, PIN_RELAY_Q9
    };
    if (active) {
        gpio_set_level((gpio_num_t)pins[idx], 0); // Active-LOW ON
        s_active_relay_mask |= (1 << idx);
    } else {
        gpio_set_level((gpio_num_t)pins[idx], 1); // Active-LOW OFF
        s_active_relay_mask &= ~(1 << idx);
    }
}

bool relay_get(uint8_t idx) {
    if (idx >= 10) return false;
    return (s_active_relay_mask & (1 << idx)) != 0;
}

/**
 * @brief CORE 1: Deterministic Real-Time Industrial PLC Loop
 * Cycle Scan Time: Fixed 50ms Deterministic Cycle
 */
void task_plc_engine(void *param) {
    (void)param;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 50ms Deterministic Cycle

    while (1) {
        uint32_t now = millis();
        // Execute deterministic bakery mixer state machine
        g_machine_fsm.update(now);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

static void on_splash_complete(void) {
    Serial.println("[MAIN] on_splash_complete: building main UI...");
    // Transition to Main Bakery Mixer Dashboard
    HafflerRecipe* active_r = g_recipes.getActive();
    float init_val = (float)g_machine_fsm.getConfig().mix_time_sec;

    haffler_ui_builder_init(
        "HAFFLER BAKERY MIXER",
        "زمان باقی‌مانده میکس",
        "s",
        init_val
    );
    Serial.println("[MAIN] haffler_ui_builder_init complete!");

    if (active_r) {
        haffler_ui_set_recipe_name(active_r->name);
    }
}

/**
 * @brief CORE 0: LVGL Graphics, Touch, WebApp & Telemetry
 */
void task_ui_engine(void *param) {
    (void)param;
    Serial.println("[BOOT] task_ui_engine started on Core 0");

    // 1. Hardware Backlight & Display Initialization
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    Serial.println("[BOOT] Backlight pin 38 set HIGH");

    lv_init();
    Serial.println("[BOOT] lv_init complete");

    tft.begin();
    tft.setRotation(SCREEN_ROTATION);
    Serial.println("[BOOT] tft.begin complete");

    // 2. Hardware Capacitive Touch Initialization (GT911)
    bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
    bbct.setOrientation(TOUCH_ORIENTATION, SCREEN_HEIGHT, SCREEN_WIDTH);
    Serial.println("[BOOT] bbct touch init complete");

    // 3. LVGL Display Buffer and Driver Registration
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("[BOOT] lv_disp_drv_register complete");

    // 4. LVGL Touchpad Input Driver Registration
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    indev_drv.long_press_time = 350;
    indev_drv.long_press_repeat_time = 80;
    lv_indev_drv_register(&indev_drv);
    Serial.println("[BOOT] lv_indev_drv_register complete");

    // 5. Initialize Localization Subsystem (Default Persian)
    ui_lang_init(LANG_FA);

    // 6. Start Embedded WebServer on Core 0
    web_portal_start();
    Serial.println("[BOOT] web_portal_start complete");

    // 7. Initialize Screenshot Subsystem
    shot_init_worker();
    Serial.println("[BOOT] shot_init_worker complete");

    // 8. Initialize Agent HMI Serial Control Subsystem
    agent_hmi_init();
    Serial.println("[BOOT] agent_hmi_init complete");

    // 9. Show Haffler Industrial Startup Splash & Loading Screen
    splash_screen_show("HAFFLER BAKERY SPIRAL MIXER", "V.1.3.0", on_splash_complete);
    Serial.println("[BOOT] splash_screen_show complete");

    uint32_t last_ui_refresh_ms = 0;

    while (1) {
        // Screenshot State Machine
        shot_poll();

        // Remote Agent HMI Command Processing (Virtual Navigation, Ingestion, Shots, Overrides)
        agent_hmi_poll(Serial);

        // Hardware Front Panel Keypad Scanning (F1..F5, START, STOP, SAVE, CANCEL)
        panel_key_t key = touchless_nav_scan();
        if (key != KEY_NONE) {
            if (key == KEY_START) {
                g_machine_fsm.startCycle();
            } else if (key == KEY_STOP) {
                g_machine_fsm.stopCycle();
            } else if (key == KEY_SAVE) {
                if (g_ui_ctx.current_screen == HAFFLER_SCREEN_SETTINGS) {
                    g_machine_fsm.saveConfigToNVS();
                    haffler_ui_mark_settings_dirty(false);
                }
            } else if (key == KEY_CANCEL) {
                if (g_ui_ctx.current_screen == HAFFLER_SCREEN_SETTINGS) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_MAIN);
                }
            } else if (key >= KEY_F1 && key <= KEY_F5) {
                uint8_t tab_idx = (uint8_t)(key - KEY_F1);
                if (g_ui_ctx.current_screen != HAFFLER_SCREEN_SETTINGS) {
                    haffler_ui_switch_screen(HAFFLER_SCREEN_SETTINGS);
                }
                haffler_ui_set_settings_tab(tab_idx);
            }
        }

        // 4. LVGL Timer Handler (runs graphics pipelines)
        lv_timer_handler();

        uint32_t now = millis();
        // 5. Dirty-driven UI metrics update (only once main UI is constructed)
        if (g_ui_ctx.scr_main != NULL && (now - last_ui_refresh_ms >= 100)) {
            last_ui_refresh_ms = now;

            const bakery_live_t& live = g_machine_fsm.getLive();
            const bakery_config_t& cfg = g_machine_fsm.getConfig();

            // Hero Metric: remaining mix seconds
            haffler_ui_set_hero_value((float)live.remaining_mix_sec, 0);

            // Mix Countdown Progress Bar (0..100%)
            uint8_t progress = 0;
            if (cfg.mix_time_sec > 0 && live.remaining_mix_sec <= cfg.mix_time_sec) {
                progress = (uint8_t)((100ULL * (cfg.mix_time_sec - live.remaining_mix_sec)) / cfg.mix_time_sec);
            }
            haffler_ui_set_mix_progress(progress);

            // Live RTC Clock
            haffler_ui_set_clock(live.rtc_hour, live.rtc_minute, live.rtc_second);

            // Machine state badge color coding and Persian localization
            uint32_t state_col = 0x16A34A; // Default green
            const char* state_title = "آماده";
            uint8_t pipe_stage = 0;
            const char* cur_stage_name = "آماده شروع";
            const char* next_stage_name = "آبگیری (۲۰.۰ لیتر)";

            switch (g_machine_fsm.getState()) {
                case STATE_IDLE:
                    state_col = 0x16A34A;
                    state_title = "آماده";
                    pipe_stage = 0;
                    cur_stage_name = "آماده شروع چرخه";
                    next_stage_name = "آبگیری و تنظیم دما";
                    break;
                case STATE_PRE_TEMP_REGULATION:
                    state_col = 0xD97706;
                    state_title = "تنظیم دما";
                    pipe_stage = 0;
                    cur_stage_name = "تنظیم دمای آب مخزن";
                    next_stage_name = "آبگیری مخزن";
                    break;
                case STATE_WATER_DOSING:
                    state_col = 0x0284C7;
                    state_title = "تزریق آب";
                    pipe_stage = 1;
                    cur_stage_name = "آبگیری خمیر";
                    next_stage_name = (cfg.flour_mode == 1) ? "تزریق ماردون آرد" : "میکس خمیر";
                    break;
                case STATE_FLOUR_FEEDING:
                    state_col = 0x8957E5;
                    state_title = "تزریق آرد";
                    pipe_stage = 2;
                    cur_stage_name = "تزریق ماردون آرد";
                    next_stage_name = "میکس و ورز دادن خمیر";
                    break;
                case STATE_MIXING:
                    state_col = 0x16A34A;
                    state_title = "میکس خمیر";
                    pipe_stage = 3;
                    cur_stage_name = "میکس و ورز دادن خمیر";
                    next_stage_name = "ترمز کاسه و پایان پخت";
                    break;
                case STATE_CYCLE_COMPLETE:
                    state_col = 0x10B981;
                    state_title = "پایان پخت";
                    pipe_stage = 4;
                    cur_stage_name = "پایان پخت و تخلیه خمیر";
                    next_stage_name = "آماده چرخه بعدی";
                    break;
                case STATE_PAUSED_GUARD:
                    state_col = 0xDC2626;
                    state_title = "توقف حفاظ";
                    pipe_stage = 3;
                    cur_stage_name = "توقف به دلیل باز شدن توری";
                    next_stage_name = "بستن توری جهت ادامه";
                    break;
                case STATE_FAULT:
                case STATE_ESTOP:
                    state_col = 0xDC2626;
                    state_title = "استپ ایمنی";
                    pipe_stage = 0;
                    cur_stage_name = "توقف اضطراری سیستم";
                    next_stage_name = "بررسی کلید استپ و خطای حرارتی";
                    break;
                default:
                    state_col = 0x4B5563;
                    state_title = "آماده";
                    break;
            }
            haffler_ui_set_machine_state(state_title, state_col);
            haffler_ui_set_pipeline_stage(pipe_stage, cur_stage_name, next_stage_name);

            // Detailed Status Sub-cards
            haffler_ui_set_water_info(live.current_water_dosed_l, cfg.water_target_liters);
            haffler_ui_set_temp_info(live.water_temp_c, cfg.water_target_temp_c, live.water_temp_ready);
            haffler_ui_set_flour_info(cfg.flour_mode == 1, cfg.flour_feed_time_sec);
            haffler_ui_set_brake_info(cfg.bowl_brake_enabled, live.bowl_brake_active);
            haffler_ui_set_guard_status(live.guard_closed);
            haffler_ui_set_batch_count(live.batches_count);

            // Day-Lock Screen & Tab 4 updates
            if (g_ui_ctx.current_screen == HAFFLER_SCREEN_DAY_LOCK ||
               (g_ui_ctx.current_screen == HAFFLER_SCREEN_SETTINGS && g_ui_ctx.active_settings_tab == 4)) {
                jalali_date_t jd = gregorian_to_jalali(live.rtc_year, live.rtc_month, live.rtc_day);
                char jalali_str[32];
                snprintf(jalali_str, sizeof(jalali_str), "%04d/%02d/%02d", jd.year, jd.month, jd.day);
                haffler_ui_update_day_lock_screen(
                    live.daylock_stage,
                    live.daylock_days_left,
                    g_machine_fsm.getDayLockChallenge(),
                    jalali_str
                );
            }

            // Diagnostics Screen & Tab 3 I/O status lamps
            if (g_ui_ctx.current_screen == HAFFLER_SCREEN_DIAGNOSTICS ||
               (g_ui_ctx.current_screen == HAFFLER_SCREEN_SETTINGS && g_ui_ctx.active_settings_tab == 3)) {
                haffler_ui_set_input_lamp(0, !live.estop_ok);
                haffler_ui_set_input_lamp(1, digitalRead(PIN_INPUT_START) == 0);
                haffler_ui_set_input_lamp(2, live.guard_closed);
                haffler_ui_set_input_lamp(3, !live.overload_ok);
                haffler_ui_set_input_lamp(4, digitalRead(PIN_INPUT_FLOWMETER) == 0);
                haffler_ui_set_input_lamp(5, live.water_temp_valid);
                haffler_ui_set_input_lamp(6, false);
                haffler_ui_set_input_lamp(7, false);

                for (uint8_t i = 0; i < 10; ++i) {
                    haffler_ui_set_output_lamp(i, relay_get(i));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void setup() {
    // 1. CRITICAL: Boot Safe Clamping FIRST
    hal_relays_boot_clamp();

    // 2. Initialize Hardware Serial for Telemetry & Debugging
    Serial.begin(921600);
    Serial.println("[BOOT] setup() starting...");

    // 3. Initialize I2C Bus for DS3231 RTC and GT911 Capacitive Touch
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Serial.println("[BOOT] Wire.begin() complete");

    // 4. Initialize Recipes Subsystem
    g_recipes.begin();

    // 5. Initialize Bakery Mixer FSM Engine
    g_machine_fsm.init();

    // Initialize Front-Panel Touchless Navigation (Keypad Ladder ADC)
    touchless_nav_init();

    // Synchronize active recipe to FSM
    HafflerRecipe* r = g_recipes.getActive();
    if (r) {
        g_machine_fsm.setMixTimeSec((uint32_t)r->params[0]);
        g_machine_fsm.setDosingMode((uint8_t)r->params[1]);
        g_machine_fsm.setWaterTargetLiters(r->params[2]);
        g_machine_fsm.setWaterTargetTemp(r->params[3]);
    }

    // 6. Launch Core 1 Deterministic PLC Engine
    xTaskCreatePinnedToCore(
        task_plc_engine,
        "PLC_Task",
        8192,
        NULL,
        configMAX_PRIORITIES - 1, // Highest Real-Time Priority
        &h_task_plc,
        1 // CORE 1
    );

    // 7. Launch Core 0 UI & Network Engine
    xTaskCreatePinnedToCore(
        task_ui_engine,
        "UI_Task",
        16384,
        NULL,
        2, // UI Priority
        &h_task_ui,
        0 // CORE 0
    );
}

void loop() {
    // Arduino default loop is idle; FreeRTOS dual-core tasks handle all operations.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
