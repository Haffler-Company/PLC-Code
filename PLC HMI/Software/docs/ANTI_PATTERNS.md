# Haffler PLC Fleet — Anti-Patterns Catalog
## روش‌هایی که امتحان شد و شکست خورد

> **AGENT MANDATE:** This is MORE valuable than best-practices. Before writing any code,
> scan this list. If your approach matches an anti-pattern, STOP and use the correct alternative.

---

## 🔴 CRITICAL ANTI-PATTERNS (Will Cause Crashes or Safety Failures)

### AP-CRIT-01: Calling lv_* Functions from Core 0 Without Lock
- **What agents try:** Updating LVGL labels or bars from PLC scan task (Core 0) directly.
- **What happens:** Random crashes, display corruption, watchdog resets. LVGL is NOT thread-safe.
- **Correct approach:**
  ```c
  if (lvgl_lock(pdMS_TO_TICKS(50))) {
      lv_label_set_text(lbl, "text");
      lvgl_unlock();
  }
  ```
- **Lesson learned:** Even "just setting a label" from Core 0 causes corruption under load.

### AP-CRIT-02: Initializing Relay Outputs Without Setting Them HIGH First
- **What agents try:** `gpio_set_direction(PIN_Q0, GPIO_MODE_OUTPUT)` without setting level first.
- **What happens:** GPIO floats during direction-set transition → relay pulses ON briefly → machine runaway.
- **Correct approach:**
  ```c
  gpio_set_level(PIN_Q0, 1);  // Set HIGH (OFF) FIRST
  gpio_set_direction(PIN_Q0, GPIO_MODE_OUTPUT);  // THEN configure as output
  ```

### AP-CRIT-03: Bypassing Anti-Short-Cycle Timer for Compressor
- **What agents try:** Directly starting compressor after fault clearance without checking delay.
- **What happens:** Compressor motor burns out (liquid refrigerant enters compressor before gas pressure equalizes).
- **Correct approach:** Always check `(xTaskGetTickCount() - g_comp_last_off_tick) >= COMP_MIN_OFF_TICKS` before starting.
- **Applies to:** ICE CREAM machine large compressor (180s minimum off time).

### AP-CRIT-04: Hardcoding Pin Numbers Outside project_config.h
- **What agents try:** `#define MY_PIN 42` in a local .c file or directly in logic code.
- **What happens:** When board is revised, pin changes are missed. Two definitions conflict silently.
- **Correct approach:** ALL pins must be in `main/project_config.h`. Zero exceptions.

### AP-CRIT-05: vTaskDelay() in PLC Scan Task (actions_task)
- **What agents try:** Adding `vTaskDelay(pdMS_TO_TICKS(1000))` inside the PLC state machine for debounce or timing.
- **What happens:** PLC scan cycle becomes non-deterministic. Safety timeouts drift. Watchdog may trigger.
- **Correct approach:** Use tick counters (`xTaskGetTickCount()`) for timing. Never block the scan task.

### AP-CRIT-06: malloc() in Periodic Loops
- **What agents try:** Allocating buffers with `malloc()` inside event handlers or periodic tasks.
- **What happens:** Heap fragmentation over time → allocation failure after hours of operation → crash.
- **Correct approach:** Pre-allocate all buffers at startup. Use static or stack-local buffers in loops.

---

## 🟠 HIGH SEVERITY ANTI-PATTERNS

### AP-HIGH-01: Unconditional lv_label_set_text_fmt in tick_screen_*
- **What agents try:** Calling `lv_label_set_text_fmt(lbl, "%d", value)` every frame without checking if value changed.
- **What happens:** DMA buffer saturated, LCD flickers, frame rate drops from 60 to 10 FPS.
- **Correct approach:**
  ```c
  static int16_t s_last_temp = INT16_MIN;
  if (current_temp != s_last_temp) {
      s_last_temp = current_temp;
      lv_label_set_text_fmt(lbl, "%d°C", current_temp);
  }
  ```

