# Haffler PLC Fleet — Engineering Decision Log
## تاریخچه تصمیمات مهندسی (Single Source of Truth)

> **AGENT MANDATE:** Before suggesting an alternative architecture, check this log.
> If the alternative was already considered and rejected, do NOT re-propose it.
> Add every major decision here with the reasoning — no matter how obvious it seems.

---

## HARDWARE DECISIONS

### DEC-HW-01: ESP32-S3 (N16R8) as Main Controller
- **Decision:** Use ESP32-S3 with 16MB Flash + 8MB PSRAM for all 11 Haffler PLC machines.
- **Alternatives Considered:** STM32, Raspberry Pi Pico, ESP32 standard.
- **Reasoning:**
  - PSRAM required for LVGL frame buffers (double buffer 30KB each minimum).
  - 16MB flash needed for OTA partition + filesystem + firmware.
  - Dual-core FreeRTOS: Core 1 LVGL isolation, Core 0 PLC scan.
  - Wide community + PlatformIO + ESP-IDF support.
  - Cost-effective vs industrial PLCs (Siemens S7, etc.).
- **Decision Date:** Early project inception. **Status:** FINAL — do not revisit.

### DEC-HW-02: ILI9488 3.5" Parallel LCD (Not SPI)
- **Decision:** Use ILI9488 3.5" with 8-bit parallel interface + DMA.
- **Alternatives Considered:** SPI ILI9341 (slower), MIPI DSI (too complex).
- **Reasoning:** Parallel interface gives ~40MHz effective bandwidth vs SPI 40MHz theoretical (but with overhead). Needed for 60 FPS at 480×320.
- **Status:** FINAL.

### DEC-HW-03: GT911 Capacitive Touch (Not Resistive)
- **Decision:** GT911 I2C capacitive touch for all machines.
- **Alternatives Considered:** XPT2046 resistive, GT928.
- **Reasoning:** Industrial glove-friendly if needed. GT911 has multi-touch. More reliable in dusty/oily workshop.
- **KNOWN TRAP:** GT911 requires `0x814E` clear on every poll. See DYNAMIC_KNOWLEDGE_BASE.md.
- **Status:** FINAL.

### DEC-HW-04: Active-LOW Relay Outputs
- **Decision:** All relay outputs on all 11 PLC machines are Active-LOW (0 = ON, 1 = OFF).
- **Alternatives Considered:** Active-HIGH (used in Coffee Mixer).
- **Reasoning:** Fail-safe design — if ESP32 resets/crashes with floating pins, pull-up resistors keep relays OFF, preventing dangerous machine runaway.
- **Boot safety:** `outputs_init()` must set all output pins HIGH before configuring as GPIO output.
- **EXCEPTION:** Coffee Mixer (separate project) uses Active-HIGH for motor relay — do NOT copy PLC logic there.
- **Status:** FINAL — safety-critical, never change.

### DEC-HW-05: MAX6675 for Thermocouple Reading
- **Decision:** MAX6675 SPI thermocouple IC for temperature sensing.
- **Alternatives Considered:** MAX31855 (more accurate), ADS1115 with TC amplifier.
- **Reasoning:** MAX6675 sufficient for ±2°C accuracy needed in ice cream / bakery. Simpler SPI protocol. Lower cost.
- **Limitation:** MAX6675 has 0.25°C resolution, read interval minimum 170ms (chip datasheet).
- **Status:** FINAL for current machines. May upgrade to MAX31855 in future versions.

### DEC-HW-06: DS3231 RTC for Timekeeping
- **Decision:** DS3231 I2C hardware RTC for the Day-Lock leasing system.
- **Alternatives Considered:** Internal ESP32 RTC (drifts, loses on power loss).
- **Reasoning:** DS3231 maintains time across power cuts. Required for tamper-resistant day-count in leasing system.
- **Status:** FINAL.

---

## SOFTWARE ARCHITECTURE DECISIONS

### DEC-SW-01: LVGL as UI Framework
- **Decision:** LVGL v8.x for all HMI screens on PLC machines.
- **Alternatives Considered:** TouchGFX (expensive license), custom framebuffer, Squareline Studio.
- **Reasoning:** Open source, ESP32 optimized, large community, portable across hardware revisions.
- **CRITICAL RULE:** Never call LVGL from Core 0. All lv_* calls must be on Core 1 or behind `lvgl_lock()`.
- **Status:** FINAL.

### DEC-SW-02: FreeRTOS Dual-Core Isolation (Core 0 PLC / Core 1 UI)
- **Decision:** Core 0 = PLC scan task (50ms deterministic), Core 1 = LVGL render task.
- **Alternatives Considered:** Single-core with time-slicing.
- **Reasoning:** PLC scan must be deterministic. LVGL rendering is variable-time. Separating them prevents render lag from affecting safety-critical scan cycle.
- **Status:** FINAL — safety-critical, never change.

