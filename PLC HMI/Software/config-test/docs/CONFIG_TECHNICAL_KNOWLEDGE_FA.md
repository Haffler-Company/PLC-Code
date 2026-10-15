# دانش فنی پروژه‌ی Config_Plus_Test (نصاب/لانچر HAFFLER HT-V3A)

> این فایل برای شروعِ هر چتِ جدید نوشته شده. اگر چیزی نمی‌دانی، **اول این را کامل بخوان**.
> آخرین به‌روزرسانی: 2026-06-30.
>
> 🚀 **سرعت/روان‌سازیِ UI:** برای هر کارِ مربوط به لگ، اسکرول، fps و رندر، فایلِ جداگانه‌ی
> [`CONFIG_PERFORMANCE_KNOWLEDGE_FA.md`](CONFIG_PERFORMANCE_KNOWLEDGE_FA.md) را بخوان (شخصی‌سازی‌شده‌ی دانشِ جوش + درس‌های برش، با اعدادِ واقعیِ همین پروژه).

---

## ۱) این پروژه چیست؟

برنامه‌ی **نصاب/لانچر** روی نمایشگرِ صنعتیِ **HAFFLER | HT-V3A** (برد ESP32-S3).
کاربر صفحه‌ی «به‌روزرسانی» را باز می‌کند، کارتِ هر دستگاه (جوش، برش، تزریق گاز، …) را می‌بیند و با دکمه‌ی **نصب** آن برنامه را از اینترنت (OTA) دانلود و روی همین برد فلش می‌کند.

- مسیر سورس (بیلد): `D:\SOFT WARE PLC HMI\Config_Plus_Test\V.1.0.0\config_hwtest\` — سورس اصلی `config_hwtest.ino`.
- پروژه‌ی EEZ Studio (مولّدِ UI): `D:\SOFT WARE PLC HMI\Config Project\V.1.0.0\Frameware\frameware_App.eez-project`.
- نسخه‌ی خودِ Config: `defines.h` → `#define VERSION "V1.0.0"`.

---

## ۲) سخت‌افزار و پین‌ها (از `defines.h` و `hwtest.cpp`)

| بخش | پین/آدرس |
|---|---|
| نمایشگر | SPI: MISO=11, MOSI=10, SCLK=12, CS=13, DC=14, RST=-1, BL=-1. ۴۸۰×۳۲۰، rotation=3 |
| تاچ (bb_captouch) | SDA=8, SCL=9, INT=40, RST=39, orientation=270، آدرس I2C `0x5D` |
| بازر | GPIO45 (PWM) |
| ترموکوپل MAX6675 | SO=37, SCK=47, **CS1=21، CS2=46** (دو عدد) — کتابخانه `max6675.h` (CS2 قبلاً ۰ بود) |
| MCP23017 | I2C (8/9)، آدرس `0x20..0x27` |
| RTC DS3231 | I2C آدرس `0x68` |
| RS485 | DE=46, Serial2 = RX18/TX17, 9600 |
| رله‌ها (محیط تست) | `pins[10] = {16,42,39,48,40,15,35,36,38,41}` |
| ورودی ADC (تست) | `adcpins[7] = {1,2,3,4,5,6,7}` |

نکته: GPIO39/40 با خطِ تاچ مشترک‌اند؛ فقط در محیط تست خروجی می‌شوند.

---

## ۳) ساختار نرم‌افزار

UI با **EEZ Studio + LVGL v9**. دو UIِ موازی:
1. **Config** (صفحه‌ی نصب/به‌روزرسانی): `screens.c/h`, `ui.c/h`, `images.*`, `vars.*`, `actions.h`, `fonts.h`.
2. **تست سخت‌افزار** (پیشوند `t_`): `t_screens.c`, `t_ui.*`, `t_vars.*` + `hwtest.cpp/h`. فقط با رمز `0000`.

### نخ‌ها (RTOS) — `tasks.h`
- `lv_tick_task`(هسته۱)، `lvgl_task`(هسته۱: `lv_timer_handler`+`ui_tick`؛ کارهای نخِ LVGL: نوشتن روی لیبل/arc/سازگاری)، `app_task`(هسته۰: وای‌فای، خواندن نسخه‌ها، دانلود OTA؛ کارهای بلاک‌کننده‌ی شبکه)، `buzzer_task`.

