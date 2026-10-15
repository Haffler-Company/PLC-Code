#ifndef GASFILLING_VARIABLES_H
#define GASFILLING_VARIABLES_H

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char general1;
extern volatile unsigned int TimerValve1, TimerValve1_Saved;
extern volatile unsigned int TimerValve2, TimerValve2_Saved;
extern volatile unsigned int TimerLogo;

extern char vr_OutPutType;
extern unsigned char buzzerStatus, buzzerEnable;
extern volatile unsigned char buzzerTimeVlaue;
/* UI tap beeps: gated by Sound (buzzerEnable) AND Touch feedback. */
extern unsigned char touchFeedbackEnable;

/* ISR -> main-loop UI requests (LVGL is not ISR-safe). */
extern volatile uint8_t ui_logo_hide_req;
extern volatile uint8_t ui_bar1_pending;
extern volatile uint8_t ui_bar2_pending;
extern volatile uint8_t ui_bar1_value;
extern volatile uint8_t ui_bar2_value;
extern volatile uint8_t ui_complete1_pending;
extern volatile uint8_t ui_complete2_pending;

#define buzzer_key buzzerTimeVlaue = (unsigned char)(2 * buzzerEnable * touchFeedbackEnable)

typedef enum { L_Persion = 0, L_english, L_Turkish, L_Russian } table_language;

void ui_process_isr_requests(void);
void q2_alert_start(void);
void q2_alert_complete(void);
void ApplyOutputTypeSafe(char type);
void HwSetValve1(bool active);
void HwSetValve2(bool active);
void HwSetBuzzer(bool active);

#ifdef __cplusplus
}
#endif

#endif /* GASFILLING_VARIABLES_H */
