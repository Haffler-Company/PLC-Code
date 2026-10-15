#include "variables.h"
#include "actions.h"
#include "screens.h"
#include "a2_eeprom.h"
#include "webportal.h"

unsigned char general1 = 0;
volatile unsigned int TimerValve1 = 0, TimerValve1_Saved = 0;
volatile unsigned int TimerValve2 = 0, TimerValve2_Saved = 0;
volatile unsigned int TimerLogo = 0;
char vr_OutPutType = 0;
unsigned char buzzerStatus = 0, buzzerEnable = 1;
volatile unsigned char buzzerTimeVlaue = 0;
unsigned char touchFeedbackEnable = 1;

volatile uint8_t ui_logo_hide_req = 0;
volatile uint8_t ui_bar1_pending = 0;
volatile uint8_t ui_bar2_pending = 0;
volatile uint8_t ui_bar1_value = 0;
volatile uint8_t ui_bar2_value = 0;
volatile uint8_t ui_complete1_pending = 0;
volatile uint8_t ui_complete2_pending = 0;

extern unsigned long millis(void);

/* Q2 smart acoustic alert state machine */
typedef enum {
    Q2_ALERT_IDLE = 0,
    Q2_ALERT_START_BEEP,      /* 180ms single beep */
    Q2_ALERT_COMPLETE_BEEP1,  /* 100ms first beep */
    Q2_ALERT_COMPLETE_GAP,    /* 80ms silence */
    Q2_ALERT_COMPLETE_BEEP2   /* 100ms second beep */
} q2_alert_state_t;

static q2_alert_state_t s_alert_state = Q2_ALERT_IDLE;
static uint32_t s_alert_timer = 0;

void q2_alert_start(void)
{
    if (!buzzerEnable) return;
    HwSetBuzzer(true);
    s_alert_state = Q2_ALERT_START_BEEP;
    s_alert_timer = (uint32_t)millis() + 180;
}

void q2_alert_complete(void)
{
    if (!buzzerEnable) return;
    HwSetBuzzer(true);
    s_alert_state = Q2_ALERT_COMPLETE_BEEP1;
    s_alert_timer = (uint32_t)millis() + 100;
}

static void q2_alert_poll(void)
{
    if (s_alert_state == Q2_ALERT_IDLE) return;

    if (!buzzerEnable) {
        HwSetBuzzer(false);
        s_alert_state = Q2_ALERT_IDLE;
        return;
    }

    uint32_t now = (uint32_t)millis();
    if ((int32_t)(now - s_alert_timer) < 0) return;

    switch (s_alert_state) {
        case Q2_ALERT_START_BEEP:
            HwSetBuzzer(false);
            s_alert_state = Q2_ALERT_IDLE;
            break;

        case Q2_ALERT_COMPLETE_BEEP1:
            HwSetBuzzer(false);
            s_alert_state = Q2_ALERT_COMPLETE_GAP;
            s_alert_timer = now + 80;
            break;

        case Q2_ALERT_COMPLETE_GAP:
            HwSetBuzzer(true);
            s_alert_state = Q2_ALERT_COMPLETE_BEEP2;
            s_alert_timer = now + 100;
            break;

        case Q2_ALERT_COMPLETE_BEEP2:
            HwSetBuzzer(false);
            s_alert_state = Q2_ALERT_IDLE;
            break;

        default:
            HwSetBuzzer(false);
            s_alert_state = Q2_ALERT_IDLE;
            break;
    }
}

void ui_process_isr_requests(void)
{
    if (ui_logo_hide_req) {
        ui_logo_hide_req = 0;
        HideLogo();
    }
    if (ui_bar1_pending) {
        ui_bar1_pending = 0;
        ChangeBar(ui_bar1_value);
    }
    if (ui_bar2_pending) {
        ui_bar2_pending = 0;
        ChangeBar1(ui_bar2_value);
    }
    if (ui_complete1_pending) {
        ui_complete1_pending = 0;
        ui_main_show_station_status(0, UI_MAIN_STATUS_COMPLETE);
        ui_main_schedule_bar_reset(0);
        if (TimerValve2 == 0) {
            q2_alert_complete();
        }
        EEprom_TotalFilled_Glass2++;
        EEprom_TotalSeconds_Glass2 += (TimerValve1_Saved / 10);
        stats_increment(0, 1);
        a2_eeprom_write_all();
    }
    if (ui_complete2_pending) {
        ui_complete2_pending = 0;
        ui_main_show_station_status(1, UI_MAIN_STATUS_COMPLETE);
        ui_main_schedule_bar_reset(1);
        if (TimerValve1 == 0) {
            q2_alert_complete();
        }
        EEprom_TotalFilled_Glass1++;
        EEprom_TotalSeconds_Glass1 += (TimerValve2_Saved / 10);
        stats_increment(1, 0);
        a2_eeprom_write_all();
    }

    q2_alert_poll();
}

