// task: buzzer handler
#include "driver/ledc.h"

#define BUZZER_PIN 45
#define BUZZER_FREQ 2700
#define BUZZER_RES 8
#define BUZZER_CH 2

// Task2 نباید حین بوق، LEDC پین ۴۵ را صفر کند (با بک‌لایت مشترک است)
extern volatile uint32_t buzzer_hold_until_ms;
extern uint8_t sys_volume;
extern uint8_t alarm_volume;

static portMUX_TYPE buzzer_request_mux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t buzzer_pending_pattern = BUZZER_PATTERN_NONE;
static bool buzzer_request_accepting = true;

bool buzzer_request_pattern(uint8_t pattern) {
  if (pattern < BUZZER_PATTERN_SHORT || pattern > BUZZER_PATTERN_STEP_DOWN) return false;
  bool accepted = false;
  portENTER_CRITICAL(&buzzer_request_mux);
  if (buzzer_request_accepting && buzzer_pending_pattern == BUZZER_PATTERN_NONE) {
    buzzer_pending_pattern = pattern;
    buzzer_request_accepting = false;
    accepted = true;
  }
  portEXIT_CRITICAL(&buzzer_request_mux);
  return accepted;
}

static uint8_t buzzer_take_pattern() {
  portENTER_CRITICAL(&buzzer_request_mux);
  const uint8_t pattern = buzzer_pending_pattern;
  buzzer_pending_pattern = BUZZER_PATTERN_NONE;
  portEXIT_CRITICAL(&buzzer_request_mux);
  return pattern;
}

static void buzzer_finish_pattern() {
  portENTER_CRITICAL(&buzzer_request_mux);
  buzzer_pending_pattern = BUZZER_PATTERN_NONE;
  buzzer_request_accepting = true;
  portEXIT_CRITICAL(&buzzer_request_mux);
}

struct BuzzerToneStep {
  uint16_t freq;
  uint16_t dur_ms;
  uint16_t gap_ms;
};

static const BuzzerToneStep PAT_SHORT[] = {
  { 2700, 35, 0 }
};
static const BuzzerToneStep PAT_SUCCESS[] = {
  { 2200, 50, 40 },
  { 3200, 70, 0 }
};
static const BuzzerToneStep PAT_WARN[] = {
  { 1900, 160, 0 }
};
static const BuzzerToneStep PAT_LONG[] = {
  { 1300, 320, 0 }
};
static const BuzzerToneStep PAT_BOOT[] = {
  { 2000, 45, 35 },
  { 3000, 60, 0 }
};
static const BuzzerToneStep PAT_COMPLETE[] = {
  { 2000, 45, 35 },
  { 2600, 45, 35 },
  { 3400, 70, 0 }
};
static const BuzzerToneStep PAT_STEP_UP[] = {
  { 3400, 22, 0 }
};
static const BuzzerToneStep PAT_STEP_DOWN[] = {
  { 2000, 22, 0 }
};