### نکات معماری
- **دکمه‌های کارت‌ها eez-flow نیستند:** هر دکمه `event_handler_*` در `screens.c` دارد که مستقیم `action_*` (در `.ino`، declare در `actions.h`) را صدا می‌زند.
- **bindingِ متغیرها** از طریق eez-flow و بایناریِ `assets[]` است؛ دستی اضافه‌شدنی نیست. پس نسخه‌ها/سازگاری مستقیم روی آبجکت‌ها نوشته می‌شوند.
- `vars.c/h`: متغیرهای native (`get_var_/set_var_`)؛ از `app_task` ست و در نخِ LVGL اعمال می‌شوند (thread-safe).

### ترجمه — `lang.c/lang.h`
- `enum StrId` + `I18N[STR_ID_COUNT][LANG_COUNT]`، ستون‌ها **EN, TR, FA**. `tr(StrId)`, `i18n_register`, `i18n_apply` (سقف `I18N_MAX 80`). فارسی در EEZ `0x020–0xffff`.

---

## ۴) صفحات و جریان

`splash` → `password` → `wifi` → `framware`.
رمز (`action_admin_password_redy`): `123`=منوی به‌روزرسانی، `0000`=محیط تست، غیره=اشتباه.

---

## ۵) صفحه‌ی به‌روزرسانی (چیدمانِ فعلی)

- کادر اطلاعاتِ چپ **حذف** شده. کل صفحه یک **ردیفِ افقیِ اسکرول‌شونده** (`objects.cards_row`، flex row، `LV_DIR_HOR`).
- هر کارت **عمودی** ۲۵۰×۳۱۶ با `build_prog_card(parent, idx, StrId titleId, const lv_img_dsc_t *img)`:
  اسم بالا (`kalame_bold_14`) ← عکسِ ۲۴۰px وسط ← نسخه/«ناموجود» (`card_ver[idx]`) ← لیبلِ «نرم‌افزار سازگار نیست» قرمز مخفی (`card_incompat[idx]`) ← دکمه‌ی نصب (`card_btn[idx]`) با `event_handler_card_install` و `user_data=idx`.
- پنلِ دانلود `waiting_panle_1`: لیبلِ `obj28` + **حلقه‌ی arc `obj29`** که با درصد پر می‌شود + آیکونِ دانلود.
- پنلِ **تأیید نصب** `confirm_panel`: `confirm_label` + `confirm_yes`/`confirm_no`.

### ۸ برنامه (ترتیب = جدول‌های `tasks.h`)
| idx | اسم | پوشه‌ی هاست (مدل) | نیاز سخت‌افزار |
|---|---|---|---|
| 0 | جوش تک‌سر (STR_PROD_SINGLE) | `plc_hmi/upvc/firmware_welder_single_head` | ≥۱ MAX6675 |
| 1 | جوش دوسر (STR_PROD_WELDER_DOUBLE) | `plc_hmi/upvc/firmware_welder_double_head` | ۲×MAX6675 + MCP23017 |
| 2 | برش متر دیجیتال تک‌سر (STR_PROD_CUTTER_SINGLE) | `plc_hmi/upvc/firmware_digital_cutter_single_head` | — |
| 3 | برش متر دیجیتال دوسر (STR_PROD_CUTTER_DOUBLE) | `plc_hmi/upvc/firmware_digital_cutter_double_head` | MCP23017 |
| 4 | تزریق گاز شیشه (STR_PROD_GAS) | `plc_hmi/upvc/firmware_gas_filling` | — |
| 5 | گوشه‌تمیزکن (STR_PROD_CORNER) | `plc_hmi/upvc/firmware_corner_cleaning` | — |
| 6 | کپی فرز (STR_PROD_COPY) | `plc_hmi/upvc/firmware_copy_router` | — |
| 7 | پانچ آلومینیوم (STR_PROD_PUNCH) | `plc_hmi/upvc/firmware_aluminum-punching` | — |

