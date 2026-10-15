#include "ui_lang.h"
#include "storage.h"
#include "fonts.h"
#include "esp_log.h"
#include "lvgl_port.h"
#include <string.h>

static const char *TAG = "ui_lang";

static ui_lang_t s_current_lang = UI_LANG_FA;
static ui_lang_refresh_cb_t s_refresh_cb = NULL;

static const char * const s_dict[STR_COUNT][3] = {
    /* Main Screen */
    [STR_MAIN_TITLE]    = {"دستگاه بستنی‌ساز", "Ice Cream Machine", "Dondurma Makinesi"},
    [STR_MODE_WASH]     = {"شستشو", "Wash", "Yikama"},
    [STR_MODE_PROD]     = {"تولید", "Produce", "Uretim"},
    [STR_MODE_HOLD]     = {"نگهداری", "Standby", "Bekleme"},
    [STR_MODE_STOP]     = {"توقف", "Stop", "Durdur"},
    [STR_EDGE_INJECT]   = {"تزریق لبه", "Edge", "Kenar"},
    [STR_RAINBOW]       = {"پارو شیر", "H-Paddle", "H-Palet"},
    [STR_TODAY]         = {"امروز:", "Today:", "Bugun:"},
    [STR_TOTAL]         = {"کل", "Total", "Toplam"},

    /* Settings Tabs Headers */
    [STR_TAB_F1_HDR]    = {"تنظیمات تایمرهای تولید", "Production Timers Settings", "Uretim Zamanlayici Ayarlari"},
    [STR_TAB_F2_HDR]    = {"تنظیمات ساعت و تاریخ", "Clock & Date Settings", "Saat ve Tarih Ayarlari"},
    [STR_TAB_F3_HDR]    = {"تنظیمات حالت نگهداری و برودت", "Holding & Storage Cooling", "Bekleme ve Sogutma Ayari"},
    [STR_TAB_F4_HDR]    = {"ارتباط وای‌فای و مانیتورینگ", "WiFi & Monitoring", "WiFi ve Izleme"},
    [STR_TAB_F5_HDR]    = {"تست و دیاگ ورودی و خروجی‌ها", "I/O Hardware Diagnostics", "Giris/Cikis Test ve Diyagnostik"},
    [STR_TAB_F6_HDR]    = {"تنظیمات فروش و حفاظت ایمنی", "Dispense & Safety Settings", "Servis ve Guvenlik Ayarlari"},
    [STR_TAB_F7_HDR]    = {"تنظیمات فروش و حفاظت ایمنی", "Dispense & Safety Settings", "Servis ve Guvenlik Ayarlari"},

    /* F1 Timers */
    [STR_TMR_PADDLE]    = {"کارکرد پارو (دقیقه)", "Paddle Time (min)", "Palet Suresi (dk)"},
    [STR_TMR_COMP_RST]  = {"تاخیر کمپرسور (ثانیه)", "Comp. Delay (sec)", "Kompresor (sn)"},
    [STR_TMR_EYE]       = {"مهلت دور پولی (ثانیه)", "Pulley Timeout (s)", "Kasnak Suresi (sn)"},
    [STR_T1_FALLBACK_ON]  = {"پارو روشن (ثانیه)", "Paddle ON (sec)", "Palet Calisma (sn)"},
    [STR_T1_FALLBACK_OFF] = {"پارو خاموش (دقیقه)", "Paddle OFF (min)", "Palet Bekleme (dk)"},
    [STR_COMP_FALLBACK_ON]  = {"نگهدارنده روشن (دقیقه)", "Hold ON (min)", "Bekleme Calisma (dk)"},
    [STR_COMP_FALLBACK_OFF] = {"نگهدارنده خاموش (دقیقه)", "Hold OFF (min)", "Bekleme Durma (dk)"},
    [STR_COUNTER_TITLE] = {"شمارنده تولید", "Production Count", "Uretim Sayaci"},
    [STR_COUNTER_RESET] = {"صفر کردن", "Reset", "Sifirla"},
    [STR_STANDBY_PREFIX]= {"خاموشی خودکار صفحه", "Auto Screen Sleep", "Ekran Uyku Modu"},
    [STR_STANDBY_OFF]   = {"خاموش", "Off", "Kapali"},
    [STR_MINUTES]       = {"دقیقه", "min", "dk"},
    [STR_SECONDS]       = {"ثانیه", "sec", "sn"},
    [STR_HOURS]         = {"ساعت", "hrs", "saat"},

    /* F2 Clock & NTP */
    [STR_YEAR]          = {"سال", "Year", "Yil"},
    [STR_MONTH]         = {"ماه", "Month", "Ay"},
    [STR_DAY]           = {"روز", "Day", "Gun"},
    [STR_HOUR]          = {"ساعت", "Hour", "Saat"},
    [STR_MINUTE]        = {"دقیقه", "Minute", "Dakika"},
    [STR_NTP_SYNC_BTN]  = {"تنظیم ساعت از اینترنت", "Auto Time (Internet)", "Internetten Saat Ayari"},
    [STR_NTP_SYNCING]   = {"در حال دریافت ساعت...", "Syncing time...", "Saat eslestiriliyor..."},
    [STR_NTP_SUCCESS]   = {"ساعت با موفقیت تنظیم شد", "Time synchronized", "Saat eslestirildi"},
    [STR_NTP_NO_WIFI]   = {"اینترنت متصل نیست (تنظیمات وای‌فای)", "No internet (open F4)", "Internet yok (F4 agina baglan)"},

    /* F3 Inverter / Hold */
    [STR_MODBUS_DELAY]  = {"تاخیر مدباس (میلی‌ثانیه)", "Modbus Delay (ms)", "Modbus Gecikmesi (ms)"},
    [STR_COMP_CYCLE]    = {"دوره استراحت کمپرسور سیلندر (دقیقه)", "Cylinder Comp Rest Cycle (min)", "Silindir Kompr. Dinlenme (dk)"},
    [STR_HOLD_DELAY]    = {"تاخیر استارت نگهدارنده (ثانیه)", "Hold Start Delay (s)", "Bekleme Baslama Gecikmesi (sn)"},
    [STR_CURRENT_SET1]  = {"جریان اول (یک‌دهم آمپر)", "Current Limit 1 (0.1A)", "Motor Akim Limiti 1 (0.1A)"},
    [STR_CURRENT_SET2]  = {"جریان دوم (یک‌دهم آمپر)", "Current Limit 2 (0.1A)", "Motor Akim Limiti 2 (0.1A)"},
    [STR_TEMP_T1_ON]    = {"دمای استارت مخزن شیر (°C)", "Milk Tank Start Temp (°C)", "Sut Tanki Baslatma Sicakligi (°C)"},
    [STR_TEMP_T1_OFF]   = {"دمای استپ مخزن شیر (°C)", "Milk Tank Stop Temp (°C)", "Sut Tanki Durdurma Sicakligi (°C)"},

    /* F4 WiFi */
    [STR_WIFI_SCAN]     = {"جستجو", "Scan", "Ara"},
    [STR_WIFI_CONNECT]  = {"اتصال", "Connect", "Baglan"},
    [STR_OTA_UPDATE_BTN]= {"بروزرسانی فریمور و اطلاعات برنامه", "Firmware Update & Info", "Yazilim Guncelleme ve Bilgi"},

    /* F5 Schedules */
    [STR_F5_HOLD_TIME]  = {"زمان نگهدارنده (ساعت)", "Holding Time (hr)", "Bekleme Suresi (saat)"},
    [STR_F5_PROD_TIME]  = {"زمان تولید بستنی (ساعت)", "Ice Cream Time (hr)", "Dondurma Uretim (saat)"},
    [STR_F5_PADDLE_TIME]= {"زمان کارکرد پارو (دقیقه)", "Paddle Work (min)", "Karistirici Calisma (dk)"},
    [STR_F5_MAIN_COMP]  = {"سقف کارکرد کمپرسور سیلندر (دقیقه)", "Cylinder Comp Max Run (min)", "Silindir Kompr. Azami Calisma (dk)"},
    [STR_F5_HOLD_COMP]  = {"نگهدارنده (دقیقه)", "Holding (min)", "Bekleme (dk)"},

    /* F6 Diagnostics */
    [STR_F6_INPUTS_HDR] = {"سنسورهای ورودی", "Input Sensors", "Giris Sensorleri"},
    [STR_F6_OUTPUTS_HDR]= {"تست دستی خروجی‌ها", "Manual Output Test", "Manuel Cikis Testi"},
    [STR_F6_IN_FRONT_SW]= {"میکروسوئیچ جلو", "Front Microswitch", "On Limit Switch"},
    [STR_F6_IN_REAR_SW] = {"میکروسوئیچ عقب", "Rear Microswitch", "Arka Limit Switch"},
    [STR_F6_IN_EYE]     = {"سنسور پولی گیربکس", "Pulley Sensor", "Kasnak Sensoru"},
    [STR_F6_IN_STOP]    = {"سنسور خالی شیر", "Milk Low Sensor", "Sut Bos Sensoru"},
    [STR_F6_IN_START]   = {"شستی استارت پنل", "Panel Start", "Panel Baslatma"},
    [STR_F6_IN_MTR_OVL] = {"اضافه بار موتور", "Motor Overload", "Motor Asiri Yuk"},
    [STR_F6_IN_COMP_OVL]= {"اضافه بار کمپرسور", "Comp Overload", "Kompresor Asiri Yuk"},
    [STR_F6_OUT_COMP]   = {"نگهدارنده", "Holding", "Bekleme"},
    [STR_F6_OUT_PROD]   = {"کمپرسور بزرگ", "Main Compressor", "Ana Kompresor"},
    [STR_F6_OUT_MIXER]  = {"موتور پارو", "Paddle Motor", "Palet Motoru"},
    [STR_F6_OUT_EDGE]   = {"تزریق لبه", "Edge Injection", "Kenar Enjeksiyon"},
    [STR_F6_OUT_RAINBOW]= {"پارو مخزن شیر", "Hopper Paddle", "Hazne Paleti"},
    [STR_F6_OUT_AIR]    = {"شیر باد", "Air Valve", "Hava Valfi"},
    [STR_F6_OUT_RESET]  = {"ریست کنترل بار", "Load Reset", "Yuk Sifirla"},
    [STR_F6_OUT_WASH]   = {"شستشو", "Wash Output", "Yikama Cikisi"},
    [STR_F6_TEMP_T1]    = {"دمای مخزن شیر", "Hopper (T1)", "Sut Tanki (T1)"},
    [STR_F6_TEMP_T2]    = {"دمای سیلندر", "Cylinder (T2)", "Silindir (T2)"},

    /* F7 Dispense & Safety */
    [STR_TMR_DSP_RUN]   = {"شارژ برودت (ثانیه)", "Rechill Time (s)", "Sogutma (sn)"},
    [STR_TMR_FRT_HOLD]  = {"نگهداشت اهرم (ثانیه)", "Lever Hold (s)", "Kol Tutma (sn)"},
    [STR_CNT_SCP_COMP]  = {"شمارنده قیف شارژ", "Rechill Scoops", "Sogutma Kulahi"},
    [STR_TMR_FRZ_TO]    = {"سقف انجماد (دقیقه)", "Max Freeze (min)", "Maks Donma (dk)"},
    [STR_TMR_LEV_MAX]   = {"هشدار اهرم (ثانیه)", "Max Lever Alarm (s)", "Maks Kol Alarm (sn)"},
    [STR_LOCKED_TIMERS_HDR] = {"تایمرهای ثابت ایمنی", "Safety Interlocks", "Sabit Guvenlik"},
    [STR_LOCKED_BADGE]  = {"قفل سخت افزار", "Hardware Locked", "Donanim Kilidi"},
    [STR_FIXED_COOLDOWN]= {"تاخیر کمپرسور", "Comp Delay", "Kompresor Gecikme"},
    [STR_FIXED_RESET_PULSE] = {"پالس ریست", "Reset Pulse", "Reset Palsi"},
    [STR_FIXED_DEBOUNCE]= {"فیلتر نویز", "Noise Filter", "Giris Filtresi"},
    [STR_FIXED_CYL_LIMIT]= {"حد انجماد", "Freeze Limit", "Donma Limiti"},
    [STR_FIXED_PULLEY_TIMEOUT] = {"پایش دور پولی", "Pulley Monitor", "Kasnak Takibi"},

    /* Bottom Toolbar */
    [STR_BTN_HOME]      = {"خانه", "Home", "Ana Sayfa"},
    [STR_BTN_SAVE]      = {"ذخیره", "Save", "Kaydet"},
    [STR_BTN_HELP]      = {"راهنما", "Help", "Yardim"},
    [STR_BTN_FAULTS]    = {"خطاها", "Faults", "Hatalar"},

    /* Help Dialogs */
    [STR_HELP_GOT_IT]   = {"متوجه شدم", "Got it", "Anladim"},
    [STR_HELP_F1_TITLE] = {"راهنمای زمان‌بندی و دوره‌های تولید", "Production Timers Guide (F1)", "Uretim Zamanlayici Rehberi (F1)"},
    [STR_HELP_F1_BODY]  = {
        "• زمان کارکرد پارو (دقیقه):\nمدت زمان چرخش موتور پارو سیلندر در چرخه تولید بستنی. چرخش پارو باعث هوادهی یکنواخت و قوام آمدن بافت بستنی می‌شود. در صورت تنظیم کمتر از 8 دقیقه، بستنی شل و آبکی مانده و در صورت تنظیم بیشتر از 15 دقیقه، اصطکاک بیهوده پارو باعث افت کیفیت بافت خواهد شد (بازه استاندارد: 8 الی 12 دقیقه - پیش‌فرض: 10 دقیقه).\n\n"
        "• تاخیر کمپرسور بزرگ (ثانیه):\nفاصله زمانی استارت کمپرسور برودت سیلندر پس از به راه افتادن موتور پارو. استارت همزمان این دو موتور بار القایی سنگینی روی برق کارگاه ایجاد می‌کند. با این تاخیر، ابتدا مایه شیر توسط پارو به گردش کامل درآمده و سپس سیستم تبرید وارد مدار می‌شود تا به گیربکس و کمپرسور فشار نیاید (پیش‌فرض: 15 ثانیه).\n\n"
        "• مهلت دور پولی گیربکس (ثانیه):\nحداکثر زمان مجاز بدون دریافت پالس از سنسور القایی پولی پارو هنگام روشن بودن موتور. در صورت پاره شدن تسمه، هرزگرد شدن پولی، یا گیر کردن پارو در یخ‌زدگی شدید، کنترلر بلافاصله موتور را ظرف این زمان خاموش کرده و خطای توقف پولی (کد ۰۱) صادر می‌کند تا سیم‌پیچ الکتروموتور نسوزد (بازه مجاز: 3 الی 5 ثانیه - پیش‌فرض: 4 ثانیه).\n\n"
        "• تایمر روشن پارو شیر (ثانیه):\nزمان چرخش الکتروموتور پارو کوچک داخل مخزن بالا (بن‌ماری). این چرخش مداوم از دوفاز شدن مایه بستنی، ته‌نشینی چربی و خامه، و بستن قشر یخ سطحی جلوگیری می‌کند (پیش‌فرض: 15 ثانیه).\n\n"
        "• تایمر خاموش پارو شیر (دقیقه):\nمدت زمان استراحت پارو مخزن بالا بین هر بار اختلاط شیر. این وقفه از گرم شدن مایه بستنی در اثر اصطکاک مداوم پروانه و استهلاک زودهنگام موتور پیشگیری می‌نماید (بازه استاندارد: 3 الی 8 دقیقه - پیش‌فرض: 5 دقیقه).\n\n"
        "• کارکرد نگهدارنده بدون سنسور (دقیقه):\nزمان روشن ماندن نگهدارنده در زمان قطعی یا عدم اتصال سنسور دمای مخزن شیر. این حالت اضطراری تضمین می‌کند حتی در صورت نبود سنسور فیزیکی، شیر مخزن ترش و فاسد نشود (پیش‌فرض: 3 دقیقه).\n\n"
        "• استراحت نگهدارنده بدون سنسور (دقیقه):\nزمان خاموش ماندن نگهدارنده در حالت بدون سنسور. این وقفه مانع از یخ‌زدگی مایه شیر در مخزن بالا و مسدود شدن مجرای تغذیه به سیلندر می‌شود (پیش‌فرض: 20 دقیقه).\n\n"
        "• سنسور دور پولی (فعال / غیرفعال):\nکلید فعال‌سازی پایش هوشمند دور پولی پارو. در ساعات شلوغی اگر سنسور القایی دچار خرابی موقت شود، اپراتور می‌تواند با غیرفعال کردن آن دستگاه را تا زمان تعویض سنسور موقتاً به کار ادامه دهد.\n\n"
        "• شمارنده کل و روزانه و کلید صفر کردن:\nثبت دقیق تعداد قیف‌های بستنی فروخته شده؛ شمارنده کل غیرقابل دستکاری تا ۹۹ میلیون است و شمارنده روزانه با لمس کلید صفر کردن جهت حسابداری روزانه ریست می‌شود.\n\n"
        "• خاموشی خودکار صفحه (Screen Sleep):\nتنظیم زمان رفتن نمایشگر به حالت خواب پس از آخرین لمس، جهت صرفه‌جویی در مصرف برق و افزایش طول عمر پنل ال‌سی‌دی (گزینه‌های خاموش یا ۱ الی ۳۰ دقیقه).",

        "• Paddle Run Time (min):\nCylinder paddle run duration in ice cream production. Paddle rotation aerates the mix and produces silky texture. Below 8 min leaves mix watery; above 15 min causes frictional heat and texture breakdown (Shop range: 8-12 min, Factory: 10 min).\n\n"
        "• Compressor Delay (sec):\nSoft-start delay for main compressor after paddle motor starts. Prevents high starting current surges on workshop wiring and eases gearbox load (Default: 15 sec).\n\n"
        "• Pulley Timeout (sec):\nMaximum allowed time without pulses from the pulley inductive sensor while paddle is powered. Protects belt, gearbox, and prevents motor burnout by tripping E01 within this window (Range: 3-5 sec, Default: 4 sec).\n\n"
        "• Hopper Paddle ON Time (sec):\nRotation time for top hopper paddle. Prevents mix separation, fat settling, and surface freezing (Default: 15 sec).\n\n"
        "• Hopper Paddle OFF Time (min):\nRest duration for hopper paddle between mixing cycles. Avoids mix warming and motor wear (Range: 3-8 min, Default: 5 min).\n\n"
        "• Sensorless Hold ON (min):\nEmergency holding run time when hopper thermocouple T1 is disconnected, keeping milk preserved (Default: 3 min).\n\n"
        "• Sensorless Hold OFF (min):\nRest time for holding in sensorless mode, preventing hopper milk freezing (Default: 20 min).\n\n"
        "• Pulley Sensor (En/Dis):\nSmart toggle for pulley rotation monitoring. Allows emergency bypass during peak hours if inductive sensor fails.\n\n"
        "• Total & Daily Counters + Reset:\nMonitors dispensed cones up to 99,999,999 with daily reset button for shift accounting.\n\n"
        "• Auto Screen Sleep:\nConfigures LCD sleep timeout after inactivity to save power and extend backlight lifespan (Off or 1-30 min).",

        "• Palet Suresi (dk):\nDondurma uretiminde silindir palet calisma suresi. 8 dakikanin alti sulu birakir, 15 dakikanin ustu surtunme isisiyla kivami bozar (Standart: 8-12 dk, Onerilen: 10 dk).\n\n"
        "• Kompresor Gecikmesi (sn):\nPalet kalkisi sonrasi ana kompresor gecikmesi. Ani akim cekilmesini onler ve disli kutusunu korur (Varsayilan: 15 sn).\n\n"
        "• Kasnak Zaman Asimi (sn):\nMotor calisirken kasnak sensorunden sinyal gelmeme emniyet suresi. Kayis kopmasinda E01 hatasi vererek motoru korur (Varsayilan: 4 sn).\n\n"
        "• Sut Paleti Calisma (sn):\nUst tank paleti donus suresi. Yagin dibe cokmesini ve sutun ayrismasini engeller (Varsayilan: 15 sn).\n\n"
        "• Sut Paleti Dinlenme (dk):\nUst tank paletinin bekleme suresi. Sutun isinmasini onler (Standart: 3-8 dk, Varsayilan: 5 dk).\n\n"
        "• Sensorsiz Bekleme Calisma (dk):\nT1 termokupl yokken veya arizalandiginda sutun bozulmasini onleyen acil durum calisma suresi (Varsayilan: 3 dk).\n\n"
        "• Sensorsiz Bekleme Dinlenme (dk):\nSensorsiz modda tankta sutun buz tutmasini engelleyen bekleme suresi (Varsayilan: 20 dk).\n\n"
        "• Kasnak Sensoru (Aktif/Pasif):\nKasnak izleme sensorunu acma/kapama secenegi. Sensor arizasinda acil durumda gecici calisma saglar.\n\n"
        "• Toplam ve Gunluk Sayac + Sifirlama:\nVardiya takibi icin gunluk sifirlama ve 99 milyona kadar degismez toplam sayac.\n\n"
        "• Otomatik Ekran Uykusu:\nEnerji tasarrufu ve ekran omru icin bosta kalma uyku suresi (Kapali veya 1-30 dk)."
    },
    [STR_HELP_F2_TITLE] = {"راهنمای ساعت و تقویم سیستم", "Clock & Date Settings Guide (F2)", "Saat ve Tarih Rehberi (F2)"},
    [STR_HELP_F2_BODY]  = {
        "• تنظیم سال، ماه و روز:\nتنظیم دقیق تاریخ کاری دستگاه بر حسب تقویم جلالی (در منوی فارسی) یا میلادی (در انگلیسی و ترکی). ثبت دقیق سوابق خطاها در جعبه سیاه، استندبای هوشمند، و پایش چرخه‌های تولید به این تاریخ وابسته است و با باتری پشتیبان سخت‌افزاری ساعت (RTC) در قطعی کامل برق حفظ می‌شود.\n\n"
        "• تنظیم ساعت و دقیقه:\nتنظیم ساعت رسمی سیستم جهت ثبت دقیق زمان وقوع خطاها، شیفت کاری و زمان‌بندی تولید.\n\n"
        "• همگام‌سازی ساعت با اینترنت (NTP):\nبا اتصال دستگاه به مودم کارگاه و لمس این کلید، ساعت و تاریخ دستگاه مستقیماً از اتمیک کلاک‌های جهانی هماهنگ شده و خطای کریستال کالیبره می‌شود.\n\n"
        "• سیستم قفل اقساط و روزشمار (Day-Lock):\nنمایش روزشمار اعتبار فعال دستگاه، شمارش معکوس اقساط 5گانه، و ثبت کدهای چالش-پاسخ 5 رقمی کارخانه. در صورت انقضای روزشمار، دستگاه وارد قفل محافظتی شده و پس از اتمام چرخه جاری، از شروع چرخه جدید جلوگیری می‌کند.\n\n"
        "• کد چالش ۵ رقمی و کلید تمدید:\nکد رمزنگاری‌شده دستگاه جهت دریافت رمز فعال‌سازی از شرکت آستیاج و وارد کردن رمز بازگشایی.\n\n"
        "• تنظیم زنجیره ۵ مرحله‌ای اقساط:\nمدیریت و مشاهده زمان‌بندی اقساط ۵گانه کارخانه آستیاج.",

        "• Year, Month & Day Settings:\nAccurate date settings supporting Solar Jalali for Persian and Gregorian for English/Turkish. Backed by DS3231 RTC hardware with coin-cell battery for full power-loss memory.\n\n"
        "• Hour & Minute Settings:\nOfficial system time for accurate blackbox event logging, shift audits, and production schedules.\n\n"
        "• Internet Time Sync (NTP):\nOne-touch sync aligning the internal clock with global atomic time servers via WiFi.\n\n"
        "• Day-Lock Lease & Installments:\nMonitors remaining lease days and 5-stage installment codes. Safely completes active batch before entering soft lock on expiration.\n\n"
        "• 5-Digit Challenge Code & Renewal:\nEncrypted hardware challenge code used to receive official activation passwords from ASTIAJ.\n\n"
        "• 5-Stage Installment Chain:\nView and manage the 5 factory installment stages.",

        "• Yil, Ay ve Gun Ayari:\nFarsca icin Semsi, Turkce ve Ingilizce icin Miladi takvim. DS3231 dahili pil destekli saat ile elektrik kesintisinde zaman kaybolmaz.\n\n"
        "• Saat ve Dakika Ayari:\nKara kutu hata kayitlari ve vardiya takibi icin hassas sistem saati.\n\n"
        "• Internet Saat Eslestirme (NTP):\nWiFi uzerinden dunya atomik saat sunuculariyla tek tusla hassas saat ayari.\n\n"
        "• Taksit ve Lisans Kilidi (Day-Lock):\nKalan calisma gunu ve 5 asamadaki taksit yonetimi. Sure doldugunda mevcut uretimi tamamlayip yeni uretimi kilitler.\n\n"
        "• 5 Haneli Onay Kodu ve Yenileme:\nASTIAJ fabrikasindan kilit acma sifresi almak icin uretilen guvenlik kodu.\n\n"
        "• 5 Kademeli Taksit Yonetimi:\nASTIAJ 5 asamali taksit zincirini izleme ve yonetme."
    },
    [STR_HELP_F3_TITLE] = {"راهنمای حالت نگهداری و برودت", "Holding & Storage Cooling Guide (F3)", "Bekleme ve Sogutma Rehberi (F3)"},
    [STR_HELP_F3_BODY]  = {
        "• دوره استراحت کمپرسور سیلندر (دقیقه):\nمدت زمان وقفه بین بررسی‌های خودکار بافت بستنی در حالت آماده‌باش/نگهدارنده. پس از پایان این زمان، کنترلر وضعیت میکروسوئیچ کلاچ عقب سیلندر را می‌سنجد: اگر میکروسوئیچ فشرده باشد (بستنی سفت است)، فقط موتور پارو به مدت ۱۰ ثانیه می‌چرخد تا بافت بستنی یکدست و تازه شود، کمپرسور سیلندر خاموش می‌ماند و زمان‌سنجی دوره بعدی از انتهای چرخش پارو مجدداً آغاز می‌شود. در صورت شل شدن بستنی، چرخه شارژ سرما فعال می‌گردد (پیش‌فرض: 120 دقیقه).\n\n• سقف کارکرد کمپرسور سیلندر (دقیقه):\nحداکثر زمان مجاز روشن ماندن کمپرسور سیلندر در فاز نگهداری. اگر روی 0 تنظیم شود، کنترل توقف کاملاً خودکار و هوشمندانه بر اساس فشرده شدن کلاچ عقب (سفت شدن بستنی) یا رسیدن به دمای ایمن انجام می‌گیرد (پیش‌فرض: 0 دقیقه = کنترل خودکار با کلاچ).\n\n• تاخیر استارت نگهدارنده (ثانیه):\nفاصله زمانی چرخش اولیه پارو پیش از روشن شدن کمپرسور سیلندر هنگام شارژ سرما در حالت نگهداری جهت راه‌اندازی نرم و عدم شوک برق (پیش‌فرض: 30 ثانیه).\n\n• دمای استارت مخزن شیر (°C):\nآستانه دمایی گرم شدن مخزن بالا که با عبور دما از آن، سرمایش مخزن روشن می‌شود تا شیر خنک بماند (بازه: +4 الی +8 درجه - پیش‌فرض: +4.0°C).\n\n• دمای استپ مخزن شیر (°C):\nآستانه دمایی خنک شدن مخزن بالا که با رسیدن دما به آن سرمایش قطع می‌شود تا شیر یخ نزند (بازه: +1 الی +3 درجه - پیش‌فرض: +2.0°C).",

        "• Cylinder Comp Rest Cycle (min):\nInterval between auto-checks in Ready/Holding mode. When this timer expires, the controller checks the rear clutch microswitch: if firm, only paddle runs for 10 seconds to refresh texture, compressor stays OFF, and auto-check timer restarts. If softened, cooling begins (Default: 120 min).\n\n• Cylinder Comp Max Run (min):\nMaximum cooling run time per holding cycle. Setting to 0 relies strictly on rear microswitch clutch and cylinder thermocouple cut-off (Default: 0 min = auto clutch control).\n\n• Hold Start Delay (sec):\nDelay between paddle rotation and compressor start for soft-start protection against line surges (Default: 30 sec).\n\n• Hopper Start Temp (°C):\nThermostat upper threshold; holding cooling kicks in automatically when hopper warms up to preserve milk freshness (Default: +4.0°C).\n\n• Hopper Stop Temp (°C):\nThermostat lower threshold; holding cooling turns off to prevent milk freeze-up and feed blockage (Default: +2.0°C).",

        "• Silindir Kompr. Dinlenme (dk):\nBekleme modunda otomatik kontrol araligi. Sure doldugunda arka switch basiliysa (dondurma sert), sadece palet motoru 10 saniye doner, kompresor calismaz ve bir sonraki sure baslar (Varsayilan: 120 dk).\n\n• Silindir Kompr. Azami Calisma (dk):\nBekleme modunda silindir kompresor azami calisma suresi. 0 yapildiginda durus arka debriyaj switchine ve silindir termokupluna baglidir (Varsayilan: 0 dk = otomatik debriyaj).\n\n• Bekleme Kalkis Gecikmesi (sn):\nPalet kalkisi ile kompresor arasindaki yumusak yol verme gecikmesi (Varsayilan: 30 sn).\n\n• Sut Tanki Baslama Sicakligi (°C):\nUst sut haznesi baslama derecesi; sutun bozulmasini onlemek icin bekleme sogutmasi devreye girer (Varsayilan: +4.0°C).\n\n• Sut Tanki Durma Sicakligi (°C):\nUst sut haznesi durma derecesi; bekleme sogutmasi durarak sutun buz tutmasini engeller (Varsayilan: +2.0°C)."
    },
    [STR_HELP_F4_TITLE] = {"راهنمای وای‌فای و مانیتورینگ", "WiFi & Remote Monitoring (F4)", "WiFi ve Uzaktan Izleme (F4)"},
    [STR_HELP_F4_BODY]  = {
        "• اتصال مستقیم با گوشی (بدون مودم):\n"
        "در تنظیمات وای‌فای گوشی به شبکه زیر وصل شوید:\n"
        "نام شبکه: ASTIAJ-IceCream\n"
        "رمز عبور: 12345678\n"
        "سپس در مرورگر اینترنت آدرس زیر را باز کنید:\n"
        "http://192.168.4.1\n\n"
        "• اتصال از طریق مودم مغازه (شبکه محلی):\n"
        "با زدن دکمه اتصال، دستگاه به مودم وصل می‌شود. پس از اتصال، آی‌پی دستگاه در کادر بالای صفحه نمایش داده شده و از هر گوشی یا کامپیوتر متصل به همان مودم با وارد کردن آن آی‌پی (مانند http://192.168.1.50) وب‌پورتال در دسترس خواهد بود.\n\n"
        "• امکانات وب‌پورتال دستگاه:\n"
        "مانیتورینگ زنده دماها، کنترل رله‌ها، مشاهده شمارنده قیف و تنظیم تمام پارامترها بدون نیاز به نصب نرم‌افزار.",

        "• Direct Phone Connection (No Router):\n"
        "Connect phone WiFi to this network:\n"
        "SSID: ASTIAJ-IceCream\n"
        "Password: 12345678\n"
        "Then open web browser and go to:\n"
        "http://192.168.4.1\n\n"
        "• Local Router Connection:\n"
        "Connect to your shop WiFi router. The assigned IP will appear on screen. Browse to http://<Device-IP> from any phone or PC on the same WiFi network.\n\n"
        "• WebPortal Features:\n"
        "Live temperature monitoring, relay control, cone counters and parameter settings without installing any app.",

        "• Modemsiz Telefona Dogrudan Baglanti:\n"
        "Telefon WiFi ayarlarindan su aga baglanin:\n"
        "Ag Adi: ASTIAJ-IceCream\n"
        "Sifre: 12345678\n"
        "Ardindan tarayicida su adresi acin:\n"
        "http://192.168.4.1\n\n"
        "• Yerel Modem Uzerinden Baglanti:\n"
        "Cihazi dukkanin WiFi modemine baglayin. Ekranda gorunen IP adresini (http://<IP>) ayni agdaki cihazlardan acin.\n\n"
        "• WebPortal Ozellikleri:\n"
        "Uygulama yuklemeden canli sicaklik izleme, role kontrolu ve servis sayaci yonetimi."
    },
    [STR_HELP_F5_TITLE] = {"راهنمای تست و دیاگ ورودی و خروجی‌ها (F5)", "I/O Diagnostics Guide (F5)", "Giris/Cikis Diyagnostik Rehberi (F5)"},
    [STR_HELP_F5_BODY]  = {
        "• چراغ‌های ورودی دیجیتال (I0 تا I7):\nلامپ‌های سیگنال سبز نئونی که با تحریک فیزیکی هر سنسور روشن می‌شوند و به اپراتور اجازه تست سلامت سیم‌کشی را می‌دهند:\n• I0: سنسور القایی پولی گیربکس (پایش چرخش شفت پارو و تشخیص پارگی تسمه یا قفل گیربکس ظرف ۴ ثانیه)\n• I1: سنسور سطح مخزن شیر (هشدار اتمام مایه بستنی در بن‌ماری بالا)\n• I2: میکروسوئیچ عقب سیلندر (تشخیص سفتی و قوام نهایی بستنی جهت توقف کمپرسور)\n• I3: رله حرارتی / کنترل‌بار الکتروموتور پارو (قطع اضطراری)\n• I4: میکروسوئیچ اهرم جلو (شمارنده قیف‌ها و سنسور فرمان تخلیه بستنی)\n• I5: رله حرارتی / کنترل‌بار کمپرسور بزرگ (محافظت در برابر اضافه بار)\n• I6: استپ اضطراری قارچی پنل (E-Stop) جهت قطع آنی و بی‌درنگ کلیه رله‌ها\n• I7: شستی فیزیکی استارت تولید\n\n• تست دستی رله‌های قدرت (خروجی‌های Q0 تا Q6):\nدر وضعیت توقف کامل، لمس هر کلید باعث فعال شدن مستقل و ایمن رله مربوطه به مدت ۵ ثانیه می‌شود:\n• Q0: نگهدارنده\n• Q1: کمپرسور بزرگ برودت سیلندر انجماد\n• Q2: الکتروموتور پارو\n• Q3: پمپ کرم / سیروپ لبه رنگی\n• Q4: موتور پارو مخزن شیر بن‌ماری\n• Q5: شیر برقی هوای فشرده (شیر باد)\n• رله ریست: ایجاد پالس ۵۰۰ میلی‌ثانیه‌ای ریست مدار\n\n• پایش زنده سنسورهای دما (ترمینال‌های T1 و T2):\nنمایش لحظه‌ای دمای مخزن شیر (ترموکوپل T1) و دمای استوانه برودت سیلندر (ترموکوپل T2) بر حسب دهم درجه با تشخیص قطعی سنسور.",

        "• Digital Input Status (I0..I7):\nNeon green signal lamps reflecting real-time physical states of sensors and switches:\n• I0: Pulley Inductive Sensor (paddle rotation monitor & belt break detector within 4s)\n• I1: Milk Hopper Level Sensor (low milk warning)\n• I2: Rear Clutch Microswitch (ice cream hardness detector for compressor cut-off)\n• I3: Paddle Motor Overload relay\n• I4: Front Dispense Lever Microswitch (cone counter and dispense trigger)\n• I5: Main Compressor Overload relay\n• I6: Emergency Stop button (E-Stop, <20ms instant shutdown)\n• I7: Physical Start push button\n\n• Manual Relay Testing (Q0..Q6):\nIn stopped mode, touch buttons to independently test relays for 5 seconds:\n• Q0: Holding\n• Q1: Cylinder main compressor\n• Q2: Paddle gearbox motor\n• Q3: Cream / syrup injection pump\n• Q4: Hopper milk paddle motor\n• Q5: Compressed air valve\n• Reset Relay: 500ms safety reset pulse\n\n• Live Temperature Monitor (Terminals T1 & T2):\nReal-time temperatures for milk hopper (T1) and freezing cylinder (T2) with sensor break detection.",

        "• Dijital Giris Durumlari (I0..I7):\nFiziksel sensorler tetiklendiginde neon yesil lamba yanarak tesisat durumunu gosterir:\n• I0: Kasnak enduktif sensoru (palet donus takibi ve 4 sn icinde kayis kopma tespiti)\n• I1: Sut haznesi seviye sensoru\n• I2: Arka kivam mikroswitchi (dondurma sertligini algilayarak kompresoru durdurur)\n• I3: Palet motoru termik/asiri yuk rolesi\n• I4: On servis kolu mikroswitchi (kulah sayaci ve servis algilayici)\n• I5: Ana kompresor termik/asiri yuk rolesi\n• I6: Acil stop mantar butonu (E-Stop)\n• I7: Fiziksel baslat butonu\n\n• Manuel Role Cikis Testi (Q0..Q6):\nCihaz dururken her cikis 5 saniye guvenle calistirilarak elektrik panosu test edilir:\n• Q0: Bekleme\n• Q1: Silindir ana kompresor\n• Q2: Palet reduktor motoru\n• Q3: Kenar krema pompasi\n• Q4: Sut haznesi palet motoru\n• Q5: Hava elektrovalfi\n• Reset Rolesi: 500ms guvenlik reset pulsu\n\n• Sicaklik Takibi (T1 ve T2 Klemensleri):\nSut haznesi (T1) ve dondurma silindiri (T2) anlik sicaklik degerleri."
    },
    [STR_HELP_F6_TITLE] = {"راهنمای فروش، شارژ سرما و حفاظت ایمنی (F6)", "Dispense, Rechill & Safety Guide (F6)", "Servis, Sogutma ve Guvenlik Rehberi (F6)"},
    [STR_HELP_F6_BODY]  = {
        "• زمان کارکرد تخلیه بستنی (ثانیه):\nحداکثر زمان مجاز چرخش پارو هنگام پایین کشیدن اهرم خروجی بستنی. در صورت رها شدن اهرم، موتور پارو بلافاصله متوقف می‌شود (پیش‌فرض: 45 ثانیه).\n\n• زمان نگهداشتن اهرم جلو (ثانیه):\nمدت زمان باز ماندن مداوم اهرم جلو (I4) در فروش‌های حجیم یا پر کردن ظروف بستنی. پس از گذشت این زمان، اگر بستنی هنوز سفت باشد و میکروسوئیچ عقب فشرده بماند، فقط موتور پارو به مدت ۱۰ ثانیه می‌چرخد و کمپرسور مطلقاً روشن نمی‌شود؛ اما اگر بستنی شل شده باشد، چرخه کامل انجماد فعال می‌گردد (پیش‌فرض: 6 ثانیه).\n\n• تعداد پالس اهرم برای شارژ سرما:\nتعداد قیف‌های فروخته شده با اهرم جلو (I4) پیش از صدور فرمان شارژ سرما. همانند نگهداشتن اهرم، در صورت سفت بودن بستنی فقط ۱۰ ثانیه چرخش پارو انجام می‌شود و در صورت شل شدن، کمپرسور وارد مدار می‌گردد (پیش‌فرض: 3 قیف).\n\n• حداکثر زمان انجماد سیلندر (دقیقه):\nسقف زمانی مجاز کارکرد مداوم کمپرسور بزرگ در یک سیکل تولید. اگر میکروسوئیچ عقب خراب شود، رگلاژ فنر کلاچ به هم بخورد یا گاز تبرید نشت کند، کنترلر برای نجات موتور و کمپرسور پس از این زمان برق سیستم را قطع کرده و آلارم توقف صادر می‌کند (پیش‌فرض: 15 دقیقه).\n\n• زمان پایش سنسور سطح شیر (ثانیه):\nمدت زمان پایش تداوم سیگنال سنسور شناور شیر پیش از صدور هشدار کمبود شیر در مخزن بن‌ماری (پیش‌فرض: 25 ثانیه).\n\n• کارت تایمرهای ثابت قفل سخت‌افزار کارخانه:\nنمایش مقادیر غیرقابل تغییر شامل پالس ریست 500ms، فیلتر دیبانس نویز ورودی‌ها 70ms، و حداقل دمای مجاز سیلندر (-22°C) برای حفاظت مکانیکی از تیغه‌های پارو و جداره سیلندر.",

        "• Dispense Run Time (sec):\nMaximum paddle rotation time when pulling the dispensing lever. Paddle stops immediately upon lever release (Default: 45 sec).\n\n• Bulk Dispense Hold (sec):\nContinuous lever hold duration triggering rechill. In Ready mode, if ice cream is firm (rear micro active), ONLY paddle runs for 10s and compressor stays OFF. If softened, full freeze recharge begins (Default: 6 sec).\n\n• Scoops for Rechill:\nNumber of dispensed cones triggering rechill evaluation. Like lever hold, if firm only 10s paddle runs; if softened compressor starts (Default: 3 cones).\n\n• Max Freeze Timeout (min):\nSafety cutoff protecting compressor from running continuously if rear clutch switch fails or refrigerant is low (Default: 15 min).\n\n• Milk Level Probe Timeout (sec):\nDebounce monitoring time for hopper float level probe before alerting low milk (Default: 25 sec).\n\n• Factory Fixed Safeguards:\nDisplay of hardware-locked interlocks: 500ms reset pulse, 70ms input debounce, and -22C freeze safety limit.",

        "• Servis Calisma Suresi (sn):\nDondurma kolu cekildiginde paletin azami calisma suresi. Kol birakildiginda palet derhal durur (Varsayilan: 45 sn).\n\n• Toptan Servis Kol Suresi (sn):\nKolu basili tutma suresi. Dondurma sertse (arka switch basiliysa) sadece palet 10 sn calisir, yumusamissa tam sogutma baslar (Varsayilan: 6 sn).\n\n• Sogutma Kulah Sayisi:\nKac kulah satis sonrasi sogutma kontrolu yapilacagini belirler. Sertse 10 sn palet doner, yumusaksa kompresor calisir (Varsayilan: 3 kulah).\n\n• Maksimum Donma Suresi (dk):\nArka switch basmazsa veya gaz eksikse kompresorun guvenlik durma suresi (Varsayilan: 15 dk).\n\n• Sut Seviye Filtre Suresi (sn):\nUst haznede sut seviyesi samandirasinin alarm vermeden onceki filtre suresi (Varsayilan: 25 sn).\n\n• Fabrika Kilitli Emniyet Kartlari:\n500ms reset pulsu, 70ms giris filtresi ve -22C donma koruma limitlerini gosterir."
    },
    [STR_HELP_F7_TITLE] = {"راهنمای شناسنامه سیستم و آپدیت ابری", "System Info & Cloud OTA Guide (F7)", "Sistem Bilgisi ve OTA Rehberi (F7)"},
    [STR_HELP_F7_BODY]  = {
        "• شناسنامه و نگارش فریمور:\nنمایش نسخه رسمی سیستم‌عامل، تاریخ و زمان کامپایل، مدل کنترلر صنعتی آستیاج و سلامت حافظه‌های داخلی RAM و PSRAM.\n\n"
        "• به‌روزرسانی اینترنتی ابری (Cloud OTA):\nاتصال به سرورهای ابری امن آستیاج جهت دریافت آخرین نسخه‌های نرم‌افزار بدون نیاز به باز کردن درب دستگاه یا کابل پروگرم.\n\n"
        "• جعبه سیاه و ثبت خطاها (ISA-18.2):\nذخیره رویدادها، قطعی‌های برق، افت ولتاژ، و هشدارهای ثبت شده همراه با ساعت دقیق وقوع در حافظه فلش پایدار.",

        "• System Identity & Firmware:\nDisplays operating system release, build timestamp, controller model, and RAM/PSRAM diagnostic health.\n\n"
        "• Cloud OTA Updates:\nDirect download of validated firmware updates from ASTIAJ cloud servers without cables or PC.\n\n"
        "• Blackbox Fault Logging (ISA-18.2):\nStores timestamps, power brownouts, and alarm codes securely in non-volatile flash memory.",

        "• Sistem Kimligi ve Yazilim Surumu:\nIsletim sistemi versiyonu, derleme tarihi, kontrolor modeli ve dahili bellek saglik durumu.\n\n"
        "• Bulut OTA Guncelleme:\nASTIAJ bulut sunuculari uzerinden kablosuz ve bilgisayarsiz dogrudan yazilim yukleme.\n\n"
        "• Kara Kutu Hata Kayitlari (ISA-18.2):\nElektrik kesintileri, arizalar ve uyarilar zaman damgasiyla kalici hafizada saklanir."
    },

    /* Dashboard Extras & Notifications */
    [STR_HOPPER_LABEL]         = {"شیر:", "Hopper:", "Sut:"},
    [STR_CYL_LABEL]            = {"سیلندر:", "Cylinder:", "Silindir:"},
    [STR_SENSOR_NC]            = {"قطع", "NC", "Kesik"},
    [STR_HOPPER_SENSOR_EN]     = {"سنسور مخزن شیر فعال شد", "Hopper sensor enabled", "Sut sensoru aktif"},
    [STR_HOPPER_SENSOR_DIS]    = {"سنسور مخزن شیر غیرفعال شد", "Hopper sensor disabled", "Sut sensoru pasif"},
    [STR_MODE_PADDLE_BEATER]   = {"پارو", "Paddle", "Palet"},
    [STR_READY_STANDBY]        = {"دستگاه آماده به کار", "System Ready", "Sistem Hazir"},
    [STR_READY_DISPENSE]       = {"آماده توزیع (۱۰۰ درصد)", "Ready to Dispense (100%)", "Servise Hazir (%100)"},
    [STR_FREEZING_PCT]         = {"انجماد بستنی:", "Freezing:", "Dondurma:"},
    [STR_COMP_COOLDOWN_PREFIX] = {"تعادل گاز کمپرسور:", "Comp. Cooldown:", "Kompresor Dengesi:"},
    [STR_WASH_DRAIN_PREFIX]    = {"تخلیه سیلندر:", "Cyl. Drain:", "Silindir Tahliye:"},
    [STR_WASH_AUTO_PREFIX]     = {"شستشوی خودکار:", "Auto Wash:", "Otomatik Yikama:"},

    /* Status Badges */
    [STR_STATUS_ON]            = {"روشن", "ON", "ACIK"},
    [STR_STATUS_OFF]           = {"خاموش", "OFF", "KAPALI"},
    [STR_STATUS_READY]         = {"آماده", "READY", "HAZIR"},

    /* F2 Day-Lock & Leasing */
    [STR_DL_TITLE]             = {"روزشمار و قفل اقساط", "Lease & Day-Lock", "Kiralama ve Gun Kilidi"},
    [STR_DL_STATUS_LOCKED]     = {"وضعیت: دستگاه قفل است", "Status: System Locked", "Durum: Sistem Kilitli"},
    [STR_DL_STATUS_DAYS]       = {"%d روز اعتبار باقی مانده", "Credit: %d days left", "Kredi %d gun kaldi"},
    [STR_DL_STATUS_UNLIMITED]  = {"کاربری آزاد (نامحدود)", "Unlimited License", "Sinirsiz Kullanim"},
    [STR_DL_CHALLENGE]         = {"کد چالش دستگاه", "Challenge Code", "Cihaz Onay Kodu"},
    [STR_DL_EXTEND_BTN]        = {"تمدید و ورود رمز", "Renew & Enter Key", "Yenile ve Kod Gir"},
    [STR_DL_5TIMERS_BTN]       = {"تنظیم زنجیره اقساط", "Installment Chain", "Taksit Zinciri"},
    [STR_DL_SUPPORT]           = {"پشتیبانی: 09102367062\nASTIAJ.ir", "Support: 09102367062\nASTIAJ.ir", "Destek: 09102367062\nASTIAJ.ir"},
    [STR_DL_EMERGENCY]         = {"کد اضطراری: ۱۱۲۲۳۳", "Emergency: 112233", "Acil Kod: 112233"},
    [STR_DL_TAMPER_ALERT]      = {"[!] خطای دستکاری و عقب کشیدن ساعت سخت‌افزاری", "[!] Hardware RTC Tamper Detected", "[!] Donanim Saati Manipulasyonu"},
    [STR_DL_LOCKED_TITLE]      = {"[قفل] دستگاه قفل است - سیستم روزشمار آستیاج", "[LOCKED] System Locked - ASTIAJ Day-Lock", "[KILITLI] Sistem Kilitli - ASTIAJ Gun Kilidi"},
    [STR_DL_HELP_TITLE]        = {"راهنمای پشتیبانی و فعالسازی روزشمار آستیاج", "ASTIAJ Day-Lock Support Guide", "ASTIAJ Gun Kilidi Destek Rehberi"},
    [STR_DL_HELP_MSG]          = {
        "• موعد یکی از مراحل اقساط دستگاه فرا رسیده است.\n"
        "• کد چالش ۵ رقمی مندرج در کادر آبی را یادداشت نمایید.\n"
        "• با پشتیبانی کارخانه (09102367062) تماس گرفته و با اعلام کد، رمز فعال‌سازی را دریافت نمایید.",

        "• An installment period has expired.\n"
        "• Note the 5-digit challenge code shown in the blue card.\n"
        "• Call factory support at 09102367062 with this code to receive the unlock key.",

        "• Taksit vadelerinden biri dolmustur.\n"
        "• Mavi kutudaki 5 haneli onay kodunu not ediniz.\n"
        "• 09102367062 numarali fabrika destegi arayarak kodu bildirip acma kodunu aliniz."
    },
    [STR_DL_HELP_SITE]         = {"وب‌سایت رسمی: ASTIAJ.ir", "Official Website: ASTIAJ.ir", "Resmi Web Sitesi: ASTIAJ.ir"},
    [STR_DL_SYNCING_TIME]      = {"در حال دریافت ساعت...", "Syncing time...", "Saat esitleniyor..."},
    [STR_DL_NO_WIFI]           = {"وای‌فای متصل نیست؛ به تنظیمات بروید", "Wi-Fi not connected; go to settings", "WiFi bagli degil; ayarlara gidin"},
    [STR_DL_GRACE_USED]        = {"این کد اضطراری قبلاً مصرف شده است!", "This emergency code was already used!", "Bu acil kod daha once kullanildi!"},
    [STR_DL_INVALID_PIN]       = {"رمز نامعتبر است؛ مجدداً تلاش نمایید", "Invalid code; please try again", "Gecersiz kod; lutfen tekrar deneyin"},

    /* F4 WiFi Extras */
    [STR_WIFI_NO_NETS]         = {"شبکه‌ای یافت نشد. جهت جستجو کلید «جستجو» را لمس کنید.", "No networks found. Touch Scan to search.", "Ag bulunamadi. Aramak icin Ara tusuna basin."},
    [STR_WIFI_SCANNING]        = {"در حال جستجوی شبکه‌های وای‌فای...", "Scanning for Wi-Fi networks...", "WiFi aglari taraniyor..."},
    [STR_WIFI_PASS_TITLE]      = {"ورود رمز عبور اتصال به وای‌فای:", "Enter WiFi Network Password:", "WiFi Ag Sifresini Girin:"},
    [STR_WIFI_PASS_PLACEHOLDER]= {"WiFi Password...", "WiFi Password...", "WiFi Sifresi..."},

    /* Fault History Modal */
    [STR_FAULT_HIST_TITLE]     = {"تاریخچه ثبت ده رویداد اخیر خطاها", "Recent Faults History (ISA-18.2)", "Son 10 Hata Kayit Gecmisi"},
    [STR_FAULT_HIST_EMPTY]     = {"هیچ خطایی در حافظه ثبت نشده است - وضعیت پایدار", "No faults recorded in memory - System normal", "Hafizada kayitli hata yok - Sistem stabil"},
    [STR_FAULT_HIST_CLEAR]     = {"پاکسازی حافظه", "Clear Log", "Hafizayi Temizle"},
    [STR_FAULT_HIST_CLOSE]     = {"متوجه شدم", "Got It", "Anladim"},

    /* Screensaver */
    [STR_SS_STANDBY]           = {"سیستم در حالت استراحت", "System in Rest Mode", "Sistem Dinlenme Modunda"},
    [STR_SS_READY_DISPENSE]    = {"بستنی آماده توزیع", "Ice Cream Ready to Dispense", "Dondurma Servise Hazir"},
    [STR_SS_DISPENSING]        = {"در حال توزیع و استخراج", "Dispensing in Progress", "Dondurma Cekiliyor"},
    [STR_SS_PROD_CYCLE]        = {"سیکل سرمایش و تولید فعال", "Freezing & Production Active", "Sogutma ve Uretim Aktif"},
    [STR_SS_PADDLE_ACTIVE]     = {"پارو فعال (شستشو)", "Paddle Active (Wash)", "Palet Aktif (Yikama)"},
    [STR_SS_WAKE_HINT]         = {"برای خروج از حالت استراحت صفحه را لمس کنید", "Touch screen to exit rest mode", "Dinlenmeden cikmak icin ekrana dokunun"},

    /* Manager PIN Dialog */
    [STR_PIN_TITLE]            = {"ورود به بخش مدیریت و اقساط", "Manager & Leasing Access", "Yonetici ve Kira Girisi"},
    [STR_PIN_PROMPT]           = {"رمز عبور", "Enter PIN", "Sifre Girin"},
    [STR_PIN_CLEAR]            = {"پاک", "Clear", "Sil"},
    [STR_PIN_ENTER]            = {"ورود", "Enter", "Giris"},

    /* Counter Details Dialog */
    [STR_CNT_STATS_TITLE]      = {"آمار جامع تولید، دخل و کارکرد دستگاه", "Production & Revenue Statistics", "Uretim ve Gelir Istatistikleri"},
    [STR_CNT_SHIFT_CONE]       = {"شمارنده شیفت (قیف)", "Shift Counter (Cones)", "Vardiya Sayaci (Kulah)"},
    [STR_CNT_SHIFT_REV]        = {"دخل شیفت:", "Shift Rev:", "Vardiya Gelir:"},
    [STR_CNT_SHIFT_RST]        = {"صفر کردن آمار شیفت", "Reset Shift Stats", "Vardiyayi Sifirla"},
    [STR_CNT_TOTAL_CONE]       = {"شمارنده کل (طول عمر)", "Total Odometer (Lifetime)", "Toplam Sayac (Omur)"},
    [STR_CNT_TOTAL_REV]        = {"دخل کل:", "Total Rev:", "Toplam Gelir:"},
    [STR_CNT_ODOMETER]         = {"کیلومترشمار دائمی", "Permanent Lifetime Meter", "Kalici Omur Sayaci"},
    [STR_CNT_PRICE_NOTE]       = {"نرخ مصوب هر قیف (تنظیم در وب‌اپ)", "Unit price per cone (set in WebApp)", "Kulah birim fiyati (WebApp ayari)"},
    [STR_CNT_DRAIN_START]      = {"تخلیه سیلندر (۳ دقیقه)", "Drain Cylinder (3 min)", "Silindiri Bosalt (3 dk)"},
    [STR_CNT_DRAIN_STOP]       = {"توقف تخلیه سیلندر", "Stop Cylinder Drain", "Bosaltmayi Durdur"},
    [STR_CLOSE_DIALOG]         = {"بستن پنجره", "Close", "Kapat"},

    /* Interactive Fault Dialog */
    [STR_FAULT_CAUSE_HDR]      = {"علت وقوع خطا:", "Root Cause:", "Hata Nedeni:"},
    [STR_FAULT_SOL_HDR]        = {"راهنمای رفع عیب و اقدامات اصلاحی:", "Troubleshooting & Corrective Action:", "Cozum ve Duzeltici Islemler:"},
    [STR_FAULT_RETRY]          = {"تلاش مجدد", "Retry", "Tekrar Dene"},
    [STR_FAULT_DISMISS]        = {"بستن", "Dismiss", "Kapat"},

    /* Weekday Names */
    [STR_WDAY_SUN]             = {"یک شنبه", "Sunday", "Pazar"},
    [STR_WDAY_MON]             = {"دو شنبه", "Monday", "Pazartesi"},
    [STR_WDAY_TUE]             = {"سه شنبه", "Tuesday", "Sali"},
    [STR_WDAY_WED]             = {"چهار شنبه", "Wednesday", "Carsamba"},
    [STR_WDAY_THU]             = {"پنج شنبه", "Thursday", "Persembe"},
    [STR_WDAY_FRI]             = {"جمعه", "Friday", "Cuma"},
    [STR_WDAY_SAT]             = {"شنبه", "Saturday", "Cumartesi"},

    /* Unsaved Changes Confirmation Dialog */
    [STR_UNSAVED_TITLE]        = {"تنظیمات ذخیره نشده است", "Unsaved Changes", "Kaydedilmemis Degisiklikler"},
    [STR_UNSAVED_MSG]          = {"تغییرات اعمال‌شده هنوز ذخیره نشده است.\nآیا مایلید ابتدا ذخیره کنید و سپس خارج شوید؟", "You have unsaved changes in settings.\nDo you want to save before leaving?", "Ayarlarda kaydedilmemis degisiklikler var.\nCikmadan once kaydetmek istiyor musunuz?"},
    [STR_UNSAVED_SAVE_EXIT]    = {"ذخیره و خروج", "Save & Exit", "Kaydet ve Cik"},
    [STR_UNSAVED_DISCARD_EXIT] = {"خروج بدون ذخیره", "Discard & Exit", "Kaydetmeden Cik"},
    [STR_UNSAVED_CANCEL]       = {"انصراف", "Cancel", "Iptal"},

    /* Live Process Monitoring Card */
    [STR_MON_IDLE]             = {"دستگاه در حالت آماده کار", "Machine Ready", "Makine Hazir"},
    [STR_MON_NEXT_SELECT]      = {"مرحله بعد: انتخاب حالت کاری", "Next: Select a Process", "Sonraki: Islem Seciniz"},
    [STR_MON_PROD_ACTIVE]      = {"تولید بستنی فعال", "Ice Cream Making Active", "Dondurma Uretimi Aktif"},
    [STR_MON_PROD_NEXT]        = {"مرحله بعد: قطع کمپرسور در برودت مطلوب", "Next: Comp Cutoff at Cold", "Sonraki: Yeterli Soguklukta Durma"},
    [STR_MON_HOLD_ACTIVE]      = {"نگهدارنده فعال (دمای مخزن)", "Hold Storage Active", "Bekleme Modu Aktif"},
    [STR_MON_HOLD_NEXT]        = {"مرحله بعد: کنترل دوره‌ای دما", "Next: Periodic Temp Control", "Sonraki: Periyodik Sicaklik Kontrolu"},
    [STR_MON_WASH_ACTIVE]      = {"شستشوی خودکار فعال", "Auto Wash Active", "Otomatik Yikama Aktif"},
    [STR_MON_WASH_NEXT]        = {"مرحله بعد: توقف و تخلیه مخزن", "Next: Stop & Tank Drain", "Sonraki: Durdurma ve Tahliye"},
    [STR_MON_DRAIN_ACTIVE]     = {"تخلیه سیلندر انجماد فعال", "Cylinder Drain Active", "Silindir Bosaltma Aktif"},
    [STR_MON_DRAIN_NEXT]       = {"مرحله بعد: پایان فرآیند شستشو", "Next: Complete Cleaning", "Sonraki: Temizleme Bitisi"},
    [STR_MON_DISPENSE_ACTIVE]  = {"توزیع بستنی در حال انجام", "Dispensing Ice Cream", "Dondurma Servisi Yapiliyor"},
    [STR_MON_DISPENSE_NEXT]    = {"مرحله بعد: قطع پس از رهایی اهرم", "Next: Stop on Lever Release", "Sonraki: Kol Birakildiginda Durma"},
    [STR_MON_COOLDOWN_ACTIVE]  = {"تعادل گاز کمپرسور", "Compressor Gas Cooldown", "Kompresor Gaz Dengesi"},
    [STR_MON_COOLDOWN_NEXT]    = {"مرحله بعد: استارت مجدد کمپرسور", "Next: Compressor Restart", "Sonraki: Kompresor Yeniden Baslatma"},
    [STR_SPLASH_INIT]          = {"در حال راه‌اندازی و بررسی سیستم...", "System Initializing & Self-Test...", "Sistem Baslatiliyor ve Kontrol Ediliyor..."},
    [STR_BOOT_ALERT_TITLE]     = {"هشدار سلامت سخت‌افزار", "Hardware Health Warning", "Donanim Saglik Uyarisi"},
    [STR_BOOT_ALERT_SUB]       = {"خطای سخت‌افزاری در تست اولیه شناسایی شد", "Hardware issue detected during self-test", "Baslangic testinde donanim hatasi tespit edildi"},
    [STR_BOOT_ALERT_VIEW]      = {"مشاهده و راهکار", "View Solutions", "Cozumleri Gor"},
    [STR_BOOT_ALERT_DISMISS]   = {"بستن", "Dismiss", "Kapat"},
    [STR_BOOT_ALERT_AUTO]      = {"بستن خودکار در %d ثانیه", "Auto-close in %d s", "Otomatik kapanis %d sn"},

    /* Counter Reset Confirmation Dialog */
    [STR_RST_CNT_TITLE]        = {"تایید صفر کردن شمارنده", "Reset Counter Confirmation", "Sayac Sifirlama Onayi"},
    [STR_RST_CNT_MSG]          = {"آیا از صفر کردن شمارنده روزانه اطمینان دارید؟", "Are you sure you want to reset the daily counter?", "Gunluk uretim sayacini sifirlamak istediginizden emin misiniz?"},
    [STR_RST_CNT_CONFIRM]      = {"بله، صفر شود", "Yes, Reset", "Evet, Sifirla"},

    /* Boot WiFi Prompt Dialog */
    [STR_BOOT_WIFI_PROMPT_TITLE] = {"اتصال به شبکه وای‌فای", "Connect to Wi-Fi", "Wi-Fi Agina Baglan"},
    [STR_BOOT_WIFI_PROMPT_MSG]   = {"دستگاه به شبکه وای‌فای متصل نیست.\nجهت فعال‌سازی خدمات هوشمند و به‌روزرسانی فریمور، دستگاه را متصل نمایید.", "Device is not connected to Wi-Fi.\nPlease connect to enable cloud features and updates.", "Cihaz Wi-Fi baglantisi yok.\nBulut hizmetleri ve guncellemeler icin baglanin."},
    [STR_BOOT_WIFI_BTN_LATER]    = {"بعداً / انصراف", "Later / Cancel", "Sonra / Iptal"},
    [STR_BOOT_WIFI_BTN_GOTO]     = {"تنظیمات وای‌فای", "Wi-Fi Settings", "Wi-Fi Ayarlari"},

    /* OTA & Firmware Update */
    [STR_OTA_UP_TO_DATE]          = {"آپدیت", "Latest", "Guncel"},
    [STR_OTA_CARD_UPDATE_BTN]     = {"بروزرسانی", "Update", "Guncelle"},
    [STR_OTA_APP_IS_UPDATED]      = {"برنامه شما آپدیت است", "Your app is up to date", "Uygulamaniz guncel"},
    [STR_BOOT_OTA_PROMPT_TITLE]   = {"به‌روزرسانی نرم‌افزار", "Software Update", "Yazilim Guncellemesi"},
    [STR_BOOT_OTA_PROMPT_MSG]     = {"نسخه جدید %s بر روی سرور موجود است.\nآیا مایل به دریافت و نصب برنامه هستید؟", "New version %s is available on server.\nDo you want to download and install?", "Sunucuda yeni %s surumu mevcut.\nIndirip yuklemek istiyor musunuz?"},
    [STR_BOOT_OTA_BTN_INSTALL]    = {"دریافت و نصب", "Download & Install", "Indir ve Yukle"},
    [STR_BOOT_OTA_BTN_LATER]      = {"انصراف", "Dismiss", "Iptal"},
    [STR_SETTINGS_SAVED]          = {"تنظیمات با موفقیت ذخیره شد", "Settings saved successfully", "Ayarlar basariyla kaydedildi"},
    [STR_WIFI_CONNECT_FIRST]      = {"ابتدا به شبکه وای‌فای متصل شوید!", "Connect to Wi-Fi first!", "Once Wi-Fi agina baglanin!"},

    /* New Features */
    [STR_FIXED_ANTI_CYCLE]        = {"استارت مجدد کمپرسور", "Anti-Short Cycle", "Kompresor Gecikmesi"},
    [STR_PULLEY_SENSOR]           = {"سنسور مجاورتی پولی", "Pulley Sensor", "Kasnak Sensoru"},
    [STR_PULLEY_ENABLED]          = {"فعال", "Active", "Aktif"},
    [STR_PULLEY_DISABLED]         = {"غیرفعال", "Disabled", "Devre Disi"},
    [STR_CALIBRATE_TEMP]          = {"کالیبراسیون و جبران خطای دما", "Temperature Calibration", "Sicaklik Kalibrasyonu"},
    [STR_RAW_TEMP]                = {"دمای خام سنسور", "Raw Sensor Temp", "Ham Sensor Degeri"},
    [STR_CAL_OFFSET]              = {"میزان تصحیح", "Offset Adjustment", "Düzeltme Degeri"},
    [STR_FINAL_TEMP]              = {"دمای کالیبره‌شده", "Calibrated Output", "Kalibre Deger"},
    [STR_OVERLOAD_POLARITY]       = {"تنظیم پولاریته رله کنترل‌بار", "Overload Relay Polarity", "Asiri Yuk Kutup Ayari"},
    [STR_POLARITY_NO]             = {"N.O. (کنتاکت باز - وصل در خطا)", "N.O. (Normally Open)", "N.O. (Normalde Acik)"},
    [STR_POLARITY_NC]             = {"N.C. (کنتاکت بسته - قطع در خطا)", "N.C. (Normally Closed)", "N.C. (Normalde Kapali)"},
    [STR_DISABLE_PULLEY_BTN]      = {"غیرفعال سازی سنسور پولی", "Disable Pulley Sensor", "Kasnak Sensorunu Kapat"},
    [STR_CAL_RESET_ZERO]          = {"ریست به صفر", "Reset to 0", "Sifirla"},
    [STR_CAL_SAVE]                = {"ذخیره کالیبراسیون", "Save Calibration", "Kalibrasyonu Kaydet"},
    [STR_FACTORY_RST_TITLE]       = {"بازنشانی تنظیمات کارخانه", "Factory Reset", "Fabrika Ayarlarina Donus"},
    [STR_FACTORY_RST_DESC]        = {"کلیه پارامترهای زمان‌بندی، دمای برودت و تنظیمات سیستم به مقادیر اولیه کارخانه بازگردانی می‌شوند. کیلومترشمار کل دستگاه حفظ خواهد شد.", "All timing parameters, refrigeration setpoints, and system settings will be restored to factory defaults. Total machine odometer is preserved.", "Tum zamanlama parametreleri, sogutma ve sistem ayarlari fabrika varsayilanlarina dondurulecektir. Toplam makine sayaci korunacaktir."},
    [STR_FACTORY_RST_CONFIRM]     = {"بله، بازنشانی شود", "Yes, Restore Defaults", "Evet, Varsayilana Don"},
    [STR_FACTORY_RST_CANCEL]      = {"انصراف", "Cancel", "Iptal"},
    [STR_FACTORY_RST_AUTO_CLOSE]  = {"بستن خودکار در %d ثانیه", "Auto-close in %d s", "Otomatik kapanma (%d sn)"},
    [STR_FACTORY_RST_AUTO_PAUSED] = {"بستن خودکار در %d ثانیه (متوقف شد)", "Auto-close in %d s (Paused)", "Otomatik kapanma (%d sn - Durduruldu)"},
    [STR_FACTORY_RST_DONE]        = {"تنظیمات کارخانه اعمال شد", "Factory defaults restored", "Fabrika ayarlarina donuldu"},
    [STR_FACTORY_RST_BTN]         = {"بازنشانی تنظیمات کارخانه", "Factory Reset", "Fabrika Ayarlarina Donus"},

    /* Warranty Activation Subsystem */
    [STR_WARR_ACTIVATE_BTN]   = {"فعالسازی گارانتی", "Activate Warranty", "Garanti Etkinlestir"},
    [STR_WARR_ACTIVE_TITLE]   = {"گارانتی فعال است", "Warranty Active", "Garanti Aktif"},
    [STR_WARR_TITLE]          = {"ثبت و فعالسازی گارانتی", "Warranty Activation", "Garanti Etkinlestirme"},
    [STR_WARR_BUYER_NAME]     = {"نام خریدار", "Buyer Name", "Alici Adi"},
    [STR_WARR_CITY]           = {"شهر محل نصب", "City", "Sehir"},
    [STR_WARR_PHONE]          = {"شماره همراه", "Phone Number", "Telefon"},
    [STR_WARR_SUBMIT]         = {"تایید و فعالسازی", "Confirm & Activate", "Onayla ve Etkinlestir"},
    [STR_WARR_DATE_LBL]       = {"تاریخ ثبت:", "Date:", "Tarih:"},
    [STR_WARR_CODE_LBL]       = {"کد مشتری:", "Customer Code:", "Musteri Kodu:"},
    [STR_WARR_EXPIRED_TITLE]  = {"اتمام مهلت تست اولیه", "Initial Test Grace Expired", "Test Suresi Doldu"},
    [STR_WARR_EXPIRED_MSG]    = {"مهلت تست اولیه بدون فعال‌سازی گارانتی به پایان رسید.\nکلیه خروجی‌ها قفل گردیدند و کلیدها غیرفعال می‌باشند.\nجهت راه‌اندازی و استفاده دائم، گارانتی را فعال نمایید.", "Initial test grace period expired without warranty activation.\nAll production outputs and keys are locked.\nPlease activate warranty to enable full operation.", "Garanti etkinlestirilmeden ilk test suresi doldu.\nTum cikislar ve tuslar kilitlendi.\nCihazi calistirmak icin garantiyi etkinlestiriniz."},
    [STR_WARR_DISMISS_BTN]    = {"متوجه شدم", "Dismiss", "Anladim"},
    [STR_WARR_NO_NET_TITLE]   = {"عدم اتصال به اینترنت", "No Internet Connection", "Internet Yok"},
    [STR_WARR_NO_NET_MSG]     = {"جهت ثبت رسمی گارانتی، اتصال به اینترنت الزامی است.\nمی‌توانید به وای‌فای متصل شوید یا با رمز مدیر فعالسازی آفلاین نمایید.", "Internet connection required to activate warranty.\nConnect to Wi-Fi or activate offline with Manager PIN.", "Garanti icin internet gereklidir.\nWi-Fi baglanin veya Yonetici PIN ile acin."},
    [STR_WARR_GOTO_WIFI]      = {"اتصال به وای‌فای", "Wi-Fi Settings", "Wi-Fi Ayarlari"},
    [STR_WARR_ADMIN_OVERRIDE] = {"رمز مدیر (آفلاین)", "Manager PIN (Offline)", "Yonetici PIN (Cevrimdisi)"},
    [STR_WARR_INVALID_INPUT]  = {"لطفاً نام، شهر و شماره همراه را کامل وارد فرمایید", "Please enter valid name, city and phone number", "Lutfen ad, sehir ve telefon numarasini dogru girin"},
    [STR_WARR_ACTIVATED_ONLINE]  = {"گارانتی دستگاه فعال و پیامک به مدیریت ارسال گردید", "Warranty activated and SMS sent to management", "Garanti aktif edildi ve yonetime SMS gonderildi"},
    [STR_WARR_ACTIVATED_OFFLINE] = {"گارانتی آفلاین با رمز مدیر فعال گردید", "Offline warranty activated with Manager PIN", "Yonetici PIN ile cevrimdisi garanti aktif edildi"},
    [STR_WARR_PH_NAME]           = {"نام کامل خریدار...", "Full buyer name...", "Alici tam adi..."},
    [STR_WARR_PH_CITY]           = {"شهر محل نصب...", "Installation city...", "Kurulum sehri..."},
    [STR_WARR_PH_PHONE]          = {"شماره همراه خریدار...", "Phone number...", "Telefon numarasi..."},
};

