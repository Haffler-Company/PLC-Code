---
name: apple-hig-design
description: >-
  Apple Human Interface Guidelines craft for industrial Gas Filling UI/UX: clarity,
  deference, depth, typography, color, layout, symmetry, and anti-AI-slop checks.
  Use when designing, polishing, or reviewing HMI layouts, or when user mentions
  Apple design, HIG, زیبا، طراحی بی‌نظیر, or visual quality.
---

# Apple HIG Design Craft — Gas Filling HMI

برای قوانین عملیاتی و محدودیت‌های فریم‌ور حتماً مطالعه شود:
`docs/OPERATIONS.md` و `docs/REFERENCE.md`.

## اصول بنیادین (غیرقابل مذاکره)
1. **وضوح (Clarity):** اعداد ابعاد شیشه و زمان قهرمان صفحه هستند.
2. **احترام (Deference):** کروم و قاب‌ها نباید با اطلاعات اصلی رقابت کنند.
3. **عمق (Depth):** تفکیک بصری با فاصله‌گذاری و لایه‌بندی ملایم به جای خطوط و حاشیه‌های تیره ضخیم.
4. **تقارن و سازگاری (Symmetry & Consistency):** کانال شیشه ۱ و کانال شیشه ۲ کاملاً آینه‌ای و متقارن.
5. **بازخورد فوری (Feedback):** هر لمس باید پاسخ فوری (تغییر وضعیت یا بوق بازر) داشته باشد.

## ارگونومی لمس و چیدمان
- گرید استاندارد ۸ پیکسلی (تنظیمات ریز با ۴ پیکسل).
- اهداف لمسی $\ge 40\times 40\text{ px}$.
- دکمه Start: سبز امرالد `#2E7D32` (تکمیل‌کننده تزریق).
- دکمه Stop: قرمز استاندارد `#D32F2F` (ایمنی و توقف اضطراری).
- کارت‌ها: پس‌زمینه سفید `#FFFFFF` روی بدنه اسلیت/خاکستری روشن.

## چک‌لیست Anti-Slop
- [ ] نبود گرادیان‌های بنفش/نئونی هوش مصنوعی
- [ ] نبود انیمیشن‌های چرخشی یا زوم که باعث فریز LVGL شود
- [ ] تقارن دقیق فاصله ستون‌های ورودی شیشه ۱ و ۲
- [ ] عدم هم‌پوشانی واحدها (`cm`, `s`) با فیلدها و اعداد
