#include "ui_HelpScreen.h"
#include "ui.h"
#include "lang.h"
#include "sound.h"

lv_obj_t *ui_HelpScreen = NULL;

static lv_obj_t *s_help_header = NULL;
static lv_obj_t *s_help_title_lbl = NULL;
static lv_obj_t *s_help_back_btn = NULL;
static lv_obj_t *s_help_scroll_container = NULL;

// Content cards labels
static lv_obj_t *s_lbl_auto_title = NULL;
static lv_obj_t *s_lbl_auto_body = NULL;
static lv_obj_t *s_lbl_manual_title = NULL;
static lv_obj_t *s_lbl_manual_body = NULL;
static lv_obj_t *s_lbl_set_title = NULL;
static lv_obj_t *s_lbl_set_body = NULL;
static lv_obj_t *s_lbl_wifi_title = NULL;
static lv_obj_t *s_lbl_wifi_body = NULL;
static lv_obj_t *s_lbl_lock_title = NULL;
static lv_obj_t *s_lbl_lock_body = NULL;
static lv_obj_t *s_lbl_contact_title = NULL;
static lv_obj_t *s_lbl_contact_body = NULL;

static void help_back_clicked_cb(lv_event_t *e) {
    sound_play_c(SOUND_CLICK);
    if (ui_SettingPage && lv_obj_is_valid(ui_SettingPage)) {
        lv_scr_load(ui_SettingPage);
    } else if (ui_Menu && lv_obj_is_valid(ui_Menu)) {
        lv_scr_load(ui_Menu);
    }
}

