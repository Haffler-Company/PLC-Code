# Haffler PLC Fleet — Release Checklist
## چک‌لیست انتشار نسخه (باید ۱۰۰٪ تیک بخورد)

> **AGENT MANDATE:** Before declaring any firmware version "ready for release",
> complete ALL items on this list. Missing even one item is a blocker.

---

## RELEASE CHECKLIST v[X.Y.Z] — Project: [نام ماشین]

### Phase 1: Code Quality ✅
- [ ] Safety linter passed: `python tools/haffler_safety_linter.py` → **0 violations**
- [ ] No `malloc()` calls in periodic loops (grep: `malloc` in `main/`)
- [ ] No `vTaskDelay()` in `actions_task` (PLC scan task)
- [ ] No hardcoded pin numbers outside `project_config.h`
- [ ] No hardcoded strings outside `ui_lang.h` (grep: Persian chars in `.c` files)
- [ ] All error codes documented in AGENTS.md error matrix

### Phase 2: LVGL / UI Quality ✅
- [ ] `shadow_width = 0` on all widgets (grep: `shadow_width` ≠ 0)
- [ ] All interactive targets ≥ 44px (or `lv_obj_set_ext_click_area` applied)
- [ ] Dirty-driven updates in all `tick_screen_*` functions (no unconditional `lv_label_set_text_fmt`)
- [ ] Language switch tested: FA → EN → TR → FA — zero string leaks
- [ ] All new STR_ tokens have all 3 translations (FA + EN + TR)
- [ ] Zero Tofu: No Turkish special chars (ü ç ı ş ğ ö) in Yekan font — ASCII equivalents used
- [ ] BiDi numbers tested: Time display not reversed in Persian mode

### Phase 3: Safety & Interlocks ✅
- [ ] E-Stop latches correctly (requires manual reset — ISO 13850)
- [ ] All relay outputs initialized HIGH (OFF) before `gpio_set_direction()`
- [ ] Anti-short-cycle timer intact for compressor (if applicable)
- [ ] Cylinder freeze protection unmodified (if applicable)
- [ ] Watchdog timeout appropriate for scan cycle
- [ ] `plc_outputs_all_off()` called on every fault/E-Stop path

### Phase 4: Day-Lock / Leasing ✅
- [ ] Day-lock subsystem unmodified (or change documented in DECISION_LOG.md)
- [ ] Challenge-response algorithm untouched
- [ ] Soft-lock and hard-lock behaviors verified

### Phase 5: OTA Package ✅
- [ ] `idf.py build` completes with **zero warnings** (treat warnings as errors)
- [ ] `firmware.bin` size within partition limits
- [ ] `lastfile` has exactly 8 sections: `company`, `version`, `binary`, `description_fa`, `description_en`, `description_tr`, `inputs`, `outputs`
- [ ] `BZ` (buzzer) NOT listed in `inputs`/`outputs` — only `M0..M7` auxiliary pins
- [ ] Rollup changelog includes all changes since last deployed version
- [ ] SHA256 of `firmware.bin` computed and recorded

### Phase 6: Cloud Deployment ✅
- [ ] `firmware.bin` uploaded to `ota-plc` bucket correct path
- [ ] `lastfile` uploaded to same path
- [ ] Both files have `public-read` ACL
- [ ] HTTP 200 verified: `curl -I https://ota.haffler-infi.ir/[path]/firmware.bin`
- [ ] Content-Length matches local file size
- [ ] SHA256 verified on downloaded file matches local
- [ ] HMI OTA update triggered on test device → successfully updated → correct version shown

### Phase 7: Documentation ✅
- [ ] `CHANGELOG.md` updated with new version entry
- [ ] `KNOWN_AUDIT_FINDINGS.md` — any fixed bugs marked RESOLVED
- [ ] `DYNAMIC_KNOWLEDGE_BASE.md` — any new traps documented
- [ ] `PROJECT_SNAPSHOT.md` version number updated
- [ ] Git committed with message: `release: v[X.Y.Z] — [brief description]`

---

## Release Sign-Off

- **Version:** [X.Y.Z]
- **Date:** [YYYY-MM-DD]
- **Agent:** [name]
- **All checks passed:** [ ] YES
- **OTA URL:** `https://ota.haffler-infi.ir/[category]/[machine-slug]/firmware.bin`
