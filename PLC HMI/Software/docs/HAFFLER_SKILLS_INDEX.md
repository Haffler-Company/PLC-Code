# دانشنامه جامع و فهرست کاتالوگ مهارت‌های هافلر (Haffler Skills Catalog & On-Demand Index)

این سند مرجع واحد حقیقت (Single Source of Truth) برای دسترسی لحظه‌ای (On-Demand) ایجنت‌های هوش مصنوعی به مهارت‌ها، کتابخانه‌ها و ابزارهای توسعه در تمامی پروژه‌های شرکت هافلر است.

---

## ۱. مهارت جامع مادر (همیشه در حافظه - Tier 1)

| نام مهارت | مسیر فایل | کاربرد و شروط فراخوانی (Trigger) |
| :--- | :--- | :--- |
| **`haffler`** | [`.agents/skills/haffler/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/haffler/SKILL.md) | **مهارت مادر و همیشگی:** تجمیع زیمنس، Apple HIG، اصول MISRA C:2012، قفل اقساط Day-Lock، سه‌زبانه، انتشار ابری، و ارکستراسیون ۲۱ کلیده. |

---

## ۲. موتور موازی و ارکستراسیون چند ایجنتی (Multi-Agent Swarm)

| نام ابزار | مسیر فایل | نحوه فراخوانی و کاربرد |
| :--- | :--- | :--- |
| **`haffler_swarm.py`** | [`tools/haffler_swarm.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_swarm.py) | **موتور موازی ۲۱ کلیده v2.5:** اجرای هم‌زمان ممیزی کد، فازینگ و کش هوشمند SHA-256. |
| **`haffler_safety_linter.py`** | [`tools/haffler_safety_linter.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_safety_linter.py) | **لینتر ایمنی صنعتی و اپل:** چک خودکار ۰.۱ ثانیه‌ای پوینترها، عدم وجود سایه و malloc. |
| **`haffler_string_extractor.py`** | [`tools/haffler_string_extractor.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_string_extractor.py) | **استخراج‌کننده سه‌زبانه:** استخراج متون هاردکد شده جهت ریشه‌کنی کامل کاراکترهای مربعی. |
| **`haffler_fault_fuzzer.py`** | [`tools/haffler_fault_fuzzer.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_fault_fuzzer.py) | **فازر سناریوهای سخت‌افزاری:** رادار وارونگی BiDi، برخورد فاز و بریدگی خط کرسی فونت. |
| **`haffler-autonomous-chief-engineer`** | [`.agents/skills/haffler-autonomous-chief-engineer/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/haffler-autonomous-chief-engineer/SKILL.md) | **مدیر ارشد مهندسی:** رهبری ارکستراسیون، دید ۳۶۰ درجه، پیکسلی اپل و دترمینیسم زیمنس. |
| **`haffler-hardware-scenario-fuzzer`** | [`.agents/skills/haffler-hardware-scenario-fuzzer/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/haffler-hardware-scenario-fuzzer/SKILL.md) | ممیز و فازر سناریوهای بحرانی، وارونگی ارقام BiDi و افت ولتاژ سخت‌افزار. |
| **`haffler-interlock-guardian`** | [`.agents/skills/haffler-interlock-guardian/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/haffler-interlock-guardian/SKILL.md) | پایش و تحمیل صلب اینترلاک‌های ایمنی فیزیکی بر اساس اطلس جامع. |
| **`haffler-cross-machine-porter`** | [`.agents/skills/haffler-cross-machine-porter/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/haffler-cross-machine-porter/SKILL.md) | انتقال هوشمند و بدون بازنویسی قابلیت‌ها بین ۱۱ پروژه هافلر. |
| **`haffler-safety-linter`** | [`.agents/skills/haffler-safety-linter/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/haffler-safety-linter/SKILL.md) | ممیزی پیش از تحویل کد بر پایه قوانین زیمنس و اپل HIG. |
| **`haffler-zero-tofu-multilingual`** | [`.agents/skills/haffler-zero-tofu-multilingual/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/haffler-zero-tofu-multilingual/SKILL.md) | تضمین صفر درصد کاراکتر مربعی و نشت زبان در فارسی، انگلیسی و ترکی. |
| **`agent-orchestrator`** | [`.agents/skills/agent-orchestrator/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/agent-orchestrator/SKILL.md) | تفکیک تسک‌ها بر اساس مالکیت فایل، جلوگیری از تداخل و همگام‌سازی موازی. |
| **`haffler_prompt_pro.py`** | [`tools/haffler_prompt_pro.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_prompt_pro.py) | **باگ‌گیر و معمار پرامپت ۲۱ کلیده:** تبدیل آنی (~۱.۲s) خواسته‌های خام کارفرما به پرامپت مهندسی ضدباگ و کپسول‌های عملیاتی با خطوط قرمز و گیت اعتبارسنجی. |
| **`prompt-architect-pro`** | [`.agents/skills/prompt-architect-pro/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/prompt-architect-pro/SKILL.md) | **مهارت مهندسی و باگ‌گیری پرامپت:** کشف ۵ باگ پنهان در دستورات، شاخ و برگ دادن فنی، مشخص‌سازی فایل‌های هدف و تعریف تست قطعی قبل از شروع کار ایجنت. |
| **`strict-hmi-firmware-judge`**| [`.agents/skills/strict-hmi-firmware-judge/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/strict-hmi-firmware-judge/SKILL.md) | ممیزی بی‌رحمانه کد در برابر چک‌لیست‌های ایمنی قبل از تحویل. |
| **`task-observer`** | [`.agents/skills/task-observer/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/task-observer/SKILL.md) | نظارت بر پیشرفت کار و پیشگیری از انحراف ایجنت‌ها. |

