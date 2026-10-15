---
name: metr-cutter-hmi-design
description: >-
  Apple-inspired HMI design loop with multi-judge review and real SHOT feedback
  for Metr & Cutter 480x320 LVGL. Use when designing or refining main/list/F1 UI,
  comparing simulator/designer vs board screenshots, or when the user asks for
  design polish, Apple HIG style, agent loop, or judges.
---

# Metr & Cutter — HMI Design Loop

## Read first
1. `.cursor/skills/apple-hig-design/SKILL.md`
2. `docs/OPERATIONS.md`
3. `docs/REFERENCE.md`
4. `docs/WORKLOG.md`
6. `.cursor/rules/60-hmi-design-apple.mdc`
7. `.cursor/rules/70-agent-design-loop.mdc`

## Goal template
```
هدف: [واضح بنویس — مثلاً «لیست برش بدون هم‌پوشانی هدر، ۱۰ ردیف، حس اپل»]
```

## Loop (3–5 rounds) — verbatim workflow
1. نسخه اولیه را خوب و کامل بساز.
2. خودت آن را بررسی کن و تمام نقاط ضعف، مشکلات یا چیزهایی که می‌شود بهتر شود را لیست کن.
3. نسخه بهتر را بنویس و ضعف‌ها را درست کن.
4. دوباره بررسی کن و نقاط ضعف باقی‌مانده را لیست کن.
5. این کار را ۳ تا ۵ دور تکرار کن تا نتیجه خیلی خوب شود.

در نهایت فقط بهترین نسخه نهایی را بده.

## Judges checklist (each round)

Copy and fill:

```
دور N:
- J1 رنگ: PASS|FAIL — ...
- J2 چیدمان: PASS|FAIL — ...
- J3 لمس/سایز: PASS|FAIL — ...
- J4 سایه/آیکن: PASS|FAIL — ...
- J5 SHOT واقعیت: PASS|FAIL — فایل: artifacts/screenshots/...
Critical: [لیست یا هیچ]
```

Any Critical → fix and continue loop. Do not declare done.

## Feedback path (scan the real UI)
1. Design idea in simulator HTML when possible (`simulator/settings-f1-designer.html`).
2. `python tools/designer_f1_shot.py` or save user target PNG.
3. Implement in `firmware/include/app/ui_extras.h` (prefer over EEZ rewrite).
4. Build/upload COM4 when flashing is in scope.
5. Serial (dtr/rts false): `SCR1`/`SCR2`/`SCR3` then `SHOT` via `tools/hmi_screenshot.py` or inline python.
6. Read the PNG with the Read tool — visually judge; do not trust memory.
7. Compare to designer/target; list gaps; next round.

## Simulator ideas to propose when user designs
- Keep one 480x320 canvas; hero content first.
- Export coordinates on 8px grid.
- Separate HTML designers for Main and List (mirror F1).
- Half-opacity compare overlay: designer vs `board_*.png`.
- Touch-target outlines toggle (40px minimum).

## Hard constraints
- LVGL 9.4 only; no pin changes; outputs active-LOW safety unchanged.
- No `loadScreen` / bulk NVS from touch callbacks.
- Persian answers for the user; ship final version only after judges pass.
