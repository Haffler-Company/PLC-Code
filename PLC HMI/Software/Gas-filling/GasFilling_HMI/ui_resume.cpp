#include "ui_resume.h"
#include <Arduino.h>
#include <esp_attr.h>
#include <esp_system.h>
#include "ui.h"
#include "screens.h"
#include "day_lock.h"

#define UI_RESUME_MAGIC   0xD4A61E5AUL
#define UI_RESUME_HEADER  0xA7000000UL

/* Fast RTC Slow Memory: Survives software resets, panics, and watchdogs.
   Zero Flash Wear, Zero EEPROM consumption, 0 microseconds bus overhead. */
RTC_NOINIT_ATTR static uint32_t s_rtc_resume_magic;
RTC_NOINIT_ATTR static uint32_t s_rtc_resume_magic_inv;
RTC_NOINIT_ATTR static uint32_t s_rtc_resume_code;

static bool s_boot_latched = false;
static bool s_resume_valid = false;
static esp_reset_reason_t s_reset_reason = ESP_RST_UNKNOWN;
static int s_target_screen = (int)SCREEN_ID_MAIN;
static int s_target_tab = 0;

static int s_last_polled_screen = -1;
static int s_last_polled_tab = -1;
static uint32_t s_last_poll_ms = 0;

static bool is_hang_reset_reason(esp_reset_reason_t rr) {
    switch (rr) {
        case ESP_RST_PANIC:       /* Software exception / crash */
        case ESP_RST_INT_WDT:     /* Interrupt watchdog */
        case ESP_RST_TASK_WDT:    /* FreeRTOS task watchdog */
        case ESP_RST_WDT:         /* Main system watchdog */
        case ESP_RST_SW:          /* Software restart (e.g. recovery restart) */
#ifdef ESP_RST_CPU_LOCKUP
        case ESP_RST_CPU_LOCKUP:  /* CPU lockup */
#endif
            return true;
        default:
            return false;         /* Cold power-on, reset button, brownout */
    }
}

void ui_resume_boot_latch_early(void) {
    if (s_boot_latched) return;
    s_boot_latched = true;

    s_reset_reason = esp_reset_reason();
    const bool is_hang = is_hang_reset_reason(s_reset_reason);

    /* Strict verification with dual magic check and 0xA7 header */
    const bool magic_ok = (s_rtc_resume_magic == UI_RESUME_MAGIC) &&
                          (s_rtc_resume_magic_inv == ~UI_RESUME_MAGIC) &&
                          ((s_rtc_resume_code & 0xFF000000UL) == UI_RESUME_HEADER);

    if (is_hang && magic_ok) {
        uint32_t code = s_rtc_resume_code;
        int scr = (int)(code & 0x7UL);
        int tab = (int)((code >> 3) & 0x7UL);

        if (scr >= (int)SCREEN_ID_MAIN && scr <= (int)_SCREEN_ID_LAST) {
            s_target_screen = scr;
            s_target_tab = (tab >= 0 && tab <= 4) ? tab : 0;
            s_resume_valid = true;
            Serial.printf("[UI_RESUME] RECOVERY LATCH: rr=%d -> Target Screen=%d Tab=%d\n",
                          (int)s_reset_reason, s_target_screen, s_target_tab);
            return;
        }
    }

    /* Cold boot or invalid RTC memory: clear and start fresh on Main */
    ui_resume_clear();
    s_resume_valid = false;
    s_target_screen = (int)SCREEN_ID_MAIN;
    s_target_tab = 0;
    Serial.printf("[UI_RESUME] Normal boot: rr=%d (hang=%d, magic=%d)\n",
                  (int)s_reset_reason, (int)is_hang, (int)magic_ok);
}

bool ui_resume_has_target(void) {
    return s_boot_latched && s_resume_valid;
}

int ui_resume_get_target_screen(void) {
    return s_target_screen;
}

int ui_resume_get_target_tab(void) {
    return s_target_tab;
}

