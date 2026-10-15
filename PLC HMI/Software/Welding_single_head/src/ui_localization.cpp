#include "ui_localization.h"
#include "vars.h"

namespace ui_localization {

int32_t get_ui_language_index() {
    int32_t lang = get_var_lang();
    if (lang == 2) return 1; // English
    if (lang == 3) return 2; // Turkish
    return 0;                // Persian
}

const char *get_wrong_password_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Wrong password";
        case 2:
            return "Hatali sifre";
        default:
            return "رمز ورود اشتباه است";
    }
}

const char *get_warr_title_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Haffler UPVC Single-Head Welder Warranty Activation";
        case 2:
            return "Haffler UPVC Tek Kose Kaynak Garanti Etkinlestirme";
        default:
            return "فعالسازی گارانتی رسمی دستگاه جوش هافلر";
    }
}

const char *get_warr_name_err_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Please enter buyer name";
        case 2:
            return "Lutfen alici adini girin";
        default:
            return "لطفاً نام خریدار را وارد کنید";
    }
}

const char *get_warr_phone_err_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Invalid phone number (09xxxxxxxxx)";
        case 2:
            return "Gecersiz telefon numarasi (09xxxxxxxxx)";
        default:
            return "شماره تماس معتبر نیست (09xxxxxxxxx)";
    }
}

const char *get_warr_submit_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Register & Activate";
        case 2:
            return "Kaydet ve Etkinlestir";
        default:
            return "ثبت و فعالسازی";
    }
}

const char *get_warr_offline_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Offline Activation";
        case 2:
            return "Cevrimdisi Etkinlestir";
        default:
            return "فعالسازی آفلاین";
    }
}

const char *get_warr_cancel_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Cancel";
        case 2:
            return "Iptal";
        default:
            return "انصراف";
    }
}

const char *get_warr_prompt_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Enter buyer info and tap Register";
        case 2:
            return "Alici bilgilerini girin ve Kaydet'e basin";
        default:
            return "اطلاعات خریدار را وارد و کلید ثبت را لمس کنید";
    }
}

const char *get_warr_expired_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "15-minute test grace period expired";
        case 2:
            return "15 dakikalik test suresi doldu";
        default:
            return "مهلت تست 15 دقیقه دستگاه پایان یافت";
    }
}

const char *get_warr_expired_desc() {
    switch (get_ui_language_index()) {
        case 1:
            return "To resume operation, heat up the plate, and start welding cycles, please activate the official warranty.\n"
                   "Activation is available online or via offline technician PIN.";
        case 2:
            return "Calismaya devam etmek, plakayi isitmak ve kaynak cevrimini baslatmak icin lutfen garantiyi etkinlestirin.\n"
                   "Etkinlestirme cevrimici veya cevrimdisi teknisyen PIN koduyla yapilabilir.";
        default:
            return "برای ادامه کار، گرم شدن اتو و شروع چرخه جوشکاری، گارانتی دستگاه را فعال کنید.\n"
                   "فعالسازی از طریق اینترنت یا با رمز آفلاین امکانپذیر است.";
    }
}

const char *get_warr_dismiss_text() {
    switch (get_ui_language_index()) {
        case 1:
            return "Dismiss";
        case 2:
            return "Kapat";
        default:
            return "بستن پیام";
    }
}

const char *get_warr_code_prefix() {
    switch (get_ui_language_index()) {
        case 1:
            return "Customer Code: ";
        case 2:
            return "Musteri Kodu: ";
        default:
            return "کد مشتری: ";
    }
}

const char *get_warr_name_ph() {
    switch (get_ui_language_index()) {
        case 1:
            return "Buyer Name";
        case 2:
            return "Alici Adi";
        default:
            return "نام خریدار";
    }
}

const char *get_warr_city_ph() {
    switch (get_ui_language_index()) {
        case 1:
            return "City";
        case 2:
            return "Sehir";
        default:
            return "شهر";
    }
}

const char *get_warr_no_net_title() {
    switch (get_ui_language_index()) {
        case 1:
            return "No Internet Connection";
        case 2:
            return "Internet Baglantisi Yok";
        default:
            return "عدم برقراری ارتباط با اینترنت";
    }
}

const char *get_warr_no_net_desc() {
    switch (get_ui_language_index()) {
        case 1:
            return "Device is offline. You can activate offline; activation SMS will be sent automatically upon next Wi-Fi connection.";
        case 2:
            return "Cihaz cevrimdisi. Cevrimdisi etkinlestirebilirsiniz; SMS ilk baglantida otomatik olarak gonderilecektir.";
        default:
            return "دستگاه به اینترنت وصل نیست. می‌توانید به صورت آفلاین فعال نمایید تا پیامک پس از اولین اتصال ارسال شود.";
    }
}

} // namespace ui_localization
