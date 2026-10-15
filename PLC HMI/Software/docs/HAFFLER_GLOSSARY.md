# Haffler Master Glossary — فرهنگ لغت جامع هافلر
## Persian Workshop Terms + Technical Definitions + Code Symbols

> **AGENT MANDATE:** When user uses any term from Column 1, execute Column 3 exactly.
> Never guess meaning of Persian workshop vocabulary.

---

## 🗣️ Persian Workshop → Technical Translation

| کارفرما می‌گوید | معنی فنی | ایجنت چه می‌کند |
|---|---|---|
| **«خفنش کن»** | Apple HIG premium styling | Dark Slate `#334155`, 1px border, zero shadows (`shadow_width=0`), 44px ergonomics, smooth transitions |
| **«صنعتیش کن»** | Siemens industrial hardening | PackML FSM, Core 1 isolation, hard timeouts, ISA-18.2 alarms, MISRA C types |
| **«گیر میکنه»** / **«هنگ میکنه»** | Timeout / freeze / deadlock | Check: GT911 0x814E, `lvgl_lock()` deadlock, I2C bus lockup, WDT timeout, Core cross-call |
| **«لقی داره»** | Backlash / timing inaccuracy | Encoder/pulley backlash correction, debounce tightening, unilateral compensation |
| **«فایل برای سرور بده»** / **«پکیج سرور»** | OTA cloud deployment | Build → `lastfile` → ArvanCloud `ota-plc` → 4-step verification. NEVER desktop |
| **«همش درست کن»** | Full audit + fix | Launch 3-agent swarm: `haffler_swarm.py --audit` across all files |
| **«سریع‌تر کن»** | Performance optimization | Dirty-driven LVGL, `shadow_width=0`, DMA double-buffer, reduce heap alloc |
| **«خرابکاری نشه»** | Zero regression | Blast radius analysis + ANTI_PATTERNS.md check + self-audit before done |
| **«پروگرم کن»** / **«فلش کن»** / **«آپلود کن»** | Flash firmware to hardware | ONLY these exact words authorize flashing. Nothing else. |
| **«بررسی کن»** | Analyze + report | Read files, run grep, check docs. Do NOT change any code. |
| **«اصلاح کن»** | Fix the specific issue | Surgical minimal fix only. No refactoring beyond scope. |
| **«از اول بساز»** | Create from scratch | Use scaffold_new_machine.py template + PROMPT_FOR_NEW_PROJECT_FA.md |
| **«مثل اون یکی ماشین»** | Port from another machine | Cross-project intelligence: scan other project → extract pattern → adapt + port |
| **«چک کن»** | Verify current state | git status + read relevant files. No changes. |
| **«مستند کن»** | Add documentation | Write to appropriate .md file. No code changes. |
| **«صفر خطا»** | Zero defect | MISRA C + no NULL dereference + bounded loops + no malloc in loop |
| **«کرش میکنه»** | Crash / panic reset | Check: Core panic, stack overflow, NULL pointer, heap corruption, WDT |
| **«دما درست نیست»** | Thermocouple reading error | Check: MAX6675 CS pin, SPI timing (170ms min between reads), E04/E05 fault codes |
| **«کمپرسور روشن نمیشه»** | Compressor won't start | Check: anti-short-cycle timer (180s), fault latch, overload input, relay wiring |
| **«موتور قطع میشه»** | Motor stops unexpectedly | Check: inductive sensor stall (E01), overload relay (E02), fault latch, pulley stall |

---

## 🔧 Technical Symbol Glossary

### Output Pins (Relay — Active LOW for PLC, Active HIGH for Coffee Mixer)
| Symbol | Machine | Function |
|---|---|---|
| `Q0` | ICE CREAM | Small compressor (milk tank) |
| `Q1` | ICE CREAM | Large compressor (cylinder) — anti-short-cycle 180s |
| `Q2` | ICE CREAM | Paddle motor (stirrer) |
| `Q3` | ICE CREAM | Cream pump (edge coloring) |
| `Q4` | ICE CREAM | Rainbow pump (gel injection) |
| `Q5` | ICE CREAM | Air valve (edge coloring SSR) |
| `M0..M7` | All PLC | Auxiliary bidirectional pins (shown in lastfile maps) |
| `BZ` | All PLC | Buzzer — **NEVER shown in lastfile pin maps** |
| `GPIO32` | Coffee Mixer | Motor relay — **Active HIGH** (opposite of PLC!) |

### Input Pins
| Symbol | Machine | Function |
|---|---|---|
| `I0` | ICE CREAM | Inductive sensor (pulley RPM) |
| `I2` | ICE CREAM | Rear microswitch (clutch) |
| `I4` | ICE CREAM | Front lever microswitch (cone counter) |
| `I6` | ICE CREAM | Mushroom E-Stop button |
| `I7` | ICE CREAM | Start button |
| `GPIO34` | Coffee Mixer | Portafilter microswitch (Active LOW, input-only, no pull-up) |

