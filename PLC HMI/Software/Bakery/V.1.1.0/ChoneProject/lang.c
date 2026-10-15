#include "lang.h"
#include <string.h>

Lang g_lang = LANG_FA;

static const char *const I18N[STR_ID_COUNT][LANG_COUNT] = {
    /* STR_APP_TITLE        */ { "چانه گیر نان",            "Dough Divider",        "Hamur Bolucu",             "Delitel Testa" },
    /* STR_AUTO_MODE        */ { "اتوماتیک",                "Automatic",            "Otomatik",                 "Avtomaticheskiy" },
    /* STR_MANUAL_MODE      */ { "دستی",                    "Manual",               "Manuel",                   "Ruchnoy" },
    /* STR_SETTINGS         */ { "تنظیمات",                 "Settings",             "Ayarlar",                  "Nastroyki" },
    /* STR_WEIGHT           */ { "وزن چانه",                "Portion Weight",       "Porsiyon Agirligi",        "Ves Porcii" },
    /* STR_SPEED            */ { "سرعت برش",                "Cutting Speed",        "Kesim Hizi",               "Skorost Rezki" },
    /* STR_COUNT            */ { "تعداد",                   "Quantity",             "Adet",                     "Kolichestvo" },
    /* STR_RESET_COUNT      */ { "ریست تعداد",              "Reset Count",          "Sayaci Sifirla",           "Sbros Schetchika" },
    /* STR_START            */ { "شروع",                    "Start",                "Baslat",                   "Pusk" },
    /* STR_STOP             */ { "توقف",                    "Stop",                 "Durdur",                   "Stop" },
    /* STR_BACK             */ { "بازگشت",                  "Back",                 "Geri",                     "Nazad" },
    /* STR_SAVE             */ { "ذخیره",                   "Save",                 "Kaydet",                   "Sohranit" },
    /* STR_MAX_PISTON       */ { "حد پیستون (ms)",          "Max Piston (ms)",      "Maks Piston (ms)",         "Maks Porshen (ms)" },
    /* STR_MAX_FUNNEL       */ { "حد قیف (ms)",             "Max Funnel (ms)",      "Maks Huni (ms)",           "Maks Voronka (ms)" },
    /* STR_PISTON_UP        */ { "پیستون بالا",             "Piston Up",            "Piston Yukari",            "Porshen Vverkh" },
    /* STR_PISTON_DOWN      */ { "پیستون پایین",            "Piston Down",          "Piston Asagi",             "Porshen Vniz" },
    /* STR_FUNNEL_FWD       */ { "قیف جلو",                 "Funnel Front",         "Huni Ileri",               "Voronka Vpered" },
    /* STR_FUNNEL_REV       */ { "قیف عقب",                 "Funnel Back",          "Huni Geri",                "Voronka Nazad" },
    /* STR_DIAGNOSTICS      */ { "تست قطعات",               "Diagnostics",          "Donanim Testi",            "Diagnostika" },
    /* STR_SYS_READY        */ { "آماده به کار",            "Ready",                "Hazir",                    "Gotov" },
    /* STR_RUNNING          */ { "در حال کار",              "Running",              "Calisiyor",                "Rabota" },
    /* STR_SAVED_OK         */ { "تنظیمات ذخیره شد",        "Settings Saved",       "Ayarlar Kaydedildi",       "Nastroyki Sohraneny" },
    /* STR_PIN_ERROR        */ { "رمز اشتباه است",          "Invalid PIN",          "Yanlis Sifre",             "Neverniy PIN" },
    /* STR_WARNING          */ { "هشدار ایمنی",             "Safety Warning",       "Guvenlik Uyarisi",         "Preduprezhdenie" },
    /* STR_ERROR            */ { "خطا",                     "Error",                "Hata",                     "Oshibka" },
    /* STR_CONFIRM_CONTINUE */ { "تایید و ادامه",           "Confirm & Continue",   "Onayla ve Devam Et",       "Podtverdit" },
    /* STR_WARN_RULE_1      */ { "[1] قبل از برق، حتما سیم ارت وصل شود.", "[1] Connect ground wire before power.", "[1] Once topraklama hattini baglayin.", "[1] Podklyuchit zazemlenie do seti." },
    /* STR_WARN_RULE_2      */ { "[2] در خطر فوری از شاسی STOP استفاده کنید.", "[2] In emergency, press red E-Stop.", "[2] Acil durumda STOP butonuna basin.", "[2] Pri opasnosti nazhat knopku STOP." },
    /* STR_WARN_RULE_3      */ { "[3] هر ماه واسکازین گیربکس ها بررسی شود.", "[3] Check gearbox oil level monthly.", "[3] Reduktor yagini ayda bir kontrol edin.", "[3] Proveryat maslo reduktora raz v mesyac." },
    /* STR_WARN_RULE_4      */ { "[4] قبل از نظافت برق را قطع کنید. دست نزنید.", "[4] Cut power before cleaning. Keep hands off.", "[4] Temizlikten once elektrigi kesin.", "[4] Otklyuchit pitanie pered chistkoy." },
    /* STR_WIFI_TITLE       */ { "ارتباط وای فای",          "WiFi & WebApp",        "Kablosuz Ag",              "Besprovodnoy WiFi" },
    /* STR_WIFI_ON          */ { "روشن: وب اپ فعال",        "ON: WebApp Active",    "ACIK: WebApp Aktif",       "VKLYUCHENO: WebApp" },
    /* STR_WIFI_OFF         */ { "خاموش",                   "OFF",                  "KAPALI",                   "VYKLYUCHENO" },
    /* STR_LANG_SELECT      */ { "انتخاب زبان",             "Select Language",      "Dil Secimi",               "Vybor Yazyka" },
    /* STR_IO_TEST          */ { "ورود به تست I/O",         "Hardware I/O Test",    "Donanim Testi",            "Test Zheleza I/O" },
    /* STR_SET_VALUE        */ { "تنظیم مقدار",             "Set Value",            "Deger Ayarla",             "Ustanovka Znacheniya" },
    /* STR_TIME_MS          */ { "زمان (میلی ثانیه)",       "Time (ms)",            "Zaman (ms)",               "Vremya (ms)" },
    /* STR_CLOSE            */ { "بستن",                    "Close",                "Kapat",                    "Zakryt" },
    /* STR_LANG_CHANGED     */ { "زبان تغییر یافت",         "Language Changed",     "Dil Degistirildi",         "Yazyk Izmenen" },
    /* STR_PROJECT_INFO     */ { "بروزرسانی و مشخصات",      "Update & System Info", "Guncelleme & Bilgi",       "Obnovlenie & Info" },
    /* STR_OTA_UPDATE       */ { "بروزرسانی نرم افزار",     "Software Update",      "Yazilim Guncelleme",       "Obnovlenie PO" },
    /* STR_WIFI_NETWORKS    */ { "شبکه های وای فای",        "WiFi Networks",        "WiFi Aglari",              "Seti WiFi" },
    /* STR_WIFI_SCAN        */ { "جستجو",                   "Scan",                 "Ara",                      "Poisk" },
    /* STR_WIFI_SCANNING    */ { "در حال جستجو...",         "Scanning...",          "Araniyor...",              "Poisk..." },
    /* STR_WIFI_CONNECT     */ { "اتصال",                   "Connect",              "Baglan",                   "Podklyuchit" },
    /* STR_WIFI_CONNECTING  */ { "در حال اتصال...",         "Connecting...",        "Baglaniyor...",            "Podklyuchenie..." },
    /* STR_WIFI_CONNECTED   */ { "متصل شد",                 "Connected",            "Baglandi",                 "Podklyucheno" },
    /* STR_WIFI_DISCONNECTED*/ { "قطع ارتباط",              "Disconnected",         "Baglanti Kesildi",         "Otklucheno" },
    /* STR_WIFI_WRONG_PASS  */ { "رمز اشتباه است، دوباره امتحان کنید", "Wrong password, please retry", "Yanlis sifre, tekrar deneyin", "Nevernyy parol, poprobuyte snova" },
    /* STR_WIFI_PASS_PROMPT */ { "رمز عبور وای فای",        "WiFi Password",        "WiFi Sifresi",             "Parol WiFi" },
    /* STR_FETCH_FROM_SERVER*/ { "دریافت از سرور",          "Fetch from Server",    "Sunucudan Al",             "Poluchit s servera" },
    /* STR_FETCHING_SERVER  */ { "در حال استعلام از سرور...", "Fetching from server...", "Sunucudan aliniyor...", "Zaprashivaetsya s servera..." },
    /* STR_SERVER_UNAVAILABLE*/{ "ارتباط با سرور برقرار نشد", "Server unavailable",  "Sunucuya baglanilamadi",   "Server nedostupen" },
    /* STR_WIFI_NOT_CONNECTED_WARN*/ { "ابتدا به اینترنت متصل شوید", "Connect to internet first", "Once internete baglanin", "Snachala podklyuchites k internetu" },
    /* STR_INSTALL_UPDATE   */ { "نصب نسخه جدید",           "Install Update",       "Guncellemeyi Yukle",       "Ustanovit obnovlenie" },
    /* STR_INSTALL_CONFIRM  */ { "آیا از نصب نسخه جدید اطمینان دارید؟", "Are you sure you want to install?", "Guncellemeyi yuklemek istiyor musunuz?", "Ustanovit novuyu versiyu?" },
    /* STR_INSTALLING       */ { "در حال دانلود و نصب...",  "Downloading & Installing...", "Indiriliyor ve kuruluyor...", "Zagruzka i ustanovka..." },
    /* STR_PINMAP_TITLE     */ { "نقشه اتصالات سخت افزاری", "Hardware Pinout Map",  "Donanim Pin Haritasi",     "Karta pinov" },
    /* STR_INPUTS_TITLE     */ { "ورودی ها",                "Inputs",               "Girisler",                 "Vhody" },
    /* STR_OUTPUTS_TITLE    */ { "خروجی ها",                "Outputs",              "Cikislar",                 "Vyhody" },
    /* STR_CHANGELOG_TITLE  */ { "تغییرات و قابلیت های نسخه", "Release Changelog",   "Surum Degisiklikleri",     "Izmeneniya versii" },
    /* STR_UP_TO_DATE       */ { "نسخه شما به روز است",     "System is up to date", "Sisteminiz guncel",        "Sistema obnovlena" },
    /* STR_HELP_MANUAL      */ { "راهنمای کاربری",           "User Manual",          "Kullanim Kilavuzu",        "Rukovodstvo" },
    /* STR_LINK_ENTRY       */ { "ورود لینک",               "Direct URL Link",      "Link Girisi",             "Vvod ssylki" },
    /* STR_DEFAULT_LINK     */ { "لینک پیش‌فرض",           "Default Link",         "Varsayilan Link",         "Standartnaya ssylka" },
    /* STR_DOWNLOAD_AND_INSTALL */ { "دانلود و نصب",        "Download and Install", "Indir ve Kur",            "Skachat i ustanovit" },
    /* STR_SPECS_TITLE      */ { "مشخصات دستگاه",          "Device Specs",         "Cihaz Ozellikleri",       "Kharakteristiki" },
    /* STR_CONFIG_TEST_TITLE*/ { "کانفیگ و تست سخت‌افزار",  "Hardware Config Test", "Donanim Testi",          "Test oborudovaniya" },
    /* STR_CHONE_GIR_TITLE  */ { "نان چانه‌گیر و گردکن",   "Dough Divider Rounder","Hamur Kesme",           "Testodelitel" },
};