جدول‌ها: `g_card_models[8]`, `g_card_name[8]`(StrId), `g_card_hwreq[8]`(0=ندارد،1=≥1MAX،2=۲MAX+MCP،3=MCP). آبجکت‌ها در `screens.h`: `card_btn[8]/card_ver[8]/card_incompat[8]`.

### جریانِ نصب با تأیید
نصب → `action_card_install` → `config_request_install(idx)` (اگر ناسازگار: هیچ؛ وگرنه پنل تأیید + `g_pending_card`). بله → `config_confirm_yes` → `config_start_install` (ست مدل + `ota_download=true`). انصراف → بستن.

---

## ۶) OTA

- هاست `haffler.com`، پایه `ota`. **سرور فقط HTTPS است؛ http را ۳۰۱ به https ریدایرکت می‌کند** (nginx). پس همه با **`WiFiClientSecure` + `setInsecure()`** و URLِ `https://` (ESP32 ریدایرکتِ http→https را دنبال نمی‌کند).
- هر مدل: نسخه `…/<model>/lastfile` (مثل `1.4.9`)، باینری `…/<model>/firmware.bin`.
- `config_fetch_lastfile(model,&exists)` (نسخه+وجود) در `app_task`؛ `download_firmware(model)` دانلودِ دستی با `Update.*` و **بافرِ ۱۶ک در PSRAM**، درصد با `ota_progress_cb` روی arc/لیبل، نبودِ فایل → «این برنامه روی سرور نیست».
- بعد از وای‌فای، نسخه/«ناموجود» هر ۸ کارت با `config_apply_card_versions()` نوشته می‌شود.
- سرعت: HTTPS کند است (~۱–۱.۵ دقیقه/۵MB). `WiFi.setSleep(false)`+`setTxPower(max)` اعمال شده. سریع‌ترین راه: سرور `/ota/` را با http باز کند (کارِ پشتیبانیِ نت‌افزار؛ `.htaccess` بی‌اثر چون nginx) → بعد کد را به http برگردان.

---

## ۷) تشخیص سخت‌افزار — `hwtest.cpp`

`hwtest_detect_hardware()` در `setup()` بعد از `hwtest_setup()`:
- `g_max6675_count` (0..2): خواندنِ خامِ بیت‌بنگ از CS1/CS2؛ حاضر اگر نه `0xFFFF` نه `0x0000` و بیت۱۵=۰.
- `g_mcp23017_present`: پینگِ I2C `0x20..0x27`.
اعمال در `config_apply_hw_compat()` (نخ LVGL، یک‌بار وقتی `card_btn[0]` ساخته شد): دکمه قرمز + لیبلِ ناسازگار + بلاکِ نصب.

---

## ۸) عکس‌ها

- منبع: PNGِ شفاف (نمونه `C:\Users\Haffler\Desktop\machine_images_240`، اندازه‌ی طراحی **۲۴۰**).
- تبدیل: `python png_to_lvgl.py <input.png> img_bitmap_<name> 240` → `ui_image_bitmap_<name>.c` (LVGL v9 `NATIVE_WITH_ALPHA`, planar RGB565+A8, `data_size=w*h*3`). نیازمندِ Pillow. **نسبت را نکش.**
- بعد: `extern const lv_img_dsc_t img_bitmap_<name>;` در `images.h` و `&img_bitmap_<name>` در کارت.
- `convert_images_v9.py` فقط v8→v9 planar. حذفِ بک‌گراند در صورت نیاز: `rembg` مدلِ کوچک `u2netp`.
- فونت‌ها (در `fonts.h`): `ui_font_kalame_bold_14`, `kalameh_bold_18`, `kalameh_regular_12`, `kalameh_regular_10`.

---

## ۹) بیلد و آپلود ⚠️

- arduino-cliِ بسته‌ی Arduino IDE: `"C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"`. هسته esp32 `3.3.8`. برد **COM6**.
- **FQBN (به فلگ‌های USB دست نزن):**
  `esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=custom,DebugLevel=none,PSRAM=enabled`