### State Machine States (PackML)
| State | Code | Description |
|---|---|---|
| `STATE_STOPPED` | 0 | Machine idle, all outputs OFF |
| `STATE_IDLE` | 1 | Ready to start, outputs pre-energized |
| `STATE_STARTING` | 2 | Startup sequence in progress |
| `STATE_EXECUTE` | 3 | Normal production running |
| `STATE_STOPPING` | 4 | Shutdown sequence |
| `STATE_FAULT` | 5 | Fault active, all outputs OFF |
| `STATE_WASHING` | 6 | Wash/clean cycle (ICE CREAM) |

### Error Codes (ISA-18.2)
| Code | FAULT_* | Trigger | Safe Action |
|---|---|---|---|
| E00 | FAULT_EMERGENCY_STOP | E-Stop button | All outputs OFF <20ms |
| E01 | FAULT_INDUCTIVE_STALL | No pulley pulse >4s | Stop paddle + compressors |
| E02 | FAULT_MOTOR_OVERLOAD | Load relay trip | Stop paddle + compressors |
| E03 | FAULT_COMP_OVERLOAD | Compressor overload trip | Stop large compressor |
| E04 | FAULT_THERMO_TOP_DISCONNECT | MAX6675 T1 open circuit | Stop small compressor |
| E05 | FAULT_THERMO_CYL_DISCONNECT | MAX6675 T2 open circuit | Switch to timed control |
| E07 | FAULT_LEASE_EXPIRED | Day-lock countdown = 0 | Soft lock (no new cycles) |
| E12 | FAULT_BROWNOUT | Voltage drop detected | Auto-recover on power restore |
| E13 | FAULT_WATCHDOG_RESET | Task WDT triggered | Seamless screen recovery |
| E14 | FAULT_CORE_PANIC | ESP32 core panic | Anti-loop crash recovery |

---

## 🌐 OTA & Cloud Terms

| Term | Definition |
|---|---|
| `lastfile` | OTA metadata file with exactly 8 sections. Parsed by HMI OTA client. |
| `ota-plc` | ArvanCloud S3 bucket name for all firmware files |
| `ota.haffler-infi.ir` | CDN domain serving OTA bucket content |
| `haffler-ai.ir` | Production web portal domain (Hetzner server) |
| Rollup Changelog | Cumulative change log shown on HMI during OTA (all versions since installed) |
| Anti-Tamper | DS3231 monotonic check preventing RTC rollback to bypass day-lock |

---

## 🎨 UI Color Palette Reference

| Name | Hex | Usage |
|---|---|---|
| Dark Slate (Primary BG) | `#334155` | Main button background, card backgrounds |
| Apple Blue (Action) | `#0284c7` | Confirm/save buttons, active elements |
| Industrial Orange | `#EA580C` | Warning buttons, industrial accent |
| Orange Border | `#C2410C` | Border for industrial orange buttons |
| Input BG | `#F1F5F9` | Input field backgrounds |
| Input Border | `#CBD5E1` | Input field borders |
| Input Text | `#0F172A` | Input field text |
| T1 Temp (Top Tank) | `#38bdf8` | Ice cream top tank temperature capsule |
| T2 Temp (Cylinder) | `#0284c7` | Cylinder freezing temperature capsule |
| Web Light BG | `#ffffff` | Haffler_WEB portal background |
| Web Text | `#0f172a` | Haffler_WEB body text |

---

## 📐 Hardware Architecture Terms

| Term | Definition |
|---|---|
| Active-LOW | Output = 0 (LOW) means device is ON. All PLC relays use this (fail-safe). |
| Active-HIGH | Output = 1 (HIGH) means device is ON. Coffee Mixer motor uses this. |
| Pull-Up | Internal resistor keeping pin HIGH when floating. All PLC inputs use this. |
| PSRAM | External 8MB RAM on ESP32-S3. Used for LVGL frame buffers. NOT available on standard ESP32. |
| DMA | Direct Memory Access. Used for LCD transfer without CPU involvement. |
| BiDi | Bidirectional text. Persian RTL mixed with English LTR causes digit reversal if not isolated. |
| Tofu (مربع) | Square character ▯ displayed when font lacks the glyph. Zero-Tofu = no squares. |
| PackML | ISA-88 standard state machine for packaging machines. Haffler uses: Stopped→Idle→Starting→Execute→Stopping→Fault |
| MISRA C | Motor Industry Software Reliability Association C coding standard. Requires explicit types, NULL checks, bounded loops. |