static void update_help_content(void) {
    int l = (int)g_lang;
    lv_base_dir_t bdir = (l == LANG_FA) ? LV_BASE_DIR_RTL : LV_BASE_DIR_LTR;
    lv_text_align_t talign = (l == LANG_FA) ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT;

    // Header Title
    const char *header_titles[LANG_COUNT] = {
        "راهنمای جامع کاربری دستگاه",
        "Operator User Manual",
        "Kullanim Kilavuzu",
        "Rukovodstvo polzovatelya"
    };
    if (s_help_title_lbl) {
        lv_label_set_text(s_help_title_lbl, header_titles[l]);
    }

    // Card 1: Auto Mode
    const char *auto_titles[LANG_COUNT] = {
        "حالت اتوماتیک (تولید پیوسته)",
        "Automatic Mode (Continuous Production)",
        "Otomatik Mod (Surekli Uretim)",
        "Avtomaticheskiy rezhim"
    };
    const char *auto_bodies[LANG_COUNT] = {
        "• برای شروع فرآیند برش، کلید استارت اتوماتیک را لمس کنید.\n"
        "• در این حالت، پیستون خمیر را مکش کرده و پس از اتمام تایم، تیغه قیف آن را برش می‌دهد.\n"
        "• با لمس آیکون بلندگو در هدر می‌توانید صدای بوق شمارش چانه‌ها را بی‌صدا (سایلنت) کنید.\n"
        "• با زدن کلید ریست تعداد، شمارنده چانه‌های تولیدی صفر می‌گردد.",

        "• Touch the Start button to begin continuous automatic dough portioning.\n"
        "• Piston draws the dough and the funnel blade cuts it when the stroke completes.\n"
        "• Touch the speaker icon in the header to mute the portion count beep.\n"
        "• Use the Reset Counter button to reset the portion count back to zero.",

        "• Surekli otomatik hamur porsiyonlamayi baslatmak icin Baslat tusuna basin.\n"
        "• Piston hamuru ceker ve strok tamamlandiginda huni bicagi keser.\n"
        "• Sayac bip sesini kapatmak icin basliktaki hoparlor simgesine dokunun.\n"
        "• Uretilen porsiyon sayacini sifirlamak icin Sifirla tusunu kullanin.",

        "• Nazhmite Pusk dlya zapuska nepreryvnoy narezki testa.\n"
        "• Porshen zatyagivaet testo, a lezvie voronki otrezaet portsiyu.\n"
        "• Nazhmite na dinamik dlya otklyucheniya zvuka schetchika.\n"
        "• Dlya sbrosa schetchika portsiy nazhmite knopku Sbros."
    };
    if (s_lbl_auto_title && s_lbl_auto_body) {
        lv_label_set_text(s_lbl_auto_title, auto_titles[l]);
        lv_label_set_text(s_lbl_auto_body, auto_bodies[l]);
        lv_obj_set_style_base_dir(s_lbl_auto_body, bdir, 0);
        lv_obj_set_style_text_align(s_lbl_auto_body, talign, 0);
    }

    // Card 2: Manual Mode
    const char *manual_titles[LANG_COUNT] = {
        "حالت دستی (تست انفرادی جک‌ها و موتورها)",
        "Manual Mode (Actuator & Jog Testing)",
        "Manuel Mod (Aktuator ve Motor Testi)",
        "Ruchnoy rezhim"
    };
    const char *manual_bodies[LANG_COUNT] = {
        "• جهت تست مستقل یا تنظیم مکانیکی قطعات از حالت دستی استفاده کنید.\n"
        "• کلیدهای رفت و برگشت پیستون امکان جابجایی موتور محرک پیستون را می‌دهند.\n"
        "• کلیدهای جلو و عقب قیف حرکت قالب خمیرگیر را کنترل می‌کنند.\n"
        "• چراغ‌های نشانگر وضعیت سنسورهای هوم پیستون و قیف را لحظه‌ای نمایش می‌دهند.",

        "• Use Manual Mode for independent diagnostic testing and mechanical adjustments.\n"
        "• Piston Up/Down controls allow inching the piston motor drive.\n"
        "• Funnel Forward/Reverse controls jog the funnel divider mechanism.\n"
        "• Status indicator lights reflect the inductive home sensor signals in real time.",

        "• Bagimsiz test ve mekanik ayarlar icin Manuel Modu kullanin.\n"
        "• Piston Ileri/Geri tuslari piston surucusunu adim adim hareket ettirir.\n"
        "• Huni Ileri/Geri tuslari kesici mekanizmayi kontrol eder.\n"
        "• Durum gosterge isiklari enduktif home sensorlerini anlik gosterir.",

        "• Ispolzuyte ruchnoy rezhim dlya diagnostiki i nastroiki mehanizmov.\n"
        "• Knopki porshnya pozvolyayut poshagovo peremeschat privod porshnya.\n"
        "• Knopki voronki upravlyayut lezviem delitelya testa.\n"
        "• Indikatory pokazyvayut sostoyanie datchikov polozheniya v realnom vremeni."
    };
    if (s_lbl_manual_title && s_lbl_manual_body) {
        lv_label_set_text(s_lbl_manual_title, manual_titles[l]);
        lv_label_set_text(s_lbl_manual_body, manual_bodies[l]);
        lv_obj_set_style_base_dir(s_lbl_manual_body, bdir, 0);
        lv_obj_set_style_text_align(s_lbl_manual_body, talign, 0);
    }

    // Card 3: Settings & Calibration
    const char *set_titles[LANG_COUNT] = {
        "تنظیمات وزن و سرعت چانه‌ها",
        "Portion Weight & Speed Calibration",
        "Gramaj ve Hiz Kalibrasyonu",
        "Kalibrovka vesa i skorosti"
    };
    const char *set_bodies[LANG_COUNT] = {
        "• حد پیستون (وزن چانه): کورس مکش خمیر را تنظیم می‌کند؛ عدد بزرگتر وزن چانه را افزایش می‌دهد.\n"
        "• حد قیف (فاصله زمانی): فاصله زمانی برش تیغه را کنترل می‌نماید.\n"
        "• سرعت اینورتر: فرکانس چرخش موتور اصلی و شتاب حرکت را مشخص می‌کند.\n"
        "• برای ویرایش سریع، روی کادر عدد لمس کنید تا کیبورد عددی باز شود.",

        "• Piston Limit (Weight): Sets the suction stroke length; higher values yield heavier dough balls.\n"
        "• Funnel Limit (Timing): Adjusts the delay interval between successive cuts.\n"
        "• Inverter Speed: Configures motor drive frequency and operational tempo.\n"
        "• For direct numeric input, tap the value box to open the on-screen keypad.",

        "• Piston Limiti (Gramaj): Eme strokunu belirler; yuksek deger daha agir hamur verir.\n"
        "• Huni Limiti (Sure): Bicağın ardışık kesimleri arasindaki bekleme suresini ayarlar.\n"
        "• Invertor Hizi: Motor surucu frekansini ve uretim hizini belirler.\n"
        "• Hizli sayisal giris icin kutucuga dokunup ekran klavyesini acin.",

        "• Hod porshnya (Ves): Reguliruet ves portsii; bolshee chislo uvelichivaet ves.\n"
        "• Zaderzhka voronki: Nastraivaet interval mezhdu narezkami.\n"
        "• Chastota invertora: Opredelyaet skorost raboty motora.\n"
        "• Dlya pryamogo vvoda nazhmite na pole chisla dlya otkrytiya klaviatury."
    };
    if (s_lbl_set_title && s_lbl_set_body) {
        lv_label_set_text(s_lbl_set_title, set_titles[l]);
        lv_label_set_text(s_lbl_set_body, set_bodies[l]);
        lv_obj_set_style_base_dir(s_lbl_set_body, bdir, 0);
        lv_obj_set_style_text_align(s_lbl_set_body, talign, 0);
    }

    // Card 4: Dual WiFi Methods
    const char *wifi_titles[LANG_COUNT] = {
        "ارتباط وای‌فای و دو روش اتصال",
        "Dual WiFi Connectivity Methods",
        "Cift WiFi Baglanti Modu",
        "Dva rezhima WiFi svyazi"
    };
    const char *wifi_bodies[LANG_COUNT] = {
        "۱. هات‌اسپات محلی (SoftAP - 192.168.4.1):\n"
        "دستگاه خود وای‌فای مستقلی تولید می‌کند. در نانوایی‌های بدون مودم یا اینترنت، با تلفن همراه به نام دستگاه وصل شوید و مرورگر را باز کنید تا پنل وب با تمام امکانات باز شود.\n\n"
        "۲. اتصال به مودم کارگاه (Station):\n"
        "از منوی وای‌فای، به وای‌فای کارگاه متصل شوید. به محض اتصال:\n"
        "• ساعت دستگاه به طور خودکار از اینترنت (NTP تهران) دقیق و تنظیم می‌گردد.\n"
        "• آپدیت‌های نرم‌افزاری جدید مستقیماً از سرور مرکزی هافلر دریافت و نصب می‌شوند.",

        "1. Local Hotspot (SoftAP - 192.168.4.1):\n"
        "The machine broadcasts its own WiFi network. In workshops without a router, connect your phone directly and open 192.168.4.1 to access the full web portal.\n\n"
        "2. Workshop Network (Station Client):\n"
        "Connect the machine to the bakery's WiFi router. Once connected:\n"
        "• Internet time (Tehran NTP +03:30) synchronizes automatically to the clock.\n"
        "• Cloud firmware updates can be downloaded and installed directly from Haffler servers.",

        "1. Yerel Hotspot (SoftAP - 192.168.4.1):\n"
        "Cihaz kendi WiFi agini yayar. Modemsiz atolyelerde telefonunuzu baglayip tarayicidan 192.168.4.1 adresine girerek tam web paneline ulasabilirsiniz.\n\n"
        "2. Atolye Agi (Station Modu):\n"
        "Cihazi firinin yerel WiFi agina baglayin. Baglanti saglandiginda:\n"
        "• İnternet saat sunucusu (NTP) uzerinden cihaz saati aninda senkronize olur.\n"
        "• Haffler bulut sunucularindan en son yazilim surumleri dogrudan yuklenebilir.",

        "1. Tochka dostupa (SoftAP - 192.168.4.1):\n"
        "Mashina sozdaet sobstvennuyu set WiFi. Podklyuchites s telefona i otkroyte 192.168.4.1 dlya polnogo upravleniya cherez veb-portal.\n\n"
        "2. Podklyuchenie k routeru (Station):\n"
        "Podklyuchite k tsehovomu WiFi. Pri podklyuchenii:\n"
        "• Avtomaticheski tochno sinhroniziruetsya vremya po internetu (NTP).\n"
        "• Poslednie obnovleniya PO skachivayutsya pryamo s servera Haffler."
    };
    if (s_lbl_wifi_title && s_lbl_wifi_body) {
        lv_label_set_text(s_lbl_wifi_title, wifi_titles[l]);
        lv_label_set_text(s_lbl_wifi_body, wifi_bodies[l]);
        lv_obj_set_style_base_dir(s_lbl_wifi_body, bdir, 0);
        lv_obj_set_style_text_align(s_lbl_wifi_body, talign, 0);
    }

    // Card 5: Day-Lock Leasing
    const char *lock_titles[LANG_COUNT] = {
        "سامانه روزشمار و لایسنس صنعتی",
        "Day-Lock Industrial Leasing System",
        "Gun Kilidi Lisans Sistemi",
        "Sistema lizingovoy blokirovki"
    };
    const char *lock_bodies[LANG_COUNT] = {
        "• روزهای باقیمانده اعتبار دستگاه به صورت خودکار و امن در حافظه ثبت می‌شود.\n"
        "• در صورت نزدیک شدن به مهلت پایانی، آلارم هشدار روی صفحه نمایش داده می‌شود.\n"
        "• برای تمدید اعتبار، رمز معتبر دریافتی از کارشناس هافلر را در کادر روزشمار وارد کنید.\n"
        "• یکبار مهلت اضطراری ۱۰ روزه جهت جلوگیری از توقف خط تولید در شرایط اضطراری تعبیه شده است.",

        "• Remaining operating days are recorded monotonically and securely in hardware storage.\n"
        "• Warning banners notify the operator when lease days are running low.\n"
        "• To extend your license, enter the authorized renewal key from Haffler support.\n"
        "• A one-time 10-day emergency grace period is provided to prevent production downtime.",

        "• Kalan calisma gunleri donanim hafizasinda guvenli sekilde sayilir.\n"
        "• Sure azaldiginda ekranda sesli ve gorsel uyari pencereleri cikar.\n"
        "• Lisansi uzatmak icin Haffler destek ekibinden alinan onay sifresini girin.\n"
        "• Uretimin aksamamasi icin bir defaya mahsus 10 gunluk acil durum suresi mevcuttur.",

        "• Ostavshiesya dni raboty fiksiruyutsya v zaschischennoy pamyati.\n"
        "• Pri priblizhenii sroka poyavlyayutsya preduprezhdeniya na ekrane.\n"
        "• Dlya prodleniya vvedite kod ot tehnicheskoy podderzhki Haffler.\n"
        "• Predusmotren odnokratny lgotny period 10 dney dlya izbezhania prostoev."
    };
    if (s_lbl_lock_title && s_lbl_lock_body) {
        lv_label_set_text(s_lbl_lock_title, lock_titles[l]);
        lv_label_set_text(s_lbl_lock_body, lock_bodies[l]);
        lv_obj_set_style_base_dir(s_lbl_lock_body, bdir, 0);
        lv_obj_set_style_text_align(s_lbl_lock_body, talign, 0);
    }

    // Card 6: Support & Contacts
    const char *contact_titles[LANG_COUNT] = {
        "خدمات فنی و پشتیبانی هافلر",
        "Haffler Technical Support & Service",
        "Haffler Teknik Servis ve Destek",
        "Tehnicheskaya podderzhka Haffler"
    };
    const char *contact_bodies[LANG_COUNT] = {
        "شرکت مهندسی ماشین‌آلات صنعتی هافلر\n"
        "• وب‌سایت رسمی: https://haffler-infi.ir\n"
        "• سرور بروزرسانی: ota.haffler-infi.ir\n"
        "• پشتیبانی فنی ۲۴ ساعته و خدمات پس از فروش قطعات نانوایی",

        "Haffler Industrial Machinery Engineering\n"
        "• Official Website: https://haffler-infi.ir\n"
        "• OTA Cloud Server: ota.haffler-infi.ir\n"
        "• 24/7 Technical Support & Genuine Bakery Spares",

        "Haffler Endustriyel Makine Muhendisligi\n"
        "• Resmi Web Sitesi: https://haffler-infi.ir\n"
        "• OTA Guncelleme Sunucusu: ota.haffler-infi.ir\n"
        "• 7/24 Teknik Servis ve Orijinal Firin Yedek Parcalari",

        "Kompaniya Haffler Promyshlennoe Oborudovanie\n"
        "• Ofitsialny sayt: https://haffler-infi.ir\n"
        "• Server obnovleniy: ota.haffler-infi.ir\n"
        "• Kruglosutochnaya podderzhka i zapasnye chasti"
    };
    if (s_lbl_contact_title && s_lbl_contact_body) {
        lv_label_set_text(s_lbl_contact_title, contact_titles[l]);
        lv_label_set_text(s_lbl_contact_body, contact_bodies[l]);
        lv_obj_set_style_base_dir(s_lbl_contact_body, bdir, 0);
        lv_obj_set_style_text_align(s_lbl_contact_body, talign, 0);
    }
}