void Task5(void *pvParameters) {
  vTaskDelay(350 / portTICK_PERIOD_MS);
  Serial.println("task5 start");
  ledcAttachChannel(BUZZER_PIN, BUZZER_FREQ, BUZZER_RES, BUZZER_CH);
  ledcWrite(BUZZER_PIN, 0);
  GUI.buzzer_cmd_short = false;
  GUI.buzzer_cmd_long = false;

  const BuzzerToneStep *cur_steps = NULL;
  uint8_t total_steps = 0;
  uint8_t cur_step_idx = 0;
  bool sounding = false;
  uint32_t deadline_ms = 0;

  while (1) {
    const uint32_t now = millis();
    if (GUI.buzzer_cmd_long) {
      GUI.buzzer_cmd_short = false;
      GUI.buzzer_cmd_long = false;
      (void)buzzer_request_pattern(BUZZER_PATTERN_LONG);
    } else if (GUI.buzzer_cmd_short) {
      GUI.buzzer_cmd_short = false;
      (void)buzzer_request_pattern(BUZZER_PATTERN_SHORT);
    }

    if (!GUI.buzzer_enable) {
      cur_steps = NULL;
      total_steps = 0;
      cur_step_idx = 0;
      sounding = false;
      ledcWrite(BUZZER_PIN, 0);
      buzzer_hold_until_ms = 0;
      buzzer_finish_pattern();
    } else if (!sounding && cur_step_idx >= total_steps) {
      const uint8_t req = buzzer_take_pattern();
      if (req == BUZZER_PATTERN_NONE) {
        vTaskDelay(pdMS_TO_TICKS(5));
        continue;
      }
      switch (req) {
        case BUZZER_PATTERN_LONG:
          cur_steps = PAT_LONG;
          total_steps = sizeof(PAT_LONG) / sizeof(PAT_LONG[0]);
          break;
        case BUZZER_PATTERN_WARN:
          cur_steps = PAT_WARN;
          total_steps = sizeof(PAT_WARN) / sizeof(PAT_WARN[0]);
          break;
        case BUZZER_PATTERN_SUCCESS:
          cur_steps = PAT_SUCCESS;
          total_steps = sizeof(PAT_SUCCESS) / sizeof(PAT_SUCCESS[0]);
          break;
        case BUZZER_PATTERN_BOOT:
          cur_steps = PAT_BOOT;
          total_steps = sizeof(PAT_BOOT) / sizeof(PAT_BOOT[0]);
          break;
        case BUZZER_PATTERN_COMPLETE:
          cur_steps = PAT_COMPLETE;
          total_steps = sizeof(PAT_COMPLETE) / sizeof(PAT_COMPLETE[0]);
          break;
        case BUZZER_PATTERN_STEP_UP:
          cur_steps = PAT_STEP_UP;
          total_steps = sizeof(PAT_STEP_UP) / sizeof(PAT_STEP_UP[0]);
          break;
        case BUZZER_PATTERN_STEP_DOWN:
          cur_steps = PAT_STEP_DOWN;
          total_steps = sizeof(PAT_STEP_DOWN) / sizeof(PAT_STEP_DOWN[0]);
          break;
        case BUZZER_PATTERN_SHORT:
        case BUZZER_PATTERN_SINGLE_CLICK:
        default:
          cur_steps = PAT_SHORT;
          total_steps = sizeof(PAT_SHORT) / sizeof(PAT_SHORT[0]);
          break;
      }
      uint8_t target_vol = sys_volume;
      if (req == BUZZER_PATTERN_LONG || req == BUZZER_PATTERN_WARN ||
          req == BUZZER_PATTERN_SUCCESS || req == BUZZER_PATTERN_COMPLETE ||
          req == BUZZER_PATTERN_BOOT) {
        target_vol = alarm_volume;
      } else {
        target_vol = sys_volume;
      }
      uint32_t duty = (uint32_t)target_vol * 254U / 100U;
      if (duty == 0 && target_vol > 0) duty = 1;

      cur_step_idx = 0;
      if (cur_steps && total_steps > 0 && target_vol > 0) {
        ledcAttachChannel(BUZZER_PIN, cur_steps[0].freq, BUZZER_RES, BUZZER_CH);
        ledcWrite(BUZZER_PIN, duty);
        sounding = true;
        deadline_ms = now + cur_steps[0].dur_ms;
        buzzer_hold_until_ms = deadline_ms;
      } else {
        buzzer_finish_pattern();
      }
    } else if (sounding && (int32_t)(now - deadline_ms) >= 0) {
      ledcWrite(BUZZER_PIN, 0);
      sounding = false;
      const uint16_t gap = cur_steps ? cur_steps[cur_step_idx].gap_ms : 0;
      cur_step_idx++;
      if (cur_step_idx < total_steps && gap > 0) {
        deadline_ms = now + gap;
      } else if (cur_step_idx >= total_steps) {
        buzzer_hold_until_ms = 0;
        buzzer_finish_pattern();
      }
    } else if (!sounding && cur_step_idx < total_steps && (int32_t)(now - deadline_ms) >= 0) {
      if (cur_steps) {
        uint8_t target_vol = sys_volume;
        if (cur_steps == PAT_LONG || cur_steps == PAT_WARN ||
            cur_steps == PAT_SUCCESS || cur_steps == PAT_COMPLETE ||
            cur_steps == PAT_BOOT) {
          target_vol = alarm_volume;
        }
        uint32_t duty = (uint32_t)target_vol * 254U / 100U;
        if (duty == 0 && target_vol > 0) duty = 1;
        if (target_vol > 0) {
          ledcAttachChannel(BUZZER_PIN, cur_steps[cur_step_idx].freq, BUZZER_RES, BUZZER_CH);
          ledcWrite(BUZZER_PIN, duty);
          sounding = true;
          deadline_ms = now + cur_steps[cur_step_idx].dur_ms;
          buzzer_hold_until_ms = deadline_ms;
        } else {
          cur_step_idx = total_steps;
          buzzer_hold_until_ms = 0;
          buzzer_finish_pattern();
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