void ui_lang_init(void)
{
    int32_t saved = storage_get_i32("ui_lang", 0);
    if (saved >= 0 && saved <= 2) {
        s_current_lang = (ui_lang_t)saved;
    } else {
        s_current_lang = UI_LANG_FA;
    }
    ESP_LOGI(TAG, "UI Language initialized: %d", (int)s_current_lang);
}

ui_lang_t ui_lang_get(void)
{
    return s_current_lang;
}

void ui_lang_set(ui_lang_t lang)
{
    if (lang > UI_LANG_TR) lang = UI_LANG_FA;
    s_current_lang = lang;
    storage_set_i32("ui_lang", (int32_t)lang);
    ESP_LOGI(TAG, "UI Language set to: %d", (int)lang);
    ui_lang_trigger_refresh();
}

const char *ui_lang_str(ui_str_key_t key)
{
    if (key >= STR_COUNT) return "";
    int lang_idx = (int)s_current_lang;
    if (lang_idx < 0 || lang_idx > 2) lang_idx = 0;
    const char *s = s_dict[key][lang_idx];
    return s ? s : "";
}

const lv_font_t *ui_lang_font_14(void)
{
    if (s_current_lang == UI_LANG_FA) {
        return &ui_font_kalame_bold_14;
    }
    return &lv_font_montserrat_14;
}

