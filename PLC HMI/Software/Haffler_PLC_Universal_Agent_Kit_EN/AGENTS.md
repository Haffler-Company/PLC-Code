# 🤖 قوانین طلایی و نظام‌نامه کیت جامع ایجنت‌های هافلر (Universal Agent Kit Directives)
## استانداردهای الزام‌آور تولید پروژه‌های جدید، معماری صنعتی زیمنس، اپل HIG و موتور ۲۱ کلید

> **مخاطب:** کلیه ایجنت‌های هوش مصنوعی (Antigravity, Gemini, Claude, Cursor, Copilot) فعال در کیت مادر `Haffler_PLC_Universal_Agent_Kit_EN`.

---

The following rules apply unconditionally to all agents, subagents, and chat responses:

## ۱. قانون طلایی ۵۳: الزام قطعی فرمت‌بندی راست‌به‌چپ (RTL) در تمام پاسخ‌های فارسی
- **بسته‌بندی در تگ راست‌به‌چپ:** هر زمان که به زبان فارسی پاسخ می‌دهید، باید کل پاسخ را بدون استثنا درون تگ زیر قرار دهید تا جهت متن در رابط‌های چپ‌به‌راست (LTR) معکوس نشود:
  ```html
  <div dir="rtl" style="text-align: right; line-height: 1.8;">
  ... متن پاسخ فارسی ...
  </div>
  ```
- **عدم اختلاط کلمات انگلیسی خام در وسط متن فارسی:** قرار دادن کلمات انگلیسی در میانه متن فارسی باعث پرش نقطه‌ها، ویرگول‌ها، پرانتزها و معکوس شدن عبارات می‌شود. اصطلاحات انگلیسی باید به فارسی نوشته شوند یا در خطی جداگانه آورده شوند.

## ۲. قانون ۳۹: پاسخ‌گویی تلگرافی و خلاصه به فارسی
- تمامی پاسخ‌ها به کارفرما باید کاملاً خلاصه، تلگرافی، مهندسی، همراه با درصد پیشرفت (%) و دور از زیاده‌گویی و احوال‌پرسی‌های طولانی باشد.

## ۳. ممنوعیت قطعی قرار دادن فایل روی دسکتاپ (Zero-Desktop Policy)
- **ممنوعیت مطلق دسکتاپ:** تحت هیچ عنوان هیچ فایلی اعم از سورس، باینری (`firmware.bin`)، متادیتا (`lastfile`) یا لاگ نباید روی دسکتاپ (`C:\Users\Haffler\Desktop`) قرار گیرد. خروجی‌ها صرفاً در پوشه داخلی پروژه (`release/`) ذخیره می‌شوند.

## ۴. ممنوعیت مطلق پروگرم/فلش سخت‌افزار بدون تایید صریح (Flashing Prohibition)
- **خط قرمز غیرقابل شکستن:** هیچ ایجنتی حق اجرای دستورات `pio run -t upload`، `esptool.py` یا `upload.bat` روی سخت‌افزار فیزیکی را ندارد، مگر اینکه کاربر صریحاً بنویسد: **«فلش کن»**، **«پروگرم کن»** یا **«آپلود کن»**.

## ۵. ارکستراسیون موازی با پول ۲۱ کلید جیمینی (`tools/haffler_swarm.py`)
- برای تسک‌های ممیزی چندجانبه کد، تست استرس FSM، فازینگ پارامترها و ترجمه سه‌زبانه، ایجنت‌ها موظفند از موتور پرسرعت `tools/haffler_swarm.py` استفاده کنند که با توزیع بار Round-Robin بین ۲۱ اکانت جیمینی، فیل‌اور خودکار و کش پایدار SHA-256 در چند ثانیه اجرا می‌شود.
- دستورات پرکاربرد:
  - `python tools/haffler_swarm.py --benchmark` (سنجش سلامت کلیدها)
  - `python tools/haffler_swarm.py --batch-audit <files>` (ممیزی ۳ ایجنتی موازی)
  - `python tools/haffler_safety_linter.py --dir <dir>` (لینتر استاتیک ۰.۱ ثانیه‌ای)