---

## ۳. مهارت‌های طراحی و گرافیک صنعتی (LVGL & Apple HIG)

| نام مهارت | مسیر مرجع | کاربرد و شروط فراخوانی |
| :--- | :--- | :--- |
| **`apple-industrial-hmi-design`** | [`docs/STANDARDS_APPLE_HIG_HMI.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_APPLE_HIG_HMI.md) | استانداردهای رنگ تیره، خطوط ۱px، پدینگ صفر کانتینر و تراز ریاضی. |
| **`apple-lvgl-component-pack`** | [`docs/APPLE_HIG_LVGL_COMPONENT_LIBRARY.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/APPLE_HIG_LVGL_COMPONENT_LIBRARY.md) | کتابخانه کدهای آماده ویجت‌های اپل، کیپد لمسی، سلکتورها و بنرها. |
| **`lvgl-zero-lag-engine`** | [`.agents/skills/lvgl-zero-lag-engine/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/lvgl-zero-lag-engine/SKILL.md) | بهینه‌سازی رندرینگ، معماری Dirty-driven و جلوگیری از افت فریم. |
| **`complete-hmi-screen`** | [`.agents/skills/complete-hmi-screen/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/complete-hmi-screen/SKILL.md) | متدولوژی ساخت صفر تا صد صفحات HMI استاندارد. |
| **`safe-layout-refinement`** | [`.agents/skills/safe-layout-refinement/SKILL.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/.agents/skills/safe-layout-refinement/SKILL.md) | اصلاح پدینگ و فواصل بدون ریسک به‌هم‌ریختگی ویجت‌های همسایه. |

---

## ۴. مهارت‌های لاجیک، سخت‌افزار و کنترل صنعتی (Siemens & Embedded)

| نام مهارت | مسیر مرجع | کاربرد و شروط فراخوانی |
| :--- | :--- | :--- |
| **`siemens-industrial-hardening`** | [`docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/STANDARDS_SIEMENS_INDUSTRIAL_PLC.md) | ماشین حالت PackML، اسکن ۵۰ms، رله‌های Active-LOW و هندلینگ خطا. |
| **`misra-zero-defect-c`** | [`docs/MISRA_C_INDUSTRIAL_SAFETY_RULES.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/MISRA_C_INDUSTRIAL_SAFETY_RULES.md) | قوانین ایمنی حافظه، عدم استفاده از malloc در لوپ و انواع صریح داده. |
| **`haffler-modbus-and-motion-control`** | [`docs/INDUSTRIAL_COMMUNICATIONS_AND_FIELDBUS.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/INDUSTRIAL_COMMUNICATIONS_AND_FIELDBUS.md) | پروتکل RS485 مالبوس، درایورهای پالس انکودر و اره‌های دیجیتال. |
| **`haffler-hardware-scenario-fuzzer`** | [`docs/CRASH_RESILIENCE_AND_RECOVERY_GUIDE.md`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/docs/CRASH_RESILIENCE_AND_RECOVERY_GUIDE.md) | فازینگ و شبیه‌سازی قطعی برق ناگهانی، نویز سنسورها و افت ولتاژ. |