const lv_font_t *ui_lang_font_16(void)
{
    if (s_current_lang == UI_LANG_FA) {
        return &ui_font_yekan_16;
    }
    return &lv_font_montserrat_14;
}

const lv_font_t *ui_lang_font_20(void)
{
    if (s_current_lang == UI_LANG_FA) {
        return &ui_font_yekan_20;
    }
    return &lv_font_montserrat_16;
}

const lv_font_t *ui_lang_font_24(void)
{
    if (s_current_lang == UI_LANG_FA) {
        return &ui_font_yekan_24;
    }
    return &lv_font_montserrat_24;
}

lv_base_dir_t ui_lang_base_dir(void)
{
    return (s_current_lang == UI_LANG_FA) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR;
}

void ui_lang_register_refresh_cb(ui_lang_refresh_cb_t cb)
{
    s_refresh_cb = cb;
}

static void ui_lang_refresh_async_cb(void *arg)
{
    (void)arg;
    if (s_refresh_cb) {
        s_refresh_cb();
    }
}

void ui_lang_trigger_refresh(void)
{
    if (s_refresh_cb) {
        if (lvgl_lock(pdMS_TO_TICKS(50))) {
            s_refresh_cb();
            lvgl_unlock();
        } else {
            lv_async_call(ui_lang_refresh_async_cb, NULL);
        }
    }
}