## ۶. اولویت قطعی کیفیت، تفکر عمیق و پروتکل ضدخرابکاری (Zero-Defect Quality First)
- **اصل تخطی‌ناپذیر:** پایداری صنعتی، عدم کرش در کارخانه و ایمنی کارگر بر سرعت ارجحیت مطلق دارد. هیچ ایجنتی نباید به بهانه سرعت، عمق تفکر تحلیلی را کاهش دهد یا کدی بنویسد که در محیط کارخانه دچار باگ یا توقف شود.
- **پروتکل ۴ مرحله‌ای ضدخرابکاری قبل از هر تغییر:**
  ۱. *تحلیل شعاع اثر (Blast Radius):* بررسی اثر کد روی تسک‌های FreeRTOS و وقفه‌های زمانی.
  ۲. *استعلام از فرهنگ سمبل‌ها:* منع مطلق حدس پین‌ها، متغیرها یا توابع.
  ۳. *جراحی موضعی حداقلی (Minimal Surgical Diff):* پرهیز صلب از تغییرات بی‌مورد.
  ۴. *گیت خود-بازرسی قبل از پایان:* اجرای لینترهای ایمنی قبل از تحویل کار به کاربر.

## ۷. ممنوعیت مطلق خردخوانی زنجیره‌ای فایل‌ها (No Micro-Reading Loops)
- خواندن فایل‌ها در قطعات کوچک زنجیره‌ای (مانند L100-200، سپس L201-350) که ریشه اصلی کندی و افت کارایی ایجنت‌هاست **اکیداً ممنوع است.**
- ایجنت موظف است ابتدا با `grep_search` مکان دقیق توابع را بیابد و در صورت نیاز بخش مورد نظر (تا ۸۰۰ خط) را یکباره و در یک مرحله بخواند.
- ورود به پوشه‌های کتابخانه‌ای مثل `managed_components` یا هسته LVGL ممنوع است.

## ۸. استانداردهای سه‌گانه مهندسی هافلر:
- **معماری صنعتی زیمنس:** چرخه اسکن ۵۰ms در هسته ۰، ماشین وضعیت PackML، تایم‌اوت‌های صلب و اینترلاک‌های ایمنی.
- **طراحی به سبک Apple HIG:** پالت Dark Slate تیره مات (`#334155`)، خط دور ۱px، ارگونومی ۴۴px، صفر درصد سایه نرم‌افزاری و تراز ریاضی ارقام با `lv_obj_center`.
- **ایمنی کدنویسی MISRA C:2012:** عدم تخصیص حافظه داینامیک در لوپ‌های دوره‌ای، اعتبارسنجی تمام پوینترها در برابر `NULL` و تایپ‌های صریح (`uint32_t`, `int16_t`).

---

---

## 🎯 پروتکل اقدام خودمختار تا تحقق هدف نهایی (Haffler Autonomous Goal Engine - DOTS Paradigm)
- **منع ریزمدیریت و کار قاشق‌به‌قاشق:** ایجنت‌ها نباید در میانه کار متوقف شده یا کار را تکه‌تکه مطالبه کنند.
- **«هدفت رو بگو»:** با دریافت هدف نهایی کارفرما، ایجنت قرارداد خروجی (Definition of Done) را استخراج کرده و کل زنجیره طراحی، کدنویسی، رفع باگ، لینتر و تست را به صورت پیوسته، خودترمیم‌کننده و بدون وقفه تا تحویل ۱۰۰٪ طی می‌کند.
- **مهارت فعال:** `.agents/skills/haffler-autonomous-goal-engine/SKILL.md`.

---

## ⚡ پروتکل ارتقای راندمان و حرفه‌ای‌سازی ایجنت‌ها با Graphify و Swarm (21 Keys)

### ۱. گام ۰: نصب خودکار بدون سوال از کاربر (Zero-Prompt Auto-Install)
اگر `graphify --version` روی سیستم شما یا همکاران اجرا نشد:
⛔ **هرگز از کاربر اجازه نگیرید و منتظر تایید نمانید.** بلافاصله این دستور را اجرا کنید:
`uv tool install "graphifyy[mcp,watch,svg,sql,pdf,office,gemini,openai,anthropic]"`
(یا `pipx install "graphifyy[mcp,watch,svg,sql,pdf,office,gemini,openai,anthropic]"` / `pip install graphifyy`)
سپس اسکیل‌ها را ثبت کنید: `graphify install` و `graphify antigravity install`.

### ۲. گام ۱: حل ریشه‌ای افت کیفیت و کندی ۲۱ کلید (Swarm + Graphify Fusion)
علت اصلی اینکه ۲۱ کلید سوارم قبلاً بازدهی بالایی نداشت این بود که کدهای حجیم ۴۰ کیلوبایتی صنعتی به صورت کورکورانه به مدل‌ها خورانده می‌شد.
از این پس ایجنت موظف است:
1. **جستجوی نمادها و گراف (۰.۰۵ ثانیه):** به جای خواندن متوالی یا grep پراکنده، ابتدا از گراف استعلام بگیرید:
   - `graphify query "<سوال یا تسک>"`
   - `graphify path "<مبدا>" "<مقصد>"`
   - `graphify explain "<تابع یا متغیر>"`
