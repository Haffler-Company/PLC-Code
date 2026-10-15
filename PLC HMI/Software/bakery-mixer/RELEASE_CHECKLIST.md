# Coffee Mixer — Release Checklist
## چک‌لیست انتشار نسخه فریمور

---

## RELEASE CHECKLIST — Coffee Mixer Firmware v[X.Y.Z]

### Phase 1: Safety ✅
- [ ] GPIO32 initialized LOW before pinMode (motor not spinning at boot)
- [ ] Portafilter check (GPIO34) active before any motor start path
- [ ] Motor auto-cutoff after 30s continuous run (manual mode safety)
- [ ] WDT feed in all loops > 5s duration
- [ ] No `delay()` > 50ms in Core 1 loop() without WDT feed

### Phase 2: GT911 & Display ✅
- [ ] `0x814E` cleared on EVERY GT911 poll cycle (no exceptions)
- [ ] 9-pulse I2C recovery handler present for bus lockup
- [ ] Screensaver timer resets on both touch and WebApp commands
- [ ] Wake-touch discarded (not processed as button press)
- [ ] Dirty-driven display: all s_last_* comparisons in place

### Phase 3: Dual-Core Isolation ✅
- [ ] No TFT calls from Core 0 (webTask) — grep: `tft.` in webTask
- [ ] No WiFi calls from Core 1 — grep: `WiFi.` in loop()
- [ ] Cross-core data via `volatile` flags or FreeRTOS queue only

### Phase 4: EEPROM ✅
- [ ] No new settings inserted in middle of layout (only appended)
- [ ] EEPROM layout table in `Docs/02_FIRMWARE_AND_DUAL_CORE_ARCHITECTURE.md` updated
- [ ] EEPROM write deferred when motor active (`s_eeprom_write_pending` flag)

### Phase 5: WebApp ✅
- [ ] All REST endpoints tested via browser or curl
- [ ] WebSocket reconnect with backoff working
- [ ] SSID and AP password accessible via web settings page

### Phase 6: Build & Flash ✅
- [ ] Arduino IDE build: **0 errors, 0 warnings**
- [ ] Binary size within ESP32 flash partition limits
- [ ] Full factory reset tested: hold boot button 5s → all EEPROM cleared

### Phase 7: Documentation ✅
- [ ] `KNOWN_AUDIT_FINDINGS.md` — fixed bugs marked RESOLVED
- [ ] `DYNAMIC_KNOWLEDGE_BASE.md` — new traps documented
- [ ] `PROJECT_SNAPSHOT.md` version updated
- [ ] Git committed: `release: v[X.Y.Z] — [brief description]`

---

## Release Sign-Off
- **Version:** [X.Y.Z] | **Date:** [YYYY-MM-DD] | **Agent:** [name]
- **All checks passed:** [ ] YES
