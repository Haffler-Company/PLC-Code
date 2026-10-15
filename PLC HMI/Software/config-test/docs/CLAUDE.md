# CLAUDE.md — Config_Plus_Test (نصاب HAFFLER HT-V3A)

**اول `CONFIG_TECHNICAL_KNOWLEDGE_FA.md` کنارِ همین فایل را کامل بخوان** — همه‌ی دانشِ فنی آنجاست.

خلاصه‌ی حیاتی:
- مسیر مخزن: `C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\config-test`
- پروژه: نصاب/لانچرِ OTA روی ESP32-S3 (HT-V3A). صفحه‌ی به‌روزرسانی = ردیفِ افقیِ کارت‌های عمودی؛ هر کارت یک برنامه‌ی ماشین (جوش/برش/گاز/…) که با دکمه‌ی نصب از `https://ota.haffler-infi.ir` دانلود/فلش می‌شود.
- UI = EEZ Studio + LVGL v9. **`screens.c` تولیدی است؛ دوباره generate نکن وگرنه دست‌بردها پاک می‌شوند.**
- بیلد/آپلود با **PlatformIO** (`pio.exe`) — نه arduino-cli:
  ```
  # بیلد (env پیش‌فرض ht_v3a)
  & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir "C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\config-test\firmware"

  # آپلود — دستگاه روی COM4 یا COM8 است (CP210x؛ شماره پورت می‌تواند عوض شود؛ با [System.IO.Ports.SerialPort]::GetPortNames() چک کن)
  & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload --upload-port COM4 --project-dir "C:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\config-test\firmware"
  ```
  - **COM3 دستگاه نیست** — پورت Intel AMT مادربرد است (دام رایج)؛ هرگز روی آن آپلود نشود.
  - فلشِ کامل سه‌ناحیه با esptool وقتی جدول پارتیشن خارجی لازم شد: `0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin` از `.pio/build/ht_v3a/` — جزئیات در `docs/REFERENCE.md`.
  - **قانون قطعی: آپلود فقط من میگم** — ایجنت تحت هیچ شرایطی حق اجرای خودکار آپلود یا فلش ندارد؛ آپلود منحصراً با دستور مستقیم کاربر.
- OTA فقط **HTTPS** (سرور http را به https ریدایرکت می‌کند) → `WiFiClientSecure.setInsecure()`.
- ۸ برنامه + مسیرهای هاست + نیاز سخت‌افزاری (MAX6675/MCP23017) و پایپ‌لاینِ عکس (`png_to_lvgl.py`، اندازه‌ی ۲۴۰) → در دانش فنی.
- رمزها: `123` منوی به‌روزرسانی، `0000` تست سخت‌افزار.
- کارهای شبکه فقط در `app_task`؛ نوشتن روی ویجت فقط در نخِ LVGL (`lvgl_task`/`ui_tick`).
