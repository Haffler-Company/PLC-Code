# مرکز اسناد و راهنمای جامع مهندسی (Engineering Documentation Index)
**پروژه:** HMI/PLC دستگاه جوش تک‌سر پروفیل UPVC هافلر (Single-Head UPVC Welder)  
**معماری:** ESP32-S3 + LVGL 9.4 + FreeRTOS Dual-Core + EEZ Flow + WebApp  
**استاندارد حاکم:** Siemens Industrial Determinism & Apple HIG Craftsmanship  

---

## 🗺️ نقشه راه و ساختار جامع مستندات فنی

کلیه مستندات فنی، معماری، سخت‌افزاری و عملیاتی پروژه در این پوشه بر اساس استانداردهای صنعتی در ۶ شاخه تخصصی طبقه‌بندی شده‌اند:

```
docs/
├── README.md                           # همین فایل: ایندکس و نقشه راه کل مستندات
│
├── 🏗️ معماری و ماشین‌های حالت (Core Architecture & State Machines)
│   ├── START.md                        # نقطه ورود اول ایجنت‌ها و برنامه‌نویسان (Quickstart & Router)
│   ├── REFERENCE.md                    # مشخصات پلتفرم، معماری سیستم، حافظه و نگاشت پایه
│   ├── ARCHITECTURE_DECISIONS.md       # سوابق تصمیمات معماری (ADR) و چرایی انتخاب الگوها
│   ├── INTER_FILE_API_CONTRACT.md      # قراردادهای دقیق جریان داده و API بین فایل‌های برنامه
│   ├── CHANGE_IMPACT_MATRIX.md         # ماتریس تحلیل اثر تغییرات و زنجیره‌های خطای آبشاری
│   └── WELDING_CYCLE_FSM.md            # تشریح کامل ۱۴ گام ماشین حالت جوش، اینترلاک‌ها و E-Stop
│
├── ⚡ سخت‌افزار، سیگنال‌ها و نقشه‌خوانی (Hardware, I/O & Pinouts)
│   ├── HARDWARE_IO_PINOUT.md           # مستند جامع جدول پایه‌ها و سیگنال‌های کنترلر HAFFLER ETV3A
│   └── pins.txt                        # نگاشت خام ترمینال‌های سخت‌افزاری (سازگار با ابزارهای ریلیز)
│
├── 🖥️ رابط کاربری و استانداردهای نمایش (UI & Apple HIG Craftsmanship)
│   ├── UI_SCREEN_INVENTORY.md          # شناسنامه و فهرست صفحات، شناسه‌های EEZ و لایه‌بندی مودال‌ها
│   ├── SCREENSHOT_TOOL_FA.md           # راهنمای موتور ثبت تصویر فریم‌بافر PSRAM بدون لگ UI
│   ├── AI_CHATBOT_AND_SCREENSAVER_SPEC_FA.md # معماری چت‌بات صوتی صنعتی و فیزیک اسکرین‌سیور
│   └── OPERATOR_VOCABULARY.md          # واژه‌نامه تخصصی اصطلاحات کارگاهی و اپراتوری صنعت UPVC
│
├── 🛡️ پروتکل‌های امنیتی، ارتباط سریال و رمزنگاری (Industrial Security & Serial)
│   └── SERIAL_SECURITY_PROTOCOL.md     # پروتکل فریم‌های امضاشده HMAC-SHA256 ($WELD) و دستورات امنیتی
│
├── 🚀 استانداردهای عملیاتی و تضمین کیفیت (Operations & Siemens Rigor)
│   ├── AGENT_PLAYBOOK_FA.md            # نظام‌نامه جامع اجرایی و راهنمای عملیاتی ارکان ۹‌گانه
│   ├── OPERATIONS.md                   # سیاست‌ها، فرامین عملیاتی و خطوط قرمز مهندسی
│   ├── ACCEPTANCE_TEST_SPEC.md         # ۵ سطح تست پذیرش، طبقه‌بندی نقص‌ها و معیار Definition of DONE
│   ├── GIT_CHECKPOINT_PROTOCOL.md      # پروتکل بازگردانی جراحی (Surgical Rollback) و بکاپ‌های چرخشی
│   ├── RELEASE_AND_VERSIONING_STANDARD.md # استاندارد بسته‌بندی، نسخه و ریلیز کامل فریم‌ور
│   ├── KNOWN_ISSUES_AND_DEBT.md        # فهرست بدهی‌های فنی شناخته‌شده و ورک‌اروندهای مجاز
│   └── PROMPT_EXAMPLES_LIBRARY.md      # کتابخانه نمونه پرامپت‌ها و روش بسط صنعتی آن‌ها
│
└── 📜 سوابق، لاگ‌ها و دانشنامه (Logs, Knowledge Base & Audits)
    ├── WORKLOG.md                      # دفترچه سوابق مهندسی، تغییرات بیلدها و قفل‌های فعال
    ├── WELDING_HMI_KNOWLEDGE_FA.md     # دانشنامه و مرجع فنی جامع سیستم (بیش از ۲۵۰۰ خط)
    └── CODE_REVIEW_2026-08-29.md       # گزارش ممیزی فنی سورس‌کد در تاریخ ۲۹ آگوست ۲۰۲۶
```

---

## 📌 راهنمای استفاده بر اساس نقش و نیاز

| اگر می‌خواهید... | مستند هدف |
|---|---|
| سریع‌ترین راه ورود به کد و شروع کار را بدانید | [`START.md`](START.md) |
| اتصالات برق، پین‌ها، شیرهای پنوماتیک و سنسورها را ببینید | [`HARDWARE_IO_PINOUT.md`](HARDWARE_IO_PINOUT.md) |
| ترتیب گام‌های جوش، ذوب، خنک‌کاری و اینترلاک‌ها را بدانید | [`WELDING_CYCLE_FSM.md`](WELDING_CYCLE_FSM.md) |
| تغییر چندفایله ایجاد کنید و از خطای آبشاری پرهیز کنید | [`CHANGE_IMPACT_MATRIX.md`](CHANGE_IMPACT_MATRIX.md) و [`INTER_FILE_API_CONTRACT.md`](INTER_FILE_API_CONTRACT.md) |
| با صفحات UI و ویجت‌های EEZ Studio کار کنید | [`UI_SCREEN_INVENTORY.md`](UI_SCREEN_INVENTORY.md) |
| دستورات سریال و مانیتورینگ امن با پایتون اجرا کنید | [`SERIAL_SECURITY_PROTOCOL.md`](SERIAL_SECURITY_PROTOCOL.md) |
| قوانین بکاپ‌گیری و میکروکامیت گیت را بررسی کنید | [`GIT_CHECKPOINT_PROTOCOL.md`](GIT_CHECKPOINT_PROTOCOL.md) |
| آخرین تغییرات، لاگ‌ها و وضعیت قفل‌ها را چک کنید | [`WORKLOG.md`](WORKLOG.md) |
| اصطلاحات کارگاهی (فک، شابلون، لنگه، چهارچوب) را بفهمید | [`OPERATOR_VOCABULARY.md`](OPERATOR_VOCABULARY.md) |
