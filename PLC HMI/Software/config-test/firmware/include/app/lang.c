#include "lang.h"

Lang g_lang = LANG_FA;  // پیش‌فرض فارسی

// جدول ترجمه: [StrId][Lang]  (ترتیب ستون‌ها: EN, TR, FA)
// نکته: ترکی بدون دیاکریتیک نوشته شده تا با فونت موجود حتماً رندر شود.
static const char *const I18N[STR_ID_COUNT][LANG_COUNT] = {
    /* STR_ADMIN_ENTER   */ { "Enter admin password",      "Yonetici sifresini girin",  "رمز ادمین را وارد کنید" },
    /* STR_SELECT_WIFI   */ { "Please select your Wi-Fi",  "Lutfen Wi-Fi agini secin",  "لطفا وای فای خود را انتخاب کنید" },
    /* STR_CONNECT       */ { "Connect",                   "Baglan",                    "اتصال" },
    /* STR_DEVICE_MODEL  */ { "Device model",              "Cihaz modeli",              "مدل دستگاه" },
    /* STR_SERIAL        */ { "Serial number",             "Seri numarasi",             "شماره سریال" },
    /* STR_WIRING_GUIDE  */ { "Wiring guide",              "Baglanti kilavuzu",         "راهنمای اتصالات" },
    /* STR_WIRE_I6       */ { "Wire 2 to i6",              "Kablo 2 -> i6",             "سیم ۲ به i6" },
    /* STR_WIRE_I7       */ { "Wire 1 to i7",              "Kablo 1 -> i7",             "سیم ۱ به i7" },
    /* STR_WIRE_Q1       */ { "Wire 5 to Q1",              "Kablo 5 -> Q1",             "سیم ۵ به Q1" },
    /* STR_WIRE_Q0       */ { "Wire 6 to Q0",              "Kablo 6 -> Q0",             "سیم ۶ به Q0" },
    /* STR_WIRE_Q2       */ { "Wire 7 to Q2",              "Kablo 7 -> Q2",             "سیم ۷ به Q2" },
    /* STR_WIRE_Q3       */ { "Wire 8 to Q3",              "Kablo 8 -> Q3",             "سیم ۸ به Q3" },
    /* STR_PROD_CHANEGIR */ { "Dough divider",             "Hamur bolucu",              "چانه گیر نان" },
    /* STR_INSTALL       */ { "Install",                   "Kur",                       "نصب" },
    /* STR_PROD_SINGLE   */ { "Single-head welder",        "Tek kafa kaynak",           "جوش تک سر" },
    /* STR_PROD_TEST     */ { "Device test",               "Cihaz testi",               "تست دستگاه" },
    /* STR_PROD_SW1      */ { "Software 1",                "Yazilim 1",                 "نرم افزار 1" },
    /* STR_SCANNING      */ { "Scanning...",               "Taraniyor...",              "در حال جستجو..." },
    /* STR_WIFI_FAIL     */ { "Wi-Fi connection failed",   "Wi-Fi baglantisi basarisiz","اتصال به وای فای ناموفق بود" },
    /* STR_WIFI_WRONGSAVED*/{ "Saved password is wrong, enter the correct one", "Kayitli sifre yanlis, dogru sifreyi girin", "رمز قبلی اشتباه بود، رمز درست را وارد کنید" },
    /* STR_ADMIN_WRONG   */ { "Wrong password",            "Yanlis sifre",              "رمز ورود اشتباه است" },
    /* STR_DL_PREFIX     */ { "Downloading ",              "Indiriliyor ",              "درحال دانلود " },
    /* STR_UP_TO_DATE    */ { "Already up to date",        "Zaten guncel",              "نرم‌افزار به‌روز است" },
    /* STR_NOT_ON_SERVER */ { "Not available on server",   "Sunucuda mevcut degil",     "این برنامه روی سرور نیست" },
    /* STR_NOT_AVAILABLE */ { "Not found",                 "Mevcut degil",              "موجود نیست" },
    /* STR_INCOMPATIBLE   */ { "Incompatible",              "Uyumsuz",                   "ناسازگار" },
    /* STR_PROD_CUTTER_SINGLE */ { "Single-head cutter",   "Tek kafa kesim",            "تک سر برش" },
    /* STR_PROD_CUTTER_DOUBLE */ { "Double-head cutter",   "Cift kafa kesim",           "دوسر برش" },
    /* STR_PROD_WELDER_DOUBLE */ { "Double-head welder",   "Cift kafa kaynak",          "جوش دوسر" },
    /* STR_PROD_GAS          */ { "Glass gas filling",     "Cam gaz dolumu",            "تزریق گاز شیشه" },
    /* STR_PROD_CORNER       */ { "Corner cleaner",        "Kose temizleyici",          "گوشه‌تمیزکن" },
    /* STR_PROD_COPY         */ { "Copy router",           "Kopya freze",               "کپی فرز" },
    /* STR_PROD_PUNCH        */ { "Aluminium punch",       "Aluminyum zimba",           "پانچ آلومینیوم" },
    /* STR_PROD_CONFIG       */ { "Config Launcher",       "Config programi",           "کانفیگ" },
    /* STR_CONFIRM_Q        */ { "Install this program?",  "Bu programi kur?",          "این برنامه نصب شود؟" },
    /* STR_YES              */ { "Yes, install",           "Evet, kur",                 "بله، نصب کن" },
    /* STR_NO               */ { "Cancel",                 "Iptal",                     "انصراف" },
    /* STR_BACK_TO_ADMIN    */ { "Back to admin login",    "Yonetici girisine don",     "برگشت به منو رمز ادمین" },
    /* STR_CANCEL_DL        */ { "Cancel Download",        "Iptal",                     "انصراف" },
    /* STR_DL_CANCELLED     */ { "Download cancelled",     "Indirme iptal edildi",      "دانلود لغو شد" },
    /* STR_DL_CANCELLING    */ { "Cancelling download...", "Iptal ediliyor...",         "در حال لغو دانلود..." },
    /* STR_DL_STALLED       */ { "Connection stalled",     "Baglanti kesildi",          "اتصال شبکه متوقف شد" },
    /* STR_DL_FINALIZING    */ { "Finalizing firmware...", "Tamamlaniyor...",           "نهایی‌سازی فریمور..." },
    /* STR_LINK_ENTRY       */ { "Direct URL Link",        "Link Girisi",               "ورود لینک" },
    /* STR_INFO             */ { "Info",                   "Bilgi",                     "مشخصات" },
    /* STR_BACK             */ { "Back",                   "Geri",                      "برگشت" },
    /* STR_INSTALL_THIS     */ { "Install this program",   "Bu programi kur",           "نصب این برنامه" },
    /* STR_INPUTS_PINS      */ { "Inputs - Sensors",       "Girisler - Sensorler",      "ورودی ها - سنسورها" },
    /* STR_OUTPUTS_PINS     */ { "Outputs - Relays",       "Cikislar - Roleler",        "خروجی ها - رله ها" },
    /* STR_FW_DETAILS       */ { "Firmware Details",       "Yazilim Ozellikleri",       "مشخصات و توضیحات فریمور" },
    /* STR_FEATURES_SPECS   */ { "Features & Specifications:", "Ozellikler ve Yetenekler:", "ویژگی ها و قابلیت های برنامه:" },
    /* STR_STATUS_READY     */ { "Repo Status: Available on Cloud Server", "Sunucu: Bulutta mevcut ve yuklemeye hazir", "وضعیت مخزن: موجود و آماده نصب روی سرور ابری" },
    /* STR_STATUS_OFFLINE   */ { "Repo Status: Not uploaded or offline", "Sunucu: Yuklenmemis veya cevrímidisi", "وضعیت مخزن: بارگذاری نشده یا عدم اتصال به سرور" },
    /* STR_HW_BASE          */ { "Hardware: ESP32-S3 CPU + Relays Q0..Q9", "Donanim: ESP32-S3 CPU + Roleler Q0..Q9", "سخت افزار: پردازنده ESP32-S3 + رله های صنعتی Q0..Q9" },
    /* STR_HW_REQ1          */ { "Hardware: Temp Controller + 1x MAX6675 (TC1)", "Donanim: Sicaklik Kontrolu + 1x MAX6675", "سخت افزار: کنترلر دما با ۱ عدد سنسور MAX6675 (TC1)" },
    /* STR_HW_REQ2          */ { "Hardware: Dual Temp (2x MAX6675) + MCP23017", "Donanim: Cift Sicaklik (2x MAX6675) + MCP23017", "سخت افزار: کنترلر دمای دوبل (۲x MAX6675) + افزاینده MCP23017" },
    /* STR_HW_REQ3          */ { "Hardware: I2C Output Expander MCP23017", "Donanim: I2C Cikis Genisletici MCP23017", "سخت افزار: ماژول توسعه خروجی های صنعتی I2C MCP23017" },
    /* STR_PROD_ICE_CREAM   */ { "Ice cream machine",      "Dondurma makinasi",         "دستگاه بستنی" },
    /* STR_PROD_BAKERY_CHANGIR*/{ "Bakery dough divider",   "Hamur kesme yuvarlama",     "نانوایی چونه‌گیر" },
    /* STR_PROD_BAKERY_MIXER*/ { "Bakery spiral mixer",    "Spiral hamur mikseri",      "نانوایی میکسر و خمیرگیر" },
    /* STR_SUITE_UPVC       */ { "UPVC & Aluminum Machinery", "UPVC & Aluminyum",       "ماشین‌سازی UPVC و آلومینیوم" },
    /* STR_SUITE_OTHER      */ { "Other Machinery",        "Diger Makineler",        "سایر شرکت‌ها" },
    /* STR_CONNECTING_TO_SERVER */ { "Connecting to server...", "Sunucuya baglaniliyor...", "در حال اتصال به سرور..." },
    /* STR_CHECKING             */ { "Checking...",             "Kontrol ediliyor...",      "در حال بررسی..." },
    /* STR_UNAVAILABLE_BTN      */ { "Unavailable",             "Erisilemez",               "عدم دسترسی" },
    /* STR_NEW_VERSION          */ { "New Version: ",           "Yeni Versiyon: ",          "نسخه جدید: " },
    /* STR_PROGRAM_UP_TO_DATE   */ { "Program up to date",      "Program guncel",           "برنامه بروز است" },
    /* STR_REINSTALL            */ { "Reinstall",               "Yeniden Yukle",            "بروزرسانی مجدد" },
    /* STR_NOT_DEFINED_ON_SERVER*/ { "Not defined on server",   "Sunucuda tanimli degil",   "تعریف نشده در سرور" },
    /* STR_NOT_RECEIVED_FROM_SERVER*/{ "Not received from server", "Sunucudan alinmadi",    "دریافت نشده از سرور" },
    /* STR_DETAILS_NOT_RECEIVED */ { "Specifications not received from server.", "Ozellikler sunucudan alinmadi.", "مشخصات و تغییرات از سرور دریافت نشده است." },
    /* STR_DEFAULT_LINK         */ { "Default Link",            "Varsayilan Link",          "لینک پیش‌فرض" },
    /* STR_DOWNLOAD_AND_INSTALL */ { "Download & Install",      "Indir ve Kur",             "دانلود و نصب" },
    /* STR_ENTER_LINK_OR_FILE   */ { "Please enter file name or link URL!", "Lutfen dosya adi veya link girin!", "لطفاً نام فایل یا آدرس لینک را وارد کنید!" },
    /* STR_LINK_CHECKING        */ { "Checking link on server...", "Sunucuda link kontrol ediliyor...", "در حال بررسی وضعیت لینک در سرور..." },
    /* STR_ENTER_FILENAME_ERR   */ { "Please enter file name at end of URL", "Lutfen adres sonuna dosya adini ekleyin", "لطفاً نام فایل را در انتهای آدرس وارد کنید" },
    /* STR_WIFI_DISCONNECTED_ERR*/ { "Error: WiFi not connected!", "Hata: WiFi baglantisi yok!", "خطا: ارتباط وای‌فای برقرار نیست!" },
    /* STR_LINK_VALID_BIN       */ { "Link is valid (Binary file available)\nPress green key to download and install", "Link gecerli (Iksili dosya mevcut)\nIndirmek ve kurmak icin yesil tusa basin", "لینک درست است (فایل باینری در سرور موجود است)\nجهت دانلود و نصب کلید سبز را بزنید" },
    /* STR_LINK_NOT_FOUND       */ { "File not found on server (Code 404)\nPlease check the URL", "Dosya sunucuda bulunamadi (Kod 404)\nLutfen link adresini kontrol edin", "فایل در سرور یافت نشد (کد ۴۰۴)\nلطفاً آدرس لینک را بررسی فرمایید" },
    /* STR_LINK_VALID_INFO      */ { "Link is valid (Press install to download)", "Link gecerli (Indirmek icin kura basin)", "لینک درست است (جهت دانلود کلید نصب را بزنید)" },
    /* STR_VERSION_INFO         */ { "Program info: Version", "Program bilgisi: Versiyon", "اطلاعات برنامه: نسخه" },
};

