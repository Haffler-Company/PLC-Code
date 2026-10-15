---
name: gas-filling-agent-handoff
description: >-
  Agent-to-agent session handoff protocol for Gas Filling HMI: clean diffs,
  evidence ledger, verified hardware state, and worklog synchronization.
---

# Gas Filling Agent Handoff Skill

## الزامات تحویل تسک (Handoff Requirements)
هر ایجنت پس از اتمام کار موظف است گزارش تحویل فشرده را با این ساختار ارائه کند:

1. **فایل‌های تغییریافته:** لیست دقیق فایل‌های ویرایش‌شده بدون فایل‌های تصادفی.
2. **سطح شواهد (Evidence Level):** تفکیک شفاف بین بازبینی سورس‌کد، بیلد موفق، تست سریال PING/STAT، و اسکرین‌شات واقعی LCD.
3. **موارد باقی‌مانده / تاییدنشده:** هرگونه تست فیزیکی که نیازمند اپراتور است (مانند لمس فیزیکی یا بررسی خروجی شیرها).
4. **به‌روزرسانی WORKLOG:** ثبت وضعیت تسک در `docs/WORKLOG.md` و آزادسازی قفل‌های ویرایش.