- ساخت+آپلود:
  `arduino-cli compile --upload -p COM6 --fqbn "<FQBN>" "D:\SOFT WARE PLC HMI\Config_Plus_Test\V.1.0.0\config_hwtest" --warnings none`
- پارتیشن: `partitions.csv` سکچ (دو اپ ۷MB + littlefs) → `PartitionScheme=custom` + `FlashSize=16M`. PSRAM اجباری.
- خطای «Some indexes could not be updated»/`exit 255` فقط نویزِ ایندکسِ بردهای ثالث است، نه کد؛ دوباره اجرا کن. قبل از آپلود Serial Monitor روی COM6 را ببند.

---

## ۱۰) دام‌ها

- **`screens.c` تولیدیِ EEZ است؛ regenerate همه‌ی دست‌بردها را پاک می‌کند.**
- ریدایرکتِ https توسط nginxِ هاست؛ `.htaccess` بی‌اثر.
- کارِ شبکه از نخِ LVGL نزن؛ نوشتن روی ویجت فقط نخِ LVGL.
- ساعتِ غلط = باتری/ماژولِ RTC، نه کد.
- همان پلتفرمِ پروژه‌های جوش/برشِ Haffler.

---

## ۱۱) کارهای متداول

**برنامه‌ی جدید:** ۱) عکس→`png_to_lvgl.py ... img_bitmap_<name> 240`→`extern` در images.h. ۲) `STR_PROD_<X>` در lang.h/lang.c. ۳) درایه در `g_card_models/g_card_name/g_card_hwreq` و بزرگ‌کردنِ طولِ آرایه‌ها و حلقه‌های `8` + `card_*[N]` در screens.h. ۴) `build_prog_card(...)` در `cards_row`. ۵) پوشه‌ی `firmware_<...>` روی هاست (با `firmware.bin` و `lastfile`).
**عوض عکس:** `png_to_lvgl.py` روی PNG جدید با همان symbol.
**اسم/ترجمه:** ردیفِ `lang.c`.
**قانون سازگاری:** `g_card_hwreq[idx]` در `tasks.h`.

---

## ۱۲) محیطِ تستِ سخت‌افزار (`hwtest.cpp` + `t_screens.c`)  ← مهم

ورود با رمزِ **`0000`** در صفحهٔ رمز. UIِ تست پیشوندِ `t_` دارد (`t_screens.c`, `t_vars.c`, `t_objects.*`, `T_SCREEN_ID_*`) و منطقش در `hwtest.cpp`. حلقه‌اش `hwtest_loop()` از `lvgl_task` صدا زده می‌شود (وقتی `g_hwtest_active`). صفحات: `setup`(خروجی/ورودی GPIO)، `ssr`، `clock_setup`، و صفحاتِ I2C/سیستم/بلوتوث/RS485.

### خروجی‌ها (Q) — صفحهٔ setup
`static int pins[10] = {16,42,39,48,40,15,35,36,38,41}`. در `hwtest_loop` وقتی `get_var_setuploaded()`:
`digitalWrite(pins[i], !get_var_qN())` — یعنی **active-LOW**. نگاشت: q1→16، q2→42، q3→39، q4→48، q5→40، q6→15، q7→35، q8→36، **q9→38، q10→41**.
شمارشِ روی صفحه Q0..Q9 است (۰-ایندکس): «Q9»یِ کاربر = آخرین کلید = q10 = **GPIO41**.
- **زدن روی لیبلِ «OUTPUT»** (هندلر `event_handler_output_all_toggle` در t_screens.c): یک‌بار همه روشن+بولد، بارِ بعد همه خاموش+نازک.
- **Q9/GPIO41 (2026-06-29):** کد درست است؛ اگر فیزیکی تاگل نشد → سخت‌افزار/سیم‌کشی (یا رله روی پایهٔ دیگر؛ آن‌وقت `pins[9]` را عوض کن). توجه: `pins[5]=15` و `pins[9]=41` در `software_pwm()` (تستِ SSR) هم استفاده می‌شوند.