static const char *const I18N_DE[STR_ID_COUNT] = {
    /* STR_ADMIN_ENTER   */ "Admin-Passwort eingeben",
    /* STR_SELECT_WIFI   */ "Bitte WLAN auswahlen",
    /* STR_CONNECT       */ "Verbinden",
    /* STR_DEVICE_MODEL  */ "Geratemodell",
    /* STR_SERIAL        */ "Seriennummer",
    /* STR_WIRING_GUIDE  */ "Verdrahtungsplan",
    /* STR_WIRE_I6       */ "Draht 2 an i6",
    /* STR_WIRE_I7       */ "Draht 1 an i7",
    /* STR_WIRE_Q1       */ "Draht 5 an Q1",
    /* STR_WIRE_Q0       */ "Draht 6 an Q0",
    /* STR_WIRE_Q2       */ "Draht 7 an Q2",
    /* STR_WIRE_Q3       */ "Draht 8 an Q3",
    /* STR_PROD_CHANEGIR */ "Teigteiler",
    /* STR_INSTALL       */ "Installieren",
    /* STR_PROD_SINGLE   */ "Einkopf-Schweisser",
    /* STR_PROD_TEST     */ "Geratetest",
    /* STR_PROD_SW1      */ "Software 1",
    /* STR_SCANNING      */ "Suche...",
    /* STR_WIFI_FAIL     */ "WLAN-Verbindung fehlgeschlagen",
    /* STR_WIFI_WRONGSAVED*/"Gespeichertes Passwort ist falsch",
    /* STR_ADMIN_WRONG   */ "Falsches Passwort",
    /* STR_DL_PREFIX     */ "Lade ",
    /* STR_UP_TO_DATE    */ "Bereits aktuell",
    /* STR_NOT_ON_SERVER */ "Nicht auf Server verfugbar",
    /* STR_NOT_AVAILABLE */ "Nicht gefunden",
    /* STR_INCOMPATIBLE  */ "Nicht kompatibel",
    /* STR_PROD_CUTTER_SINGLE */ "Digitaler Schneider einzeln",
    /* STR_PROD_CUTTER_DOUBLE */ "Digitaler Schneider doppelt",
    /* STR_PROD_WELDER_DOUBLE */ "Doppelkopf-Schweisser",
    /* STR_PROD_GAS     */ "Glas-Gasfullung",
    /* STR_PROD_CORNER  */ "Eckenreiniger",
    /* STR_PROD_COPY    */ "Kopierfrase",
    /* STR_PROD_PUNCH   */ "Aluminium-Stanze",
    /* STR_PROD_CONFIG  */ "Config-Programm",
    /* STR_CONFIRM_Q    */ "Dieses Programm installieren?",
    /* STR_YES          */ "Ja, installieren",
    /* STR_NO           */ "Abbrechen",
    /* STR_BACK_TO_ADMIN*/ "Zuruck zum Admin-Login",
    /* STR_CANCEL_DL    */ "Abbrechen",
    /* STR_DL_CANCELLED */ "Download abgebrochen",
    /* STR_DL_CANCELLING*/ "Wird abgebrochen...",
    /* STR_DL_STALLED   */ "Verbindung unterbrochen",
    /* STR_DL_FINALIZING*/ "Wird finalisiert...",
    /* STR_LINK_ENTRY   */ "Direktlink",
    /* STR_INFO         */ "Details",
    /* STR_BACK         */ "Zuruck",
    /* STR_INSTALL_THIS */ "Programm installieren",
    /* STR_INPUTS_PINS  */ "Eingange - Sensoren",
    /* STR_OUTPUTS_PINS */ "Ausgange - Relais",
    /* STR_FW_DETAILS   */ "Firmware-Details",
    /* STR_FEATURES_SPECS*/ "Funktionen & Spezifikationen:",
    /* STR_STATUS_READY */ "Server-Status: Verfugbar und bereit",
    /* STR_STATUS_OFFLINE*/ "Server-Status: Nicht hochgeladen oder offline",
    /* STR_HW_BASE      */ "Hardware: ESP32-S3 CPU + Industrierelais Q0..Q9",
    /* STR_HW_REQ1      */ "Hardware: Temperaturregler + 1x MAX6675 (TC1)",
    /* STR_HW_REQ2      */ "Hardware: Doppel-Temperaturregler (2x MAX6675) + MCP23017",
    /* STR_HW_REQ3      */ "Hardware: I2C Industrie-Ausgangserweiterung MCP23017",
    /* STR_PROD_ICE_CREAM*/"Eismaschine",
    /* STR_PROD_BAKERY_CHANGIR*/"Teigteiler und Rundwirker",
    /* STR_PROD_BAKERY_MIXER*/"Spiralkneter Backerei",
    /* STR_SUITE_UPVC   */ "UPVC & Aluminium Maschinen",
    /* STR_SUITE_OTHER  */ "Programme anderer Hersteller",
    /* STR_CONNECTING_TO_SERVER */ "Verbinde mit Server...",
    /* STR_CHECKING             */ "Prufen...",
    /* STR_UNAVAILABLE_BTN      */ "Nicht verfugbar",
    /* STR_NEW_VERSION          */ "Neue Version: ",
    /* STR_PROGRAM_UP_TO_DATE   */ "Program aktuell",
    /* STR_REINSTALL            */ "Neu installieren",
    /* STR_NOT_DEFINED_ON_SERVER*/ "Nicht auf Server definiert",
    /* STR_NOT_RECEIVED_FROM_SERVER*/"Nicht vom Server empfangen",
    /* STR_DETAILS_NOT_RECEIVED */ "Spezifikationen nicht vom Server empfangen.",
    /* STR_DEFAULT_LINK         */ "Standard-Link",
    /* STR_DOWNLOAD_AND_INSTALL */ "Herunterladen & Installieren",
    /* STR_ENTER_LINK_OR_FILE   */ "Bitte Dateinamen oder Link eingeben!",
    /* STR_LINK_CHECKING        */ "Link auf Server wird gepruft...",
    /* STR_ENTER_FILENAME_ERR   */ "Bitte Dateinamen am Ende eingeben",
    /* STR_WIFI_DISCONNECTED_ERR*/ "Fehler: Keine WLAN-Verbindung!",
    /* STR_LINK_VALID_BIN       */ "Link gultig (Binar-Datei verfugbar)\nGrun drucken zum Herunterladen/Installieren",
    /* STR_LINK_NOT_FOUND       */ "Datei nicht gefunden (Code 404)\nBitte Link-Adresse prufen",
    /* STR_LINK_VALID_INFO      */ "Link gultig (Installieren drucken)",
    /* STR_VERSION_INFO         */ "Programminfo: Version",
};