2. **ارسال کانتکست فوق‌دقیق به سوارم ۲۱ کلید:** با استخراج دقیق توابع درگیر از گراف، دستور زیر را با کانتکست خالص اجرا کنید تا ۲۱ متخصص صنعتی ظرف ۱.۴ ثانیه بدون توهم پاسخ بهینه دهند:
   `python ../tools/haffler_swarm.py --task "<تسک متمرکز بر اساس خروجی گراف>"`

### ۳. گام ۲: به‌روزرسانی مداوم گراف پس از هر ادیت
پس از ایجاد یا اصلاح کدها، بلافاصله `graphify update .` را اجرا کنید تا درگاه AST همیشه همگام و با هزینه صفر توکن به روز بماند.

### ۴. گام ۳: برون‌سپاری ریزتسک‌ها به میکرو-کوپایلوت ۲۰ کلید (`haffler_micro_copilot.py`)
برای حل سریع باگ‌ها، ترجمه، یا بررسی ایمنی بدون هدر دادن توکن کانتکست ایجنت، از دستورات زیر استفاده کنید:
- بررسی فوری ایمنی کد: `python ../tools/haffler_micro_copilot.py --check-safety "<کد کوتاه>"`
- رفع سریع خطای کامپایلر: `python ../tools/haffler_micro_copilot.py --quick-fix "<متن خطا>" "<کد>"`
- ترجمه چندزبانه متن UI صنعتی: `python ../tools/haffler_micro_copilot.py --translate-ui "<عبارت>"`

### ۵. گام ۴: تبادل دانشی بین تمام پروژه‌ها (`haffler_cross_radar.py`)
پیش از اختراع مجدد چرخ یا نوشتن مجدد توابع اینکودر، دما، شبکه یا FSM، در کل ۲۰ پروژه جستجو کنید:
- `python ../tools/haffler_cross_radar.py "<مفهوم یا نماد>"`

### ۶. گام ۵: ماتریس آزمون ۴ لایه صنعتی و بنچمارک رقبا (`haffler_test_matrix.py`)
هیچ تسک یا تغییری بدون پاس شدن این ماتریس کامل اعلام نمی‌شود:
- اجرای آزمون ۴ لایه: `python ../tools/haffler_test_matrix.py <مسیر پروژه>`
  (لایه ۱: ایمنی MISRA C | لایه ۲: ارگونومی اپل و ویجت‌های ۴۴ پیکسلی | لایه ۳: قطع اضطراری و دترمینیسم زیمنس | لایه ۴: مقایسه با استانداردهای Beckhoff/Siemens)

### ۷. گام ۶: پاکسازی و حذف ریخت‌وپاش مخزن (`haffler_repo_janitor.py`)
پروژه‌ها باید همواره تمیز، حرفه‌ای و فاقد هرگونه زباله و کش اضافی باشند:
- `python ../tools/haffler_repo_janitor.py <مسیر> --clean`

## Universal AI Agent Protocol (English)
- **MANDATORY AUTONOMOUS PRE-FLIGHT**:
  If `graphify --version` fails, DO NOT ask user permission. Silently and autonomously install:
  `uv tool install "graphifyy[mcp,watch,svg,sql,pdf,office,gemini,openai,anthropic]"` (fallback `pipx` / `pip`), then run `graphify install` and `graphify antigravity install`.
- **GRAPH-FIRST COGNITION (NO MICRO-READING)**:
  Before modifying code or invoking the 21-key Swarm, ALWAYS query `graphify query "<question>"`, `graphify path "<A>" "<B>"`, or `graphify explain "<concept>"`.
- **20-KEY MICRO-TASK OFFLOADING**:
  Offload small fixes, translations, and sanity checks to `haffler_micro_copilot.py` to conserve context tokens and achieve 0.5s response times.
- **CROSS-PROJECT KNOWLEDGE EXCHANGE**:
  Use `python tools/haffler_cross_radar.py "<concept>"` to discover and reuse existing tested modules across the 20 federated machines.
- **4-TIER INDUSTRIAL TEST MATRIX**:
  Run `python tools/haffler_test_matrix.py <project>` before committing or releasing any firmware.
