#ifndef FW_CHANGELOG_H
#define FW_CHANGELOG_H

#define FW_CHANGELOG_CUR_VER  "V.1.8.42"
#define FW_CHANGELOG_CUR_AT   ""
#define FW_CHANGELOG_CUR_N    3
static const char * const FW_CHANGELOG_CUR[FW_CHANGELOG_CUR_N] = {
  "- قفل انحصاری کیبوردها و رفع تداخل باز شدن همزمان چند کیپد",
  "- ریشه‌کنی حروف مربعی (Tofu) و اصلاح BiDi چرخش دو نقطه",
  "- ارگونومی ۴۲ پیکسل کلیدهای جهت ادمین و بهینه‌سازی سرعت نمایشگر",
};

#define FW_CHANGELOG_PREV_VER "V.1.8.41"
#define FW_CHANGELOG_PREV_AT  ""
#define FW_CHANGELOG_PREV_N   3
static const char * const FW_CHANGELOG_PREV[FW_CHANGELOG_PREV_N] = {
  "- احیای قطعی ۲ عکس اسپلش بوت با فرمت SPIFFS",
  "- ویزارد راه‌اندازی و اتصال خودکار وای‌فای و همگام‌سازی ساعت جهانی",
  "- سیستم ثبت سوابق خطا و جعبه سیاه صنعتی (Fault Log)",
};

#endif