### DEC-SW-03: PackML State Machine for PLC Logic
- **Decision:** ISA-88 PackML state machine (Stopped → Idle → Starting → Execute → Stopping).
- **Alternatives Considered:** Simple state enum without PackML structure.
- **Reasoning:** Industry standard. Makes error handling predictable. Supports remote SCADA monitoring.
- **Status:** FINAL.

### DEC-SW-04: Trilingual UI (Persian + English + Turkish)
- **Decision:** All UI strings must be registered in `ui_lang.h` with 3 translations.
- **Alternatives Considered:** Single language (Persian only).
- **Reasoning:** Haffler machines sold to Turkish-speaking customers in neighboring markets. English needed for export.
- **CRITICAL RULE:** NEVER hardcode strings. Always use `ui_lang_get(STR_TOKEN)`.
- **Status:** FINAL.

### DEC-SW-05: OTA via HTTP + ArvanCloud S3
- **Decision:** OTA updates via HTTP pull from ArvanCloud S3 bucket `ota-plc`.
- **Alternatives Considered:** MQTT-based OTA, USB flashing only.
- **Reasoning:** Machines in customer factories have internet access. Remote OTA saves travel cost. ArvanCloud is Iranian CDN with low latency.
- **OTA validation:** 4-step (HTTP 200, Content-Length, SHA256, rollback test).
- **Status:** FINAL.

### DEC-SW-06: Yekan Font with Montserrat Fallback
- **Decision:** `ui_font_yekan_16/20/24` with Montserrat fallback chain for Latin characters.
- **Reasoning:** Yekan is the standard Persian UI font with industrial readability. Fallback prevents Tofu squares for English/Turkish characters.
- **Zero Tofu Mandate:** Turkish special chars (ü, ç, ı, ş, ğ, ö) must use ASCII equivalents in Yekan — use `Turkce`, `kasnak`, `sifirla` etc.
- **Status:** FINAL.

---

## CLOUD & NETWORK DECISIONS

### DEC-NET-01: Cloudflare Tunnel for Web Portal
- **Decision:** Use Cloudflare tunnel for `haffler-ai.ir` — no open inbound ports on server.
- **Reasoning:** Security (no exposed ports), free DDoS protection, zero SSL cert management.
- **Status:** FINAL.

### DEC-NET-02: Public HiveMQ MQTT Broker (Temporary)
- **Decision:** Use `broker.hivemq.com:1883` public broker for machine telemetry.
- **Alternatives Considered:** Private EMQX on Hetzner server, HiveMQ Cloud with auth.
- **Reasoning:** Zero cost, zero setup for MVP. Machine IDs are obscure.
- **Known Risk:** No authentication — telemetry publicly visible if topics discovered.
- **Future Plan:** Migrate to private authenticated broker in next major version.
- **Status:** TEMPORARY — do not build features that depend on this being permanent.

### DEC-NET-03: Day-Lock Leasing System via DS3231 + Challenge-Response
- **Decision:** Hardware-backed day-count with 5-digit challenge-response unlock codes.
- **Reasoning:** Prevents customers from bypassing installment payments by disconnecting machine from internet.
- **Security:** Challenge derived from MAC + DS3231 monotonic timestamp. Cannot be guessed without keygen tool.
- **Status:** FINAL — never weaken this mechanism.

---

## PROCESS DECISIONS

### DEC-PROC-01: Zero Desktop Policy
- **Decision:** No project file ever touches `C:\Users\Haffler\Desktop`.
- **Reasoning:** Desktop is production workspace — contaminating it with build artifacts causes confusion.
- **Status:** ABSOLUTE RULE.

### DEC-PROC-02: lastfile Format (8-Section Standard)
- **Decision:** OTA metadata file `lastfile` must have exactly 8 sections: `company`, `version`, `binary`, `description_fa`, `description_en`, `description_tr`, `inputs`, `outputs`.
- **Reasoning:** HMI OTA client parses exactly these 8 keys. Missing or extra keys cause parse failure.
- **Status:** FINAL — changing format requires simultaneous firmware update on all machines.

---

## ✍️ HOW TO ADD DECISIONS

When a major architectural decision is made:
1. Add entry with format `DEC-{CATEGORY}-{NN}: {Title}`
2. Document: alternatives considered, reasoning, known limitations, status.
3. Status options: FINAL | TEMPORARY | UNDER REVIEW | SUPERSEDED BY DEC-XX-YY