### ورودی‌ها (I) — مدلِ ۴-سطحیِ آنالوگِ مالتی‌پلکس (پورت‌شده از جوش)
ADCها: pin6=I0/I1، pin5=I2/I3، pin4=I4/I5، pin3=I6، pin7=I7. هلپرِ `hw_sensor_band(adc)`:
اول≈4095(band0)، دوم≈1530(band1, 1300–1780)، هیچ≈2040(band2, 1850–2240)، هردو≈2530(band3, 2350–2750)، بینِ سطوح=-1 (نادیده، حالتِ قبلی).
هر پایه دو سوییچ: «اول» = band 0 یا 3، «دوم» = band 1 یا 3. **«فعال» = 0** (قراردادِ این UI: هیچ→1، هردو→0). `set_var__10..__15`.
**I6/I7:** آنالوگ **زیر ۴۳۰ → فعال (=0)** : `set_var__16/__17`.

### سنسورها / I2C
- **MAX6675** (دما): SO=37, SCK=47, **CS1=21, CS2=46** (CS2 قبلاً ۰ بود؛ 2026-06-29 به ۴۶ تغییر کرد). ⚠️ پایهٔ ۴۶ هم‌زمان **DEِ RS485** است → در محیطِ تست تداخل دارد؛ اگر RS485 لازم شد باید DE را جابه‌جا کرد.
- تشخیصِ سازگاری: `hwtest_detect_hardware()` در setup → `g_max6675_count` (خواندنِ خامِ بیت‌بنگ) و `g_mcp23017_present` (پینگِ I2C 0x20..0x27).
- I2C: تاچ، RTC(0x68). آلارمِ خطای I2C (`beepI2CErrorDouble`، هر ۳ث دوبوق) اگر تاچ یا RTC پیدا نشود. **تاچ از `bbct.sensorType()` تشخیص داده می‌شود** نه آدرسِ ثابتِ 0x5D (چون چیپ ممکن است آدرسِ دیگری داشته باشد).

### ساعت/تاریخ
- نمایش در صفحهٔ تست: `t_objects.h/m/yer/mont/dy` از `rtc.now()`.
- **نگه‌داشتنِ ۲ث روی ساعت یا تاریخ** → `action_gotortcsetup` → صفحهٔ `clock_setup`.
- `clock_setup` فعلاً ابتدایی است (تایپِ رشتهٔ `YYYY.MM.DD.HH.MM` در textarea → `set_var_rtc_text` → `action_callrtc`/`get_var_rtc_apply` → `set_rtc_from_ui` که `rtc.adjust` می‌کند). **TODO: حرفه‌ای‌کردن با roller/+−.**

### کیبردِ رمزِ ورود (`screen_password` در `screens.c`)
`lv_keyboard` با نقشهٔ سفارشیِ `LV_KEYBOARD_MODE_USER_1` (`cfg_pass_kb_map`/`cfg_pass_kb_ctrl`): فقط عدد + «پاک کردن» + «تایید». textarea به کیبرد bind **نشده** (`set_textarea(..., NULL)`)؛ ورودِ ارقام/پاک/تایید دستی در `event_handler_cb_screen_password_frameware_password_keybord` (VALUE_CHANGED). دکمهٔ «تایید» → `action_admin_password_redy`.
> ⚠️ **باگِ باز:** فونتِ دکمه‌ها هنوز پیش‌فرض (montserrat) است → «تایید/پاک کردن» **مربع** می‌شوند. فیکس و نسخهٔ سه‌زبانه/by-id در فصل ۱۳ بند ۱ توضیح داده شده.

---

## ۱۳) تغییراتِ اخیر + کارهای باقی‌مانده (2026-06-29)

**پورتِ آپلودِ فعلی: `COM20`** (قبلاً COM6 بود؛ برد روی پورت عوض می‌شود — با `[System.IO.Ports.SerialPort]::GetPortNames()` چک کن).

**انجام‌شده این جلسه:** کیبردِ رمز به نقشهٔ سفارشیِ عددی (`LV_KEYBOARD_MODE_USER_1`: عدد + پاک + تایید، هندلرِ دستی، textarea unbind)، نگه‌داشتن روی تاریخ→تنظیم، OUTPUTِ GPIO (روشن/خاموشِ همه + بولد)، ورودی‌های I0–I5 (مدل باند) و I6/I7 (<۴۳۰)، CS2→۴۶، تشخیصِ تاچ با bbct.

