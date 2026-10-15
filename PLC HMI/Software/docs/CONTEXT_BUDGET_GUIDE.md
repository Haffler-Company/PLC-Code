# Haffler Projects — Context Budget Guide
## راهنمای ایجنت: در هر نوع وظیفه چه فایل‌هایی بخوانیم

> **AGENT MANDATE:** Match your task to the category below. Read ONLY the listed files.
> Reading more than needed wastes context and slows the session 10x.
> Rule: grep_search first → read exact range → never read full large files.

---

## 🔴 EMERGENCY / SAFETY TASKS
*«دستگاه خاموش میشه» / «کرش میکنه» / «موتور قطع نمیشه»*

**Read in this order (stop when you have enough):**
1. `DYNAMIC_KNOWLEDGE_BASE.md` — hardware traps (always first)
2. `KNOWN_AUDIT_FINDINGS.md` — known open bugs
3. `docs/MASTER_SAFETY_INTERLOCK_ATLAS.md` — safety red lines
4. grep `FAULT_` in `main/` → read ±50 lines around the fault handler
5. `project_config.h` — pin definitions only

**DO NOT read:** Full `screens.c`, full `ui_lang.c`, unrelated components.

---

## 🖥️ UI / HMI TASKS
*«صفحه جدید بساز» / «رنگ عوض کن» / «تراز درست کن»*

**Read in this order:**
1. `PROJECT_SNAPSHOT.md` — which screen files exist
2. `main/ui/styles.c` — color palette and widget styles
3. grep `screen_name` in `main/ui/screens.c` → read that screen's function only
4. `main/ui/ui_lang.h` — token list (for string changes)
5. `docs/STANDARDS_APPLE_HIG_HMI.md` — if unfamiliar with design rules

**DO NOT read:** PLC logic files (`actions.c`, `plc_*.c`), full `ui_lang.c` unless adding new string.

---

## 🌐 LANGUAGE / MULTILINGUAL TASKS
*«ترجمه اضافه کن» / «نشت زبان درست کن»*

**Read in this order:**
1. `main/ui/ui_lang.h` — token list (grep for existing similar token first)
2. grep leaked string in `main/ui/screens.c` → find where it's set
3. `HAFFLER_GLOSSARY.md` — Turkish ASCII equivalents table
4. `main/ui/ui_lang.c` → only the `s_dict[]` array section (grep `s_dict`)

**DO NOT read:** Hardware files, PLC logic, full screens.c.

---

## ⚙️ PLC LOGIC TASKS
*«تایم‌اوت عوض کن» / «خطای جدید اضافه کن» / «منطق کمپرسور درست کن»*

**Read in this order:**
1. `project_config.h` — all timing constants and pin definitions
2. grep `FAULT_NAME` or `STATE_` in `main/plc_*.c` → read ±100 lines
3. `docs/MASTER_SAFETY_INTERLOCK_ATLAS.md` — interlock dependencies
4. `AGENTS.md` error matrix table — for new fault codes

**DO NOT read:** UI files, font files, OTA files unless they interact with the fault.

---

## 🚀 OTA / RELEASE TASKS
*«پکیج سرور بده» / «نسخه جدید آماده کن»*

**Read in this order:**
1. `RELEASE_CHECKLIST.md` — follow this exactly
2. `docs/STANDARDS_HAFFLER_OTA_RELEASE_AND_ROLLUP_CHANGELOG.md` — lastfile format
3. `docs/STANDARDS_HAFFLER_OTA_SERVER_DEPLOYMENT.md` — upload procedure
4. Previous `lastfile` in `release/` folder — for rollup changelog

**DO NOT read:** Source code files unless there's a build error.

---

## 🔒 SECURITY / DAY-LOCK TASKS
*«قفل اقساط» / «کد بازگشایی» / «امنیت»*

**Read in this order:**
1. `main/day_lock_core.h` — interface only (not implementation)
2. `DECISION_LOG.md` — DEC related to day-lock
3. `ANTI_PATTERNS.md` — any anti-patterns about weakening the system
4. `tools/haffler_day_lock_keygen.py` — key generation logic (read-only)

**DO NOT read:** UI files, OTA files. Do NOT modify algorithm without explicit «امنیتش کن» instruction.

---

## 🌐 HAFFLER_WEB SPECIFIC

### Backend Bug Fix
1. `DYNAMIC_KNOWLEDGE_BASE.md` → `KNOWN_AUDIT_FINDINGS.md`
2. grep exact function/class name in `main.py` → read ±80 lines
3. `docs/DECISION_LOG.md` — check if fix approach was already rejected

### Frontend / Dashboard Change
1. grep HTML element ID or JS function in `index.html` → read ±100 lines only
2. `docs/AGENT_PROMPT_TEMPLATES.md` PROMPT-WEB-04 for correct approach
3. **NEVER** read index.html from line 1

### MQTT / Fleet Change
1. `docs/20_HAFFLER_11_MACHINES_FLEET_AND_CLOUD_SCADA_SPEC.md` — topic schema
2. grep `haffler/` in `main.py` → find subscription/publish points
3. `DECISION_LOG.md` DEC-INF-03 — MQTT is temporary, do not over-engineer

---

## ☕ COFFEE MIXER SPECIFIC

### Display / Touch Task
1. grep function name in `functions.h` using grep_search (97KB — never read top-to-bottom)
2. `settings.h` — pin definitions
3. `DYNAMIC_KNOWLEDGE_BASE.md` TRAP-01 (GT911)

### Motor / Safety Task
1. `DYNAMIC_KNOWLEDGE_BASE.md` TRAP-02 (Active HIGH)
2. grep `GPIO32` or `motor` in `functions.h`
3. `KNOWN_AUDIT_FINDINGS.md` AUDIT-01, AUDIT-03

### Web Portal Task
1. grep endpoint path in `functions.h` → read that handler only
2. `DYNAMIC_KNOWLEDGE_BASE.md` PERF-03 (WebSocket throttling)

---

## 📏 File Size Reference (Always Use grep First)

| File | Size | Rule |
|---|---|---|
| `index.html` (Haffler_WEB) | 1.6MB | grep only — never read sequentially |
| `main.py` (Haffler_WEB) | ~180KB | grep only |
| `agent.py` (Haffler_WEB) | ~180KB | grep only |
| `functions.h` (Coffee Mixer) | 97KB | grep only |
| `screens.c` (PLC machines) | ~60-120KB | grep screen name first |
| `ui_lang.c` (PLC machines) | ~30-50KB | grep token or read `s_dict[]` section only |
| `project_config.h` (PLC) | ~5KB | safe to read fully |
| `DYNAMIC_KNOWLEDGE_BASE.md` | ~10KB | always read fully at session start |
