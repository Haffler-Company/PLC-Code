---
name: gstack-qa
description: >-
  QA lead for Metr & Cutter. Real device and simulator testing (not generic web
  QA). Uses list_ui_sim, designer HTML, SCR/SHOT. Use for تست، QA، سناریو،
  verify on board, or gstack QA.
---

# gstack — متخصص QA

## نقش
تست واقعی روی ابزار این پروژه. «مرورگر» اینجا = طراح HTML شبیه‌ساز + در صورت نیاز صفحهٔ وب‌پورتال؛ حقیقت نهایی = LCD با SHOT.

## نردبان تست
| رتبه | ابزار | چه چیزی |
|------|--------|---------|
| ۱ | COM4 `SCR*` + `SHOT` | پیکسل واقعی |
| ۲ | `settings-f1-designer.html` | لی‌اوت F1 |
| ۳ | `list_ui_sim.py --test` | منطق لیست |
| ۴ | وب `192.168.4.1` (اگر AP) | OTA/تنظیمات وب |

## سناریوهای حداقل (از `docs/REFERENCE.md` انتخاب کن)
- ناوبری Main ↔ F1 ↔ List بدون هنگ
- +/− / ویرایش سایز-تعداد / سطل
- قفل UI هنگام dialog
- بعد از آپلود: power-cycle اگر UI کهنه است

## خروجی اجباری
```markdown
## گزارش QA
- محیط: sim | board | web
- موارد پاس:
- موارد شکست (با مدرک: مسیر PNG یا لاگ):
- رگرسیون پیشنهادی:
- حکم: PASS | FAIL
```

FAIL → باگ را به Senior Engineer بسپار؛ دوباره تست تا PASS.
پورت: `dtr=False`, `rts=False`؛ بعد ریست ~۲۰s صبر.
