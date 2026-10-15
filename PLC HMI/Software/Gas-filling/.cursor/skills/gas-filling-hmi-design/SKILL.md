---
name: gas-filling-hmi-design
description: >-
  Visual design and layout refinement skill for Gas Filling HMI: Apple HIG
  craftsmanship, 5-judge loop, real LCD screenshot verification, and pixel perfection.
---

# Gas Filling HMI Design Skill

## گردش کار اصلاحات ظاهری (Design Workflow)
1. **بررسی شات قبلی:** اسکرین‌شات موجود در `artifacts/shots/` را بررسی کن.
2. **اعمال تغییرات پیکسلی:** موقعیت فیلدها و لیبل‌ها را بر اساس گرید ۸ پیکسلی تنظیم کن.
3. **حفظ تقارن:** هر تغییری در کانال سمت راست (شیشه ۱) باید به صورت آینه‌ای در کانال سمت چپ (شیشه ۲) پیاده‌سازی شود.
4. **تست روی نمایشگر:** پس از بیلد و آپلود، فرمان `SHOT` را اجرا کرده و تطابق بصری را تایید کن.

## استانداردهای رنگی و ابعادی
- اندازه دکمه‌های Start و Stop: دقیقاً ۹۲ در ۴۴ پیکسل.
- فیلدهای ابعاد (Length, Width, Height): فیلدهای متنی سفید با عرض مناسب برای جای‌گیری واحد `cm` در داخل فیلد بدون هم‌پوشانی.
- نوارهای پیشرفت عمودی: پر شدن روان از ۰ تا ۱۰۰ درصد با تراز عمودی دقیق.