---

## ۵. زیرسیستم‌های اختصاصی و سرور ابری هافلر

| نام زیرسیستم | مسیر ابزار / سند | شرح عملکرد |
| :--- | :--- | :--- |
| **پابلیشر ابری آروان‌کلاد** | [`tools/haffler_ota_publisher.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_ota_publisher.py) | استقرار مستقیم فریمور روی باکت `ota-plc` با اعتبارسنجی آنلاین. |
| **تولیدکننده کلید قفل اقساط** | [`tools/haffler_day_lock_keygen.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/haffler_day_lock_keygen.py) | تولید کدهای بازگشایی چالش ۵ رقمی برای اقساط دستگاه. |
| **اعتبارسنجی آفلاین فریمور** | [`tools/verify_firmware.py`](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/tools/verify_firmware.py) | تست آفلاین پایتون برای بررسی ساختار کدهای C و رجیستری زبان. |

---

## ۶. اطلس ۱۱ ماشین هافلر (جهت انتقال دانش بین پروژه‌ها)

هرگاه کارفرما ویژگی یا کدی از یک ماشین را خواست، مستقیماً از پوشه‌های زیر الگوبرداری کنید:

1. **`ICE CREEAM`:** بستنی‌ساز صنعتی (کنترل دو کمپرسور، پمپ رینبو، سنسور دور پولی، ترموکوپل‌های MAX6675).
2. **`Welding_single_head` / `Welding_double_head`:** دستگاه‌های جوش UPVC (کنترل اتوی داغ، فک‌های پنیوماتیک، زمان‌سنجی دقیق جوش).
3. **`Cutter_single_head(Digital_metr)` / `Cutter double_head(Digital_metr)`:** اره‌های برش آلومینیوم و UPVC (متر دیجیتال، انکودر نوری، تصحیح لقی/بک‌لش).
4. **`Bakery` / `bakery-mixer`:** ماشین‌آلات پخت و خمیرگیر صنعتی (موتور دور متغیر، تایمرهای دوزمانه، اینترلاک ایمنی).
5. **`Copy_router`:** فرز کپی و شیارزن دستگیره UPVC (جک‌های نگه‌دارنده، موتور اسپیندل پرسرعت).
6. **`Corner_cleaning`:** تمیزکننده گوشه جوش (تیغه‌های تراش هم‌زمان بالا و پایین، سنسورهای موقعیت).
7. **`Gas-filling`:** دستگاه تزریق گاز آرگون شیشه دوجداره (فلومتر دقیق، پایش درصد خلوص، تست فشار).
8. **`Aluminum-punching`:** پرس و پانچ پروفیل آلومینیوم (شیرهای برقی هیدرولیک، پدال دوکاره، حفاظ ایمنی).

---

## ۷. مهارت‌های نامرتبط و حذف‌شده از چرخه پردازش (Ignored Junk)
مهارت‌های زیر به دلیل عدم ارتباط با برنامه‌نویسی امبدد و PLC، به طور کامل از چرخه هوش مصنوعی هافلر کنار گذاشته شدند تا هیچ حافظه‌ای اشغال نکنند:
- کلیه مهارت‌های بیولوژی، پزشکی و ژنتیک: `alphafold_*`, `alphagenome_*`, `chembl_*`, `clinical_trials_*`, `clinvar_*`, `dbsnp_*`, `embl_*`, `encode_*`, `ensembl_*`, `foldseek_*`, `gnomad_*`, `human_protein_*`, `interpro_*`, `jaspar_*`, `ncbi_*`, `openfda_*`, `opentargets_*`, `pdb_*`, `protein_*`, `pubchem_*`, `pubmed_*`, `pymol`, `quickgo_*`, `reactome_*`, `string_*`, `unibind_*`, `uniprot_*`.
- کلیه مهارت‌های کلود دیتابیس سنگین غیرامبدد: `bigquery_*`, `bigtable_*`, `dbt_*`, `dataform_*`, `lakehouse_*`.
