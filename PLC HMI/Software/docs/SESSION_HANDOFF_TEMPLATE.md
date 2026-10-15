# Session Handoff Template — Haffler Projects
## قالب تحویل پایان سشن (پُر کردن این فایل قبل از خروج الزامی است)

> **AGENT MANDATE:** At the end of every session, fill this template and save it as
> `SESSION_HANDOFF_[DATE].md` in the project root. The next agent MUST read this first.

---

## Session Date: [YYYY-MM-DD]
## Project(s) Worked On: [نام پروژه‌ها]
## Agent: [Antigravity / Claude / Cursor / etc.]

---

## 1. WHAT WAS DONE (فهرست کارهای انجام‌شده)

| # | File Changed | Type | Description |
|---|---|---|---|
| 1 | `[path/file.c]` | MODIFY | [چه تغییری و چرا] |
| 2 | `[path/file.h]` | NEW | [توضیح] |
| 3 | `[path/file.md]` | DELETE | [دلیل حذف] |

## 2. WHAT WAS NOT DONE (کارهای ناتمام)

- [ ] [کار ناتمام ۱ — دلیل: ...]
- [ ] [کار ناتمام ۲ — دلیل: ...]

## 3. OPEN DECISIONS (تصمیماتی که هنوز گرفته نشده)

- **تصمیم ۱:** [سوال باز] — گزینه‌ها: [A] یا [B]. پیشنهاد: [A] به دلیل [...]
- **تصمیم ۲:** [...]

## 4. DISCOVERED TRAPS (تله‌های جدید که کشف شد)

> اگر تله‌ای پیدا کردی که در DYNAMIC_KNOWLEDGE_BASE.md نیست، اینجا بنویس و آنجا هم اضافه کن.

- **تله جدید:** [توضیح]
  - **محل:** [فایل و خط تقریبی]
  - **راه‌حل:** [...]
  - **DYNAMIC_KNOWLEDGE_BASE.md بروز شد؟** [ ] بله / [ ] خیر

## 5. GIT STATUS (وضعیت گیت)

```bash
# نتیجه git status --short را اینجا بچسبان:
[خروجی git status]
```

**Committed?** [ ] Yes — Commit hash: `[hash]` / [ ] No — Files pending: [لیست]

## 6. BUILD STATUS (وضعیت آخرین بیلد)

- **نتیجه بیلد:** [ ] PASS / [ ] FAIL / [ ] Not run
- **آخرین دستور اجرا شده:**
  ```
  [دستور]
  ```
- **خروجی یا خطا:**
  ```
  [نتیجه]
  ```

## 7. NEXT SESSION PRIORITIES (اولویت‌های سشن بعدی)

1. 🔴 [فوری‌ترین کار]
2. 🟠 [مهم]
3. 🟡 [می‌تواند صبر کند]

## 8. CONTEXT HINTS FOR NEXT AGENT (راهنمای سریع برای ایجنت بعدی)

> اینجا چیزی بنویس که ایجنت بعدی باید بداند و در مستندات نیست.

- [نکته ۱]
- [نکته ۲]

---

**⚠️ REMEMBER:** Next agent must read this file BEFORE reading any code.
Read order: SESSION_HANDOFF → PROJECT_SNAPSHOT → DYNAMIC_KNOWLEDGE_BASE → KNOWN_AUDIT_FINDINGS