static const char *const NATIVE_NAMES[LANG_COUNT] = {
    "فارسی",     // FA
    "English",   // EN
    "Turkce",    // TR
    "Russkiy"    // RU
};

const char *lang_native_name(Lang l) {
    if (l < 0 || l >= LANG_COUNT) return "";
    return NATIVE_NAMES[l];
}

const char *tr(StrId id) {
    if (id < 0 || id >= STR_ID_COUNT) return "";
    Lang l = g_lang;
    if (l < 0 || l >= LANG_COUNT) l = LANG_FA;
    return I18N[id][l];
}

void lang_cycle(void) {
    g_lang = (Lang)(((int)g_lang + 1) % LANG_COUNT);
    i18n_apply();
}

void lang_set(Lang l) {
    if (l >= 0 && l < LANG_COUNT) {
        g_lang = l;
        i18n_apply();
    }
}

// ---------------- Registry of static labels on active screens ----------------
#define I18N_MAX_LABELS 128
static lv_obj_t *reg_obj[I18N_MAX_LABELS];
static StrId reg_id[I18N_MAX_LABELS];
static int reg_count = 0;

void i18n_reset(void) {
    reg_count = 0;
}

void i18n_register(lv_obj_t *label, StrId id) {
    if (!label) return;
    // Prevent duplicate registrations
    for (int i = 0; i < reg_count; i++) {
        if (reg_obj[i] == label) {
            reg_id[i] = id;
            if (lv_obj_is_valid(label) && lv_obj_check_type(label, &lv_label_class)) {
                lv_label_set_text(label, tr(id));
            }
            return;
        }
    }
    if (reg_count < I18N_MAX_LABELS) {
        reg_obj[reg_count] = label;
        reg_id[reg_count] = id;
        reg_count++;
        if (lv_obj_is_valid(label) && lv_obj_check_type(label, &lv_label_class)) {
            lv_label_set_text(label, tr(id));
        }
    }
}

void i18n_apply(void) {
    for (int i = 0; i < reg_count; i++) {
        if (reg_obj[i] && lv_obj_is_valid(reg_obj[i]) && lv_obj_check_type(reg_obj[i], &lv_label_class)) {
            lv_label_set_text(reg_obj[i], tr(reg_id[i]));
        }
    }
}