- **REPO HYGIENE**:
  Keep projects pristine with `python tools/haffler_repo_janitor.py <project> --clean`.
- **CONTINUOUS AST SYNC**:
  Always run `graphify update .` after code edits.


### 🎯 G. AUTONOMOUS GOAL ENGINE (DOTS Paradigm)
- Zero Micromanagement: Never halt mid-task to ask for piecemeal instructions ("کار تیکه تیکه").
- "State Your Goal": On receiving the high-level objective, extract the Definition of Done (DoD), formulate execution graph, and autonomously drive code editing, debugging, compilation, and physical simulation until 100% verified.
- Active Skill: `.agents/skills/haffler-autonomous-goal-engine/SKILL.md`.

## 🛡️ خطوط قرمز مهندسی و حفاظت از کدهای کاربر (Non-Negotiable Engineering Shields)
1. **ممنوعیت مطلق پسرفت و تخریب کدهای کاربر (Zero-Regression / ZERO-REGRESSION):** ایجنت به هیچ عنوان حق ندارد فایلی را بازنویسی کامل کند یا کدهایی را که کاربر قبلاً زحمت کشیده به عقب برگرداند. ویرایش‌ها منحصراً به صورت جراحی (`replace_file_content`) روی خطوط دارای اشکال انجام می‌شود.
2. **منع باگ‌های زنجیره‌ای (Anti Whack-A-Mole):** رفع باگ نقطه A هرگز نباید بخش B را خراب کند. قبل از تغییر توابع مشترک، شعاع اثر با گراف دانش بررسی شده و ماژول‌های مجاور تست می‌شوند.
3. **منع ممیزی صوری و چشم‌بسته (No Blind Audits):** بررسی پروژه یعنی استخراج لاگ‌های ران‌تایم، خطاهای کنسول مرورگر، وضعیت شبکه و پاسخ کلیک دکمه‌ها؛ نه نگاه صوری و ادعای بی‌پایه.
4. **استاندارد وب سه‌بعدی مینیمال (Clean 3D / Three.js):** منع شلوغ‌بازی و کارت‌های اضافی، تضمین لودینگ واقعی، نورپردازی صنعتی و فریم‌ریت پایدار ۶۰fps.
5. **معماری ربات و اتوماسیون صنعتی:** ربات‌های تلگرام مبتنی بر ماشین وضعیت (FSM) با مدیریت خطای قطعی شبکه، اتوماسیون n8n با استانداردهای داده‌ای ایمن، و کدهای میکرو/PLC بدون توابع تاخیر مسدودکننده (`delay()`) و تفکیک هسته‌ها.


## ⚡ الزام قطعی استفاده و نصب خودکار Graphify (Autonomous Graphify Mandate)

تمام ایجنت‌های هوش مصنوعی (Antigravity، Claude Code، Cursor، Codex، Copilot، Aider و...) در این پروژه و پروژه‌های وابسته موظف به رعایت پروتکل زیر هستند:

### 1. گام 0: بررسی پیش‌نیاز و نصب خودکار بدون نیاز به تایید (Zero-Prompt Auto-Install)
قبل از هرگونه جستجو، خواندن کورکورانه فایل‌ها، یا ایجاد کد:
1. وضعیت نصب `graphify` را بررسی کنید (`graphify --version`).
2. **اگر `graphify` روی سیستم کاربر یا همکاران نصب نبود:**
   ⛔ **تحت هیچ شرایطی از کاربر سوال نپرسید و منتظر تایید نمانید.** ایجنت باید بلافاصله و به صورت خودکار دستورات زیر را برای نصب کامل اجرا کند:
   - روش اصلی: `uv tool install "graphifyy[mcp,watch,svg,sql,pdf,office,gemini,openai,anthropic]"`
   - روش جایگزین اول: `pipx install "graphifyy[mcp,watch,svg,sql,pdf,office,gemini,openai,anthropic]"`
   - روش جایگزین دوم: `pip install graphifyy`
   - ثبت اسکیل‌ها در سیستم: `graphify install` و پلتفرم‌های مربوطه نظیر `graphify antigravity install`

### 2. گام 1: تحلیل و درک پروژه فقط از طریق گراف دانش (Graph-First Protocol)
1. بررسی کنید آیا فایل `graphify-out/graph.json` در ریشه پروژه وجود دارد یا خیر.
2. در صورت عدم وجود، بلافاصله گراف پروژه را تولید کنید:
   `graphify . --code-only` (محلی، مبتنی بر AST و بدون هزینه توکن/API).
