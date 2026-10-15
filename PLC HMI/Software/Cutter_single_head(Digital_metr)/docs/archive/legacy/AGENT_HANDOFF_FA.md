# راهنمای تحویل به عامل بعدی — Metr & Cutter

این سند خلاصهٔ **اخلاق کار مالک پروژه**، **شرایط تست**، و **ابزار/افزونه‌هایی** است که برای همین مخزن ساخته شده‌اند. جزئیات اجرایی در skillهاست.

**پروفایل کامل مالک (شخصیت، اولویت، درس‌های تکراری):** `docs/OWNER_PROFILE_FA.md` — اول آن را بخوان.

| سند / مهارت | نقش |
|-------------|-----|
| `docs/OWNER_PROFILE_FA.md` | شناخت مالک + خط‌قرمز رفتاری |
| `.cursor/skills/metr-cutter-agent-handoff/SKILL.md` | اخلاق، Done، تست، ضدالگو |
| `.cursor/skills/metr-cutter-hmi/SKILL.md` | فریمور HMI / هنگ / لیست |
| `.cursor/skills/metr-cutter-build-flash/SKILL.md` | بیلد PlatformIO / COM4 / LVGL srcFilter |
| `.cursor/skills/metr-cutter-hmi-design/SKILL.md` | حلقه طراحی + قاضی + SHOT |
| `docs/INDEX_FA.md` | فهرست همهٔ اسناد در `docs/` |
| `docs/PROJECT_NOTES.md` | نکات فنی سریع |
| `docs/METR_CUTTER_TECHNICAL_KNOWLEDGE_FA.md` | دانش فنی زنده |
| `docs/SCENARIO_FA.md` | رفتار محصول روی ماشین |
| `docs/CHANGELOG_AND_BACKLOG_FA.md` | مانده و نسخه |
| `docs/SIMULATOR_FA.md` | شبیه‌ساز / SHOT |

## اخلاق و انتظار مالک (کوتاه)

منبع کامل‌تر: **`docs/OWNER_PROFILE_FA.md`**.

- **خط وحشتناک قرمز کیبرد USB:** بدون دستور صریح مالک با نام «کیبرد USB» هرگز `Task4.h` / `EspUsbHost` / LED-on-plug / mouse-reset-on-USB را لمس نکن (`.cursor/rules/06-usb-keyboard-redline.mdc`).
- پاسخ **فارسی، کوتاه، عملی**.
- باگ واضح → **درست کن، بیلد، در صورت نیاز آپلود، تأیید**؛ نیمه‌کاره نگذار (مخصوصاً هنگ UI / نشانگر جاگ / استاپ حرکت).
- اگر یک موضوع را چند بار گفت → هنوز خراب است؛ دفاع نکن؛ ریشه را عوض کن و فلش کن.
- ادعای لی‌اوت فقط با **SHOT واقعی** از برد؛ طراح HTML مرجع است نه اثبات.
- پیکسل‌دستور («N px به سمت …») را لفظاً همان جهت اجرا کن.
- ماشین برش واقعی است: پین/قطبیت/حرکت را بی‌اجازه عوض نکن؛ بعد از تغییر motion تست سخت‌افزاری را بنویس.
- بیلد کند / دکمهٔ Build خراب = اولویت رفع ابزار، نه حرف زدن طولانی.
- Commit فقط با درخواست صریح.

## شرایط تست حداقلی

1. منطق لیست: `python tools/list_ui_sim.py --test`
2. UI روی برد: پورت CP210x @ 115200، `dtr/rts=False`، صبر ~۲۰s بعد ریست، سپس `SCR*` + `SHOT` با `tools/hmi_screenshot.py`
3. بعد از آپلود اگر UI عوض نشد → **قطع برق** برد
4. حرکت: هوم، جوگ دو جهت، استاپ/E-Stop، در صورت ایمن یک برش خشک

**فلش امن:** روزمره `tools/fast_firmware.ps1 -Upload` (پورت خودکار + boot verify). هنگ بوت → `-FullFlash` (NVS پاک). OTA سرور = فقط `firmware.bin` اپ.

## افزونه‌ها و ابزار ساخته‌شده در پروژه

| ابزار | کاربرد |
|-------|--------|
| `.vscode/tasks.json` | دکمه‌های MC: Build / Fast Flash / Build+Upload / Monitor |
| `firmware/fast_flash.bat` | فقط فلش `firmware.bin` |
| `tools/hmi_screenshot.py` | عکس واقعی LCD |
| `tools/boot_verify.py` | بعد از فلش: CHK1 + T2 create |
| `tools/fast_firmware.ps1` | بیلد سریع + آپلود + verify + OTA copy |
| `tools/list_ui_sim.py` | سیم منطق لیست |
| `tools/designer_f1_shot.py` + `simulator/settings-f1-designer.html` | طراحی F1 بدون فلش |
| `libraries/lvgl/library.json` | فیلتر کامپایل باریک LVGL (osal را کامل حذف نکن) |
| `screenshot.h` + فرمان‌های سریال | دیباگ از راه دور بدون لمس اپراتور |

آینهٔ اختیاری بدون فاصله در مسیر: `C:\fwbuild\` — قبل از بیلد با درخت اصلی همگام کن؛ Tasks ورک‌اسپیس هنوز به `firmware/` داخل workspace اشاره می‌کنند.

## تعریف تمام‌شدن یک کار

- کد + (در صورت نیاز) فلش + تأیید متناسب نوع تغییر (جدول در skill handoff).
- دانش جدید را در `docs/METR_CUTTER_TECHNICAL_KNOWLEDGE_FA.md` و در صورت نیاز changelog بنویس.