static const char *const I18N_RU[STR_ID_COUNT] = {
    /* STR_ADMIN_ENTER   */ "Vvedite parol admina",
    /* STR_SELECT_WIFI   */ "Vyberite Wi-Fi",
    /* STR_CONNECT       */ "Podklyuchit",
    /* STR_DEVICE_MODEL  */ "Model ustroystva",
    /* STR_SERIAL        */ "Seriyniy nomer",
    /* STR_WIRING_GUIDE  */ "Shema podklyucheniya",
    /* STR_WIRE_I6       */ "Provod 2 k i6",
    /* STR_WIRE_I7       */ "Provod 1 k i7",
    /* STR_WIRE_Q1       */ "Provod 5 k Q1",
    /* STR_WIRE_Q0       */ "Provod 6 k Q0",
    /* STR_WIRE_Q2       */ "Provod 7 k Q2",
    /* STR_WIRE_Q3       */ "Provod 8 k Q3",
    /* STR_PROD_CHANEGIR */ "Delitel testa",
    /* STR_INSTALL       */ "Ustanovit",
    /* STR_PROD_SINGLE   */ "Svarshchik 1 golova",
    /* STR_PROD_TEST     */ "Test ustroystva",
    /* STR_PROD_SW1      */ "Programma 1",
    /* STR_SCANNING      */ "Poisk...",
    /* STR_WIFI_FAIL     */ "Oshibka Wi-Fi",
    /* STR_WIFI_WRONGSAVED*/"Sohranenny parol neveren",
    /* STR_ADMIN_WRONG   */ "Neverniy parol",
    /* STR_DL_PREFIX     */ "Zagruzka ",
    /* STR_UP_TO_DATE    */ "Uzhe obnovleno",
    /* STR_NOT_ON_SERVER */ "Net na servere",
    /* STR_NOT_AVAILABLE */ "Ne naydeno",
    /* STR_INCOMPATIBLE  */ "Nesovmestimo",
    /* STR_PROD_CUTTER_SINGLE */ "Tsifrovoy rezak odin",
    /* STR_PROD_CUTTER_DOUBLE */ "Tsifrovoy rezak dva",
    /* STR_PROD_WELDER_DOUBLE */ "Svarshchik 2 golovy",
    /* STR_PROD_GAS     */ "Zapravka stekla gazom",
    /* STR_PROD_CORNER  */ "Ochistka ugla",
    /* STR_PROD_COPY    */ "Kopir frezer",
    /* STR_PROD_PUNCH   */ "Alyuminieviy punch",
    /* STR_PROD_CONFIG  */ "Config programma",
    /* STR_CONFIRM_Q    */ "Ustanovit programmu?",
    /* STR_YES          */ "Da, ustanovit",
    /* STR_NO           */ "Otmena",
    /* STR_BACK_TO_ADMIN*/ "Nazad k adminu",
    /* STR_CANCEL_DL    */ "Otmena",
    /* STR_DL_CANCELLED */ "Zagruzka otmenena",
    /* STR_DL_CANCELLING*/ "Otmena...",
    /* STR_DL_STALLED   */ "Soedinenie prervano",
    /* STR_DL_FINALIZING*/ "Zavershenie...",
    /* STR_LINK_ENTRY   */ "Vvod ssylki",
    /* STR_INFO         */ "Info",
    /* STR_BACK         */ "Nazad",
    /* STR_INSTALL_THIS */ "Ustanovit etu programmu",
    /* STR_INPUTS_PINS  */ "Vhody - Datchiki",
    /* STR_OUTPUTS_PINS */ "Vyhody - Rele",
    /* STR_FW_DETAILS   */ "Opisanie proshivki",
    /* STR_FEATURES_SPECS*/ "Funktsii i vozmozhnosti:",
    /* STR_STATUS_READY */ "Server: Dostupno na oblachnom servere",
    /* STR_STATUS_OFFLINE*/ "Server: Ne zagruzheno ili net svyazi",
    /* STR_HW_BASE      */ "Apparat: ESP32-S3 CPU + Rele Q0..Q9",
    /* STR_HW_REQ1      */ "Apparat: Termokontroller + 1x MAX6675 (TC1)",
    /* STR_HW_REQ2      */ "Apparat: Dvoynoy termokontroller + MCP23017",
    /* STR_HW_REQ3      */ "Apparat: Modul rasshireniya I2C MCP23017",
    /* STR_PROD_ICE_CREAM*/"Apparat dlya morozhenogo",
    /* STR_PROD_BAKERY_CHANGIR*/"Testodelitel okruglitel",
    /* STR_PROD_BAKERY_MIXER*/"Testomesilnaya mashina",
    /* STR_SUITE_UPVC   */ "UPVC i alyuminievye stanki",
    /* STR_SUITE_OTHER  */ "Programmy drugikh kompaniy",
    /* STR_CONNECTING_TO_SERVER */ "Podklyuchenie k serveru...",
    /* STR_CHECKING             */ "Proverka...",
    /* STR_UNAVAILABLE_BTN      */ "Nedostupno",
    /* STR_NEW_VERSION          */ "Novaya versiya: ",
    /* STR_PROGRAM_UP_TO_DATE   */ "Programma obnovlena",
    /* STR_REINSTALL            */ "Pereustanovit",
    /* STR_NOT_DEFINED_ON_SERVER*/ "Ne opredeleno na servere",
    /* STR_NOT_RECEIVED_FROM_SERVER*/"Ne polucheno s servera",
    /* STR_DETAILS_NOT_RECEIVED */ "Spetsifikatsii ne polucheny s servera.",
    /* STR_DEFAULT_LINK         */ "Ssylka po umolchaniyu",
    /* STR_DOWNLOAD_AND_INSTALL */ "Skachat i ustanovit",
    /* STR_ENTER_LINK_OR_FILE   */ "Vvedite imya fayla ili ssylku!",
    /* STR_LINK_CHECKING        */ "Proverka ssylki na servere...",
    /* STR_ENTER_FILENAME_ERR   */ "Vvedite imya fayla v kontse adresa",
    /* STR_WIFI_DISCONNECTED_ERR*/ "Oshibka: Net podklyucheniya k WiFi!",
    /* STR_LINK_VALID_BIN       */ "Ssylka verna (Fayl dostupen)\nNazhmite zelenuyu knopku dlya ustanovki",
    /* STR_LINK_NOT_FOUND       */ "Fayl ne nayden na servere (Kod 404)\nProverte adres ssylki",
    /* STR_LINK_VALID_INFO      */ "Ssylka verna (Nazhmite dlya ustanovki)",
    /* STR_VERSION_INFO         */ "Info o programme: Versiya",
};