**انجام‌شده (ادامه):** کیبردِ سه‌زبانه+فونت (کاربر)، کارتِ نهمِ Config کامل (کاربر)، long-pressِ تاریخ (کاربر)، تپِ OUTPUT (کاربر)، و **SSR**: پیش‌فرضِ ۸۰۰/۲۰۰ms + ایمنیِ دما>۲۰۰° (پنلِ هشدارِ runtime در `hwtest.cpp`: `ssr_warn_open`/`ssr_warn_close_and_off` + نوارِ ۶ث؛ خاموشیِ المنت = `set_var_pwm_state(0)` + `pins[5]/pins[9]`=HIGH).

**باقی‌مانده (TODO):**
0. **SSR دو فیلدِ مجزای ON/OFF:** فعلاً همان textareaِ تکیِ «on.off» است (فقط پیش‌فرض و ایمنی اضافه شد)؛ تبدیل به دو فیلدِ جدا باقی مانده (تغییرِ layoutِ صفحهٔ ssr در t_screens.c).
1. 🔴 **باگِ مربع‌مربعیِ کیبردِ رمز:** [حل شد توسط کاربر] کلیدهای «تایید/پاک کردن» (و هر متنِ غیرلاتین) □□□ نشان داده می‌شوند چون **فونتِ پیش‌فرضِ دکمه‌های کیبرد (montserrat) گلیفِ فارسی ندارد**. **فیکس:** هنگام ساختِ کیبرد `lv_obj_set_style_text_font(kb, &ui_font_kalame_bold_14, LV_PART_ITEMS | LV_STATE_DEFAULT);`. پیشنهاد: نقشهٔ سه‌زبانه (`cfg_pass_kb_map_fa/en/tr` بر اساس `g_lang`) و تشخیص در هندلر **با id** (id 9=پاک، id 11=تایید) به‌جای `strcmp` با متن (تا مستقل از زبان باشد).
2. **SSR:** دو فیلدِ ON/OFF (پیش‌فرض ۸۰۰/۲۰۰ms به‌جای textareaِ تک)؛ اگر دما(temp1/temp2 از MAX6675) > **۲۰۰°** → دیالوگِ تأیید + **نوارِ شمارشِ معکوسِ ۶ثانیه**؛ تأیید یا اتمامِ ۶ث → خاموش‌کردنِ المنت (توقفِ PWM) + بستنِ پیغام. صفحهٔ فعلی: `create_screen_ssr` (textarea `pwm_time_text_tarea` + کیبرد + START/STOP؛ `software_pwm` با `pwm_on_time_ms/off`).
3. **وای‌فای/بلوتوثِ تست** را استاندارد کن.
4. **ناوبریِ کلیدی (بزرگ):** پورتِ سیستمِ `panel_nav` از جوش (`D:\V.1.4.8_fixpack_2026-06-23\src\main.cpp`) با کلیدهای **MOD/SAVE/CANCEL/START(=تایید)** برای ورود/خروج و چرخش در همهٔ منوها بدون تاچ.
5. **Q9/GPIO41:** تأیید سخت‌افزاری (کد درست است).
6. **کارتِ نهم «Config» (خودبه‌روزرسانی):** آرایه‌های `g_card_*` در `tasks.h` به ۹ رسیده (index 8 = `firmware_config`, `STR_PROD_CONFIG`). برای کامل‌شدن باید: (الف) گاردهای `config_request_install`/`config_start_install` از `idx >= 8` به `idx >= 9` تغییر کنند وگرنه کارتِ Config نصب نمی‌شود؛ (ب) `STR_PROD_CONFIG` در `lang.h/lang.c`؛ (ج) `card_*[9]` در `screens.h`؛ (د) یک `build_prog_card(..., 8, STR_PROD_CONFIG, &img...)` در `screens.c`. (firmware_config = خودبه‌روزرسانیِ نصاب؛ منطقِ `selfUpdate`/`versionNewer` در `download_firmware`.)