### AP-HIGH-02: Using ADC on GPIO34/35 for Digital Input
- **What agents try:** Reading portafilter or limit switch state with `digitalRead(34)` — works sometimes.
- **What happens:** Floating pin reads random values. False positives trigger unwanted machine states.
- **Correct approach:** GPIO34/35 on standard ESP32 are input-only with no internal pull-up. Use `analogRead()` only, or add external pull-up. For digital signals, use GPIO with internal pull-up.

### AP-HIGH-03: String Literals Hardcoded in Screen Files
- **What agents try:** `lv_label_set_text(lbl, "دما")` directly in screen code.
- **What happens:** When language changes, Persian strings remain. Zero-Language-Leak policy violated.
- **Correct approach:** Register in `ui_lang.h` as `STR_TEMPERATURE`, add all 3 translations, use `ui_lang_get(STR_TEMPERATURE)`.

### AP-HIGH-04: Reading index.html / functions.h / main.py in Sequential Slices
- **What agents try:** Read L1-200, then L201-400, then L401-600...
- **What happens:** Agent context fills with irrelevant code. Slows session by 10x. Often reads wrong section.
- **Correct approach:** `grep_search` for exact symbol → read only the ±50 lines around it in one call.

### AP-HIGH-05: Using dponqual / printf for Debugging in Production Code
- **What agents try:** Leaving `printf("DEBUG: val=%d\n", x)` in production firmware.
- **What happens:** Serial output overhead measurable in tight 50ms scan cycles. Also: customer can see debug output.
- **Correct approach:** Wrap in `#ifdef CONFIG_DEBUG_LOGGING` or use structured log levels.

---

## 🟡 MEDIUM SEVERITY ANTI-PATTERNS

### AP-MED-01: Proposing React/Vue for Haffler_WEB Frontend
- **What agents try:** "Let's migrate index.html to React for better maintainability."
- **Why rejected:** Build pipeline complexity, team has no npm expertise, would require full rewrite of 1.6MB file.
- **Decision reference:** DEC-BE-02 in Haffler_WEB DECISION_LOG.md.

### AP-MED-02: Adding New EEPROM Settings in the Middle of the Layout
- **What agents try:** Inserting a new setting byte at offset 10, shifting all subsequent offsets.
- **What happens:** All existing customer machines read wrong values for every setting after offset 10.
- **Correct approach:** Always append to the END of EEPROM layout. Document new offset in layout table.

### AP-MED-03: Soft Shadows on LVGL Objects
- **What agents try:** `lv_obj_set_style_shadow_width(obj, 10, 0)` for visual depth.
- **What happens:** Shadow rendering takes significant CPU per frame. Drops FPS from 60 to 30-40.
- **Correct approach:** `lv_obj_set_style_shadow_width(obj, 0, 0)` — zero shadows. Use borders instead.

### AP-MED-04: Importing threading.Lock in FastAPI Async Code
- **What agents try:** Using Python's `threading.Lock()` to protect shared state in async handlers.
- **Why it fails:** Blocks the event loop when contested. See KNOWN_AUDIT_FINDINGS.md BUG-01.
- **Correct approach:** `asyncio.Lock()`.

### AP-MED-05: Using %02d:%02d Format for Persian Time Display
- **What agents try:** `lv_label_set_text_fmt(lbl, "%02d:%02d", min, sec)` in Persian mode.
- **What happens:** BiDi algorithm reverses digits. "05:30" displays as "30:05" in RTL context.
- **Correct approach:** Use descriptive text: "۵ دقیقه و ۳۰ ثانیه" or use LRO Unicode isolation mark.

### AP-MED-06: Proposing STM32 or Raspberry Pi Pico as Controller
- Already rejected. See DECISION_LOG.md DEC-HW-01.

---

## ✍️ HOW TO ADD ANTI-PATTERNS

When you discover something that went wrong:
1. Give it an ID: `AP-{SEVERITY}-{NN}` (CRIT/HIGH/MED)
2. Document: "What agents try", "What happens", "Correct approach"
3. Add "Decision reference" if a DECISION_LOG entry exists
4. Never remove entries — they are permanent lessons
