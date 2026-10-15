# Coffee Mixer + Haffler_WEB — Anti-Patterns Catalog
## روش‌هایی که امتحان شد و شکست خورد

> **AGENT MANDATE:** Scan this BEFORE writing any code. If your approach matches an anti-pattern, STOP.

---

## 🔴 CRITICAL — Coffee Mixer

### AP-CM-CRIT-01: Setting GPIO32 HIGH at Boot (Motor Runaway)
- **What agents try:** Initializing GPIO32 as output without first explicitly setting it LOW.
- **What happens:** Motor spins immediately at boot — dangerous with portafilter potentially unseated.
- **Correct approach:** `digitalWrite(32, LOW); pinMode(32, OUTPUT);` — set value BEFORE direction.

### AP-CM-CRIT-02: Skipping 0x814E Clear on GT911
- **What agents try:** Only reading GT911 coordinates without clearing status register.
- **What happens:** GT911 freezes within minutes of startup. Touch stops responding permanently until power cycle.
- **Correct approach:** Write `0x00` to `0x814E` on EVERY poll iteration. No exceptions.

### AP-CM-CRIT-03: Calling TFT Functions from Core 0 (webTask)
- **What agents try:** Updating display from WiFi callback or REST handler running on Core 0.
- **What happens:** TFT_eSPI is not thread-safe — display garbles, ESP32 panics with Cache_ReadAddr error.
- **Correct approach:** Set a `volatile bool` flag in Core 0. Core 1 loop() reads flag and does the actual drawing.

### AP-CM-CRIT-04: Applying PLC Active-LOW Relay Logic to Motor
- **What agents try:** Copying relay control from ICE CREAM or Bakery PLC firmware.
- **What happens:** Motor is permanently ON (PLC logic writes 0 to "activate", Coffee Mixer GPIO32 also starts when 0).
- **WAIT — it's the opposite:** Coffee Mixer GPIO32 = Active HIGH. LOW = OFF, HIGH = ON.
- **Correct approach:** `digitalWrite(32, HIGH)` = motor ON, `digitalWrite(32, LOW)` = motor OFF.

---

## 🔴 CRITICAL — Haffler_WEB

### AP-WEB-CRIT-01: threading.Lock in async def Handlers
- **What agents try:** `with self._lock:` inside an `async def` FastAPI route.
- **What happens:** Entire event loop blocks. All other requests queue. Server appears hung under load.
- **Correct approach:** `async with self._async_lock:` using `asyncio.Lock()`.

### AP-WEB-CRIT-02: Reading index.html Sequentially (Line by Line)
- **What agents try:** ViewFile L1-800, then L801-1600, then L1601-2400...
- **What happens:** index.html is 1.6MB ≈ 50,000+ lines. Sequential reading wastes entire context window. Session becomes useless.
- **Correct approach:** `grep_search` for the CSS class, function name, or HTML ID needed. Read ±100 lines around it.

### AP-WEB-CRIT-03: Deploying Without SECRET_KEY Set
- **What agents try:** Starting server with default key fallback for "testing".
- **What happens:** JWT tokens are forgeable with known default secret. Auth bypass on production.
- **Correct approach:** Server must refuse to start if SECRET_KEY is missing or equals default.

---

## 🟠 HIGH — Both Projects

### AP-HIGH-01: Inserting EEPROM Settings in Middle of Layout
- **What agents try:** Adding new byte at offset 5, shifting all offsets after it.
- **What happens:** All deployed customer machines read corrupted settings on next boot.
- **Correct approach:** Append ONLY to end of EEPROM map.

### AP-HIGH-02: Proposing React/Vue for index.html
- Already rejected. See Haffler_WEB DECISION_LOG.md DEC-BE-02.

### AP-HIGH-03: Changing MQTT Topic Paths
- **What agents try:** Renaming `haffler/{id}/status` to `haffler/{id}/telemetry` for clarity.
- **What happens:** All 11 physical machines stop sending data to the web portal instantly.
- **Correct approach:** MQTT topics are FROZEN. Coordinate any change with firmware team + simultaneous OTA to all 11 machines.

### AP-HIGH-04: WS2812B LED Updates Without Interrupt Protection
- **What agents try:** Calling `FastLED.show()` or `pixels.show()` in a loop while WiFi is active.
- **What happens:** WiFi interrupt corrupts LED signal timing → LEDs flash wrong colors randomly.
- **Correct approach:** Use ESP32 RMT peripheral, or disable interrupts during LED write.

---

## 🟡 MEDIUM

### AP-MED-01: Proposing ESP32-S3 or LVGL for Coffee Mixer
- Coffee Mixer uses standard ESP32 + TFT_eSPI. No PSRAM = no LVGL double buffer.
- See DECISION_LOG.md DEC-HW-01 and DEC-HW-02.

### AP-MED-02: WDT-Dangerous Blocking Operations
- `WiFi.begin()`, EEPROM writes, OTA downloads must not block > 8s without feeding WDT.
- WDT timeout = 10s on Coffee Mixer.

### AP-MED-03: Proposing Private MQTT Broker as Immediate Fix
- Already planned for future. Current public HiveMQ is intentional temporary decision.
- See Haffler_WEB DECISION_LOG.md DEC-INF-03.

---

## ✍️ HOW TO ADD
ID format: `AP-{PROJECT}-{SEVERITY}-{NN}` (CM=Coffee Mixer, WEB=Haffler_WEB, CRIT/HIGH/MED)
Never remove entries. Mark superseded ones: `[SUPERSEDED by AP-xx]`