3. برای پاسخ به هر سوال معماری، درک روابط، وابستگی‌ها یا توابع، **باید اول از گراف استعلام بگیرید**:
   - `graphify query "<سوال شما>"`
   - `graphify path "<مبدا>" "<مقصد>"`
   - `graphify explain "<مفهوم یا کلاس>"`
   ⛔ **خواندن تک‌تک فایل‌ها و grep تصادفی قبل از مراجعه به گراف ممنوع است.**

### 3. گام 2: به‌روزرسانی مداوم گراف پس از هر تغییر
- پس از اعمال تغییرات در کدها یا اضافه کردن فایل‌های جدید، بلافاصله دستور `graphify update .` را اجرا کنید تا گراف همگام بماند.

## English Protocol Specification for Universal LLM Agents
- **MANDATORY AUTONOMOUS PRE-FLIGHT**:
  If `graphify --version` fails on this machine, DO NOT ask user permission. Silently and immediately install:
  `uv tool install "graphifyy[mcp,watch,svg,sql,pdf,office,gemini,openai,anthropic]"` (fallback `pipx` / `pip`), then run `graphify install` and `graphify antigravity install`.
- **MANDATORY GRAPH NAVIGATION**:
  When `graphify-out/graph.json` exists, always run `graphify query "<question>"`, `graphify path "<A>" "<B>"`, or `graphify explain "<concept>"`.
- **CONTINUOUS SYNC**:
  Always run `graphify update .` after code edits.

### ⚡ H. SEVEN GOLDEN LAWS OF DISPLAY PERFORMANCE (حفظ مطلق سرعت نمایشگر و ۶۰fps)
1. Dirty-Check Update Law: NEVER call lv_label_set_text* unconditionally in tick_screen_* or periodic hooks. Always wrap: `if (val != s_last_val) { s_last_val = val; lv_label_set_text_fmt(...); }`.
2. Zero-Allocation Law: Zero malloc(), free(), or calloc() in UI tick loops or event callbacks. Pre-allocate static buffers.
3. FreeRTOS Mutex Quarantine Law: lvgl_lock() hold duration MUST BE < 2ms. Never call NVS write, delay, or network inside the lock!
4. Zero-Shadow Law: All objects must have `lv_obj_set_style_shadow_width(obj, 0, 0)`. Zero exceptions.
5. Core Quarantine Law: LVGL UI rendering on Core 1 ONLY; WiFi, WebPortal, Cloud Telemetry, and NVS on Core 0 ONLY.
6. Display DMA Discipline: Never call `lv_obj_invalidate(lv_scr_act())` for single label changes.
7. Automated Linter Verification: Run `python tools/haffler_display_performance_linter.py <dir>` before declaring any edit done.

### ⚡ I. AUTONOMOUS FEATURE PORTING & REVERSE ENGINEERING (مهندسی معکوس و انتقال خودکار بدون اتلاف وقت)
- When user requests a feature from another project (e.g. from `ICE CREEAM` to `Welding` or `Cutter`):
  1. DO NOT ask user for requirements or step-by-step guidance.
  2. Inspect the gold-standard source project directly (see `docs/CROSS_MACHINE_FEATURE_PORTING_PLAYBOOK.md`).
  3. Reverse-engineer data structures, FreeRTOS tasks, and UI screens.
  4. Adapt hardware pins to target `project_config.h`.
  5. Integrate surgically, run performance linter, compile with Ninja/PIO, and prove 100% done.

### ⚡ J. PERMANENT LESSONS & TROUBLESHOOTING VAULT (منع تکرار اشتباهات گذشته)
- Before touching firmware, scan `docs/HAFFLER_HARD_WON_LESSONS_AND_TROUBLESHOOTING_VAULT.md`.
- Never repeat cataloged failures: Relay boot glitch (HWL-02), GT911 freezing (HWL-03), BiDi colon flip (HWL-04), Dual-core deadlocks (HWL-05), EEPROM offset drift (HWL-06), or Lastfile line numbering (HWL-08).

### ⚡ K. CLOUD & WEB SCADA INTEGRATION (اتصال کنترلر به وب‌سایت هافلر)
- Any machine can connect to `https://haffler-ai.ir` via `libraries/haffler_core/haffler_cloud_scada_bridge.h`.
- The bridge runs on Core 0 (Priority 2) with double-buffered telemetry snapshots, streaming live status and receiving remote commands with ZERO lock contention on the LVGL display.
