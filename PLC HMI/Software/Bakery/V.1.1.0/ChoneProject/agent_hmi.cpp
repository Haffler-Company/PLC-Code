#include "agent_hmi.h"
#include "ui.h"
#include <esp_system.h>
#include <esp_heap_caps.h>

volatile bool g_agent_touch_active = false;
volatile int16_t g_agent_touch_x = 0;
volatile int16_t g_agent_touch_y = 0;
volatile uint32_t g_agent_touch_release_ms = 0;

extern void go_to_menu(lv_obj_t *from);
extern void go_to_automaticMode(lv_obj_t *from);
extern void go_to_manualMode(lv_obj_t *from);
extern void go_to_setting(lv_obj_t *from);
extern void go_to_diagnostics(lv_obj_t *from);
extern void warning_screen_events_init();
extern void ui_WarningPage_screen_init();
extern void ui_SplashScreen_screen_init();
extern lv_obj_t *ui_WarningPage;
extern lv_obj_t *ui_SplashScreen;
extern lv_obj_t *ui_SplashScreen_SplashScreenImage;

void agent_hmi_init()
{
    g_agent_touch_active = false;
}

void agent_navigate(const char *screen_name)
{
    if (!screen_name) return;

    while (*screen_name == ' ') screen_name++;

    lv_obj_t *act = lv_scr_act();

    if (strcasecmp(screen_name, "MAIN") == 0 || strcasecmp(screen_name, "MENU") == 0)
    {
        go_to_menu(act);
        Serial.println("NAV_OK MAIN");
    }
    else if (strcasecmp(screen_name, "AUTO") == 0 || strcasecmp(screen_name, "AUTOMATIC") == 0)
    {
        go_to_automaticMode(act);
        Serial.println("NAV_OK AUTO");
    }
    else if (strcasecmp(screen_name, "MANUAL") == 0)
    {
        go_to_manualMode(act);
        Serial.println("NAV_OK MANUAL");
    }
    else if (strcasecmp(screen_name, "SET") == 0 || strcasecmp(screen_name, "SETTING") == 0)
    {
        go_to_setting(act);
        Serial.println("NAV_OK SET");
    }
    else if (strcasecmp(screen_name, "DIAG") == 0 || strcasecmp(screen_name, "DIAGNOSTICS") == 0)
    {
        go_to_diagnostics(act);
        Serial.println("NAV_OK DIAG");
    }
    else if (strcasecmp(screen_name, "WARN") == 0 || strcasecmp(screen_name, "WARNING") == 0)
    {
        ui_WarningPage_screen_init();
        warning_screen_events_init();
        lv_scr_load(ui_WarningPage);
        Serial.println("NAV_OK WARN");
    }
    else if (strcasecmp(screen_name, "SPLASH") == 0)
    {
        ui_SplashScreen_screen_init();
        if (ui_SplashScreen_SplashScreenImage)
        {
            lv_obj_set_style_opa(ui_SplashScreen_SplashScreenImage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_scr_load(ui_SplashScreen);
        Serial.println("NAV_OK SPLASH");
    }
    else
    {
        Serial.printf("NAV_ERR unknown screen: %s\n", screen_name);
    }
}

static char rx_buf[64];
static uint8_t rx_len = 0;

void agent_hmi_poll()
{
    while (Serial.available())
    {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n')
        {
            if (rx_len == 0) continue;
            rx_buf[rx_len] = '\0';

            if (strcmp(rx_buf, "SHOT") == 0)
            {
                shot_request(&Serial);
            }
            else if (strcmp(rx_buf, "PING") == 0)
            {
                Serial.println("PONG");
            }
            else if (strncmp(rx_buf, "NAV ", 4) == 0)
            {
                agent_navigate(rx_buf + 4);
            }
            else if (strncmp(rx_buf, "TOUCH ", 6) == 0)
            {
                int tx = 0, ty = 0;
                if (sscanf(rx_buf + 6, "%d %d", &tx, &ty) == 2)
                {
                    g_agent_touch_x = (int16_t)tx;
                    g_agent_touch_y = (int16_t)ty;
                    g_agent_touch_release_ms = millis() + 150;
                    g_agent_touch_active = true;
                    Serial.printf("TOUCH_OK %d %d\n", tx, ty);
                }
            }
            else if (strcmp(rx_buf, "MEM") == 0)
            {
                Serial.printf("MEM free_heap=%u min_heap=%u psram=%u\n",
                              (unsigned)esp_get_free_heap_size(),
                              (unsigned)esp_get_minimum_free_heap_size(),
                              (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            }

            rx_len = 0;
        }
        else if (rx_len < sizeof(rx_buf) - 1)
        {
            rx_buf[rx_len++] = c;
        }
    }
}