static lv_obj_t *create_guide_card(lv_obj_t *parent, lv_obj_t **out_title, lv_obj_t **out_body) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, 456);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    *out_title = lv_label_create(card);
    lv_obj_set_width(*out_title, 436);
    lv_obj_set_style_text_color(*out_title, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(*out_title, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(*out_title, LV_ALIGN_TOP_RIGHT, 0, 0);

    *out_body = lv_label_create(card);
    lv_obj_set_width(*out_body, 436);
    lv_label_set_long_mode(*out_body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(*out_body, lv_color_hex(0xE4E4E7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(*out_body, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align_to(*out_body, *out_title, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 8);

    return card;
}

void ui_HelpScreen_screen_init(void) {
    ui_HelpScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_HelpScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_HelpScreen, lv_color_hex(0x18181B), LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Header Bar (H: 48px, Dark Slate) ──
    s_help_header = lv_obj_create(ui_HelpScreen);
    lv_obj_set_size(s_help_header, 480, 48);
    lv_obj_set_pos(s_help_header, 0, 0);
    lv_obj_set_style_bg_color(s_help_header, lv_color_hex(0x27272A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_help_header, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_help_header, lv_color_hex(0x3F3F46), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(s_help_header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_help_header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_help_header, LV_OBJ_FLAG_SCROLLABLE);

    s_help_title_lbl = lv_label_create(s_help_header);
    lv_obj_align(s_help_title_lbl, LV_ALIGN_CENTER, 30, 0);
    lv_label_set_text(s_help_title_lbl, "راهنمای جامع کاربری دستگاه");
    lv_obj_set_style_text_color(s_help_title_lbl, lv_color_hex(0xF4F4F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_help_title_lbl, &ui_font_Dana_Black20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Unified Crimson Red Back Button
    s_help_back_btn = lv_btn_create(s_help_header);
    lv_obj_set_size(s_help_back_btn, 88, 36);
    lv_obj_align(s_help_back_btn, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_color(s_help_back_btn, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_help_back_btn, lv_color_hex(0xDC2626), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_help_back_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_help_back_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_help_back_btn, help_back_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *blbl = lv_label_create(s_help_back_btn);
    lv_label_set_text(blbl, tr(STR_BACK));
    lv_obj_center(blbl);
    lv_obj_set_style_text_color(blbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(blbl, &ui_font_Dana_Bold13, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Scrollable Body Container (H: 272px) ──
    s_help_scroll_container = lv_obj_create(ui_HelpScreen);
    lv_obj_set_size(s_help_scroll_container, 480, 272);
    lv_obj_set_pos(s_help_scroll_container, 0, 48);
    lv_obj_set_style_bg_opa(s_help_scroll_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_help_scroll_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_help_scroll_container, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(s_help_scroll_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_help_scroll_container, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    create_guide_card(s_help_scroll_container, &s_lbl_auto_title, &s_lbl_auto_body);
    create_guide_card(s_help_scroll_container, &s_lbl_manual_title, &s_lbl_manual_body);
    create_guide_card(s_help_scroll_container, &s_lbl_set_title, &s_lbl_set_body);
    create_guide_card(s_help_scroll_container, &s_lbl_wifi_title, &s_lbl_wifi_body);
    create_guide_card(s_help_scroll_container, &s_lbl_lock_title, &s_lbl_lock_body);
    create_guide_card(s_help_scroll_container, &s_lbl_contact_title, &s_lbl_contact_body);
}

void ui_HelpScreen_open(void) {
    if (!ui_HelpScreen) {
        ui_HelpScreen_screen_init();
    }
    update_help_content();
    lv_scr_load(ui_HelpScreen);
}