static const char *const NATIVE_NAMES[LANG_COUNT] = {
    "English",   // EN
    "Turkce",    // TR (بدون دیاکریتیک)
    "فارسی",     // FA
    "Deutsch",   // DE
    "Russkiy",   // RU
};

const char *lang_native_name(Lang l) {
    if (l < 0 || l >= LANG_COUNT) return "";
    return NATIVE_NAMES[l];
}

const char *tr(StrId id) {
    if (id < 0 || id >= STR_ID_COUNT) return "";
    Lang l = g_lang;
    if (l < 0 || l >= LANG_COUNT) l = LANG_FA;
    if (l == LANG_DE) return I18N_DE[id];
    if (l == LANG_RU) return I18N_RU[id];
    return I18N[id][l];
}

// ---------------- رجیستری لیبل‌های صفحه‌ی فعلی ----------------
#define I18N_MAX 80
static lv_obj_t *reg_obj[I18N_MAX];
static StrId reg_id[I18N_MAX];
static int reg_count = 0;

void i18n_reset(void) {
    reg_count = 0;
}

void i18n_register(lv_obj_t *label, StrId id) {
    if (!label || reg_count >= I18N_MAX) return;
    reg_obj[reg_count] = label;
    reg_id[reg_count] = id;
    reg_count++;
}

void i18n_apply(void) {
    for (int i = 0; i < reg_count; i++) {
        if (reg_obj[i] && lv_obj_is_valid(reg_obj[i])) {
            lv_label_set_text(reg_obj[i], tr(reg_id[i]));
        }
    }
}
