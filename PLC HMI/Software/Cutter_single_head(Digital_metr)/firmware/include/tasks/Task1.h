//task:  lvgl tick task
void Task1(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(150));
  Serial.println("task1 start");
  // B6 fix: تیک بر مبنای millis — قبلاً lv_tick_inc(1) با vTaskDelay(1) بود که
  // هنگام مشغولی CPU از زمان واقعی عقب می‌ماند (کندی انیمیشن/تایمرهای LVGL)
  uint32_t last_ms = millis();
  while (1) {
    uint32_t now_ms = millis();
    uint32_t delta = now_ms - last_ms;
    if (delta > 0) {
      lv_tick_inc(delta);
      last_ms = now_ms;
    }
    // The delta is measured from millis(), so a 5 ms poll keeps exact LVGL
    // time while avoiding a needless 1000 Hz task wake-up on the render core.
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