void ui_resume_apply_if_pending(void) {
    if (!s_resume_valid) return;
    s_resume_valid = false;

    /* If device is locked by day-lock, safety policy gives priority to lock screen */
    if (device_locked) {
        Serial.println("[UI_RESUME] Skipped: device locked by DayLock");
        return;
    }

    Serial.printf("[UI_RESUME] Seamlessly restoring screen %d tab %d\n", s_target_screen, s_target_tab);

    if (s_target_screen != (int)SCREEN_ID_MAIN) {
        loadScreen((enum ScreensEnum)s_target_screen);
    }

    if (s_target_screen == (int)SCREEN_ID_SETTING && objects.tabview_setting) {
        lv_tabview_set_active(objects.tabview_setting, (uint32_t)s_target_tab, LV_ANIM_OFF);
    }

    /* Invalidate and force immediate redraw */
    lv_obj_t *act = lv_screen_active();
    if (act) {
        lv_obj_invalidate(act);
    }
    lv_refr_now(lv_display_get_default());
}

void ui_resume_checkpoint(void) {
    /* If resume apply is pending, do not overwrite until boot recovery is finished */
    if (s_resume_valid) return;

    int scr = ui_current_screen_id();
    if (scr < (int)SCREEN_ID_MAIN || scr > (int)_SCREEN_ID_LAST) return;

    int tab = 0;
    if (scr == (int)SCREEN_ID_SETTING && objects.tabview_setting) {
        tab = (int)lv_tabview_get_tab_active(objects.tabview_setting);
        if (tab < 0) tab = 0;
        if (tab > 2) tab = 2;
    }

    uint32_t code = UI_RESUME_HEADER | ((uint32_t)scr & 0x7UL) | (((uint32_t)tab & 0x7UL) << 3);

    if (s_rtc_resume_magic == UI_RESUME_MAGIC &&
        s_rtc_resume_magic_inv == ~UI_RESUME_MAGIC &&
        s_rtc_resume_code == code) {
        return;
    }

    s_rtc_resume_code = code;
    s_rtc_resume_magic = UI_RESUME_MAGIC;
    s_rtc_resume_magic_inv = ~UI_RESUME_MAGIC;
    s_last_polled_screen = scr;
    s_last_polled_tab = tab;
}

void ui_resume_mark_screen(int screen_id, int tab_idx) {
    if (screen_id < (int)SCREEN_ID_MAIN || screen_id > (int)_SCREEN_ID_LAST) return;
    if (tab_idx < 0) tab_idx = 0;
    if (tab_idx > 4) tab_idx = 4;

    uint32_t code = UI_RESUME_HEADER | ((uint32_t)screen_id & 0x7UL) | (((uint32_t)tab_idx & 0x7UL) << 3);
    s_rtc_resume_code = code;
    s_rtc_resume_magic = UI_RESUME_MAGIC;
    s_rtc_resume_magic_inv = ~UI_RESUME_MAGIC;
    s_last_polled_screen = screen_id;
    s_last_polled_tab = tab_idx;
}

void ui_resume_poll(void) {
    uint32_t now = millis();
    if (now - s_last_poll_ms < 50) return;
    s_last_poll_ms = now;

    int scr = ui_current_screen_id();
    int tab = 0;
    if (scr == (int)SCREEN_ID_SETTING && objects.tabview_setting) {
        tab = (int)lv_tabview_get_tab_active(objects.tabview_setting);
    }
    if (scr != s_last_polled_screen || tab != s_last_polled_tab) {
        ui_resume_checkpoint();
    }
}

void ui_resume_clear(void) {
    s_rtc_resume_magic = 0;
    s_rtc_resume_magic_inv = 0;
    s_rtc_resume_code = 0;
}

const char* ui_resume_get_reset_reason_str(void) {
    switch (s_reset_reason) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "EXT_RESET";
        case ESP_RST_SW:        return "SW_RESTART";
        case ESP_RST_PANIC:     return "PANIC_CRASH";
        case ESP_RST_INT_WDT:   return "INT_WDT";
        case ESP_RST_TASK_WDT:  return "TASK_WDT";
        case ESP_RST_WDT:       return "SYS_WDT";
        case ESP_RST_DEEPSLEEP: return "DEEP_SLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "UNKNOWN";
    }
}

esp_reset_reason_t ui_resume_get_reset_reason(void) {
    return s_reset_reason;
}

bool ui_resume_is_last_boot_hang(void) {
    return is_hang_reset_reason(s_reset_reason);
}
