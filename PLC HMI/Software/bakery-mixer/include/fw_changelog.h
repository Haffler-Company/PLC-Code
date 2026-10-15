#pragma once
// Haffler Industrial Firmware Changelog (Cumulative Rollup)
// Hard limit: Maximum 13 words per item

#define FW_VERSION_STR "V.1.3.0"

static const char* s_changelog_fa[] = {
    "۱=زمان‌بندی ۳ شیفت کاری با ساعت سخت‌افزاری DS3231",
    "۲=پیش‌تنظیم خودکار دمای آب مخزن پیش از شروع شیفت",
    "۳=کالیبراسیون دبی آب و تبدیل لیتر به ثانیه",
    "۴=کنترل ترمز اصطکاکی کاسه چرخان در حین میکس",
    "۵=پشتیبانی از هاپر اتوماتیک آرد و حالت دستی",
    "۶=رابط کاربری اپل با نمایش فازها و کلاک زنده",
    "۷=سامانه قفل ۵ مرحله‌ای اقساط با تقویم شمسی جلالی",
    "۸=وب‌اپلیکیشن اختصاصی با تله‌متری بلادرنگ و تنظیم ساعت",
    "۹=طراحی یکپارچه تنظیمات عمومی و پایپ‌لاین بصری ۵ مرحله‌ای"
};

static const char* s_changelog_en[] = {
    "1=Triple-shift daily scheduler via DS3231 hardware RTC",
    "2=Pre-conditioning water tank temperature before shift start",
    "3=Water flowrate calibration with automatic liters-to-time conversion",
    "4=Pulsed rotating bowl friction brake during kneading",
    "5=Support for motorized flour auger and manual modes",
    "6=Apple HIG HMI with phase flow and live clock",
    "7=5-stage cascading day-lock engine with Jalali solar calendar",
    "8=Dedicated webapp with real-time telemetry and clock adjustment",
    "9=Unified Settings Hub and 5-stage graphical process pipeline"
};

static const char* s_changelog_tr[] = {
    "1=DS3231 donanim RTC ile gunluk 3 vardiya planlayici",
    "2=Vardiya baslamadan once su kazani sicakliginin on kosullandirilmasi",
    "3=Su debisi kalibrasyonu ve otomatik litre-sure donusumu",
    "4=Yogurma sirasinda donen kase surtunme freni darbesi",
    "5=Motorlu un helezonu ve manuel besleme destegi",
    "6=Faz akisi ve canli saatli Apple HIG HMI",
    "7=Celali takvimli 5 asamali basamakli taksit kilidi motoru",
    "8=Canli telemetri ve saat ayarli ozel web uygulamasi",
    "9=Birlesik Ayarlar Merkezi ve 5 asamali gorsel islem hatti"
};
