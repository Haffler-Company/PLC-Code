#include "date_utils.h"

namespace date_utils {

bool is_valid_jalali_date(int jy, int jm, int jd) {
    if (jy < 1200 || jm < 1 || jm > 12 || jd < 1) {
        return false;
    }

    int max_day = 31;
    if (jm >= 7 && jm <= 11) {
        max_day = 30;
    } else if (jm == 12) {
        int gy, gm, gd, b_jy, b_jm, b_jd;
        jalali_to_gregorian(jy, 12, 30, gy, gm, gd);
        gregorian_to_jalali(gy, gm, gd, b_jy, b_jm, b_jd);
        max_day = (b_jm == 12 && b_jd == 30) ? 30 : 29;
    }
    return jd <= max_day;
}

void jalali_to_gregorian(int jy, int jm, int jd, int &gy, int &gm, int &gd) {
    static const int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    static const int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    long jy2 = jy - 979;
    long jm2 = jm - 1;
    long jd2 = jd - 1;

    long j_day_no = 365 * jy2 + (jy2 / 33) * 8 + ((jy2 % 33 + 3) / 4);
    for (long i = 0; i < jm2; ++i) {
        j_day_no += j_days_in_month[i];
    }
    j_day_no += jd2;

    long g_day_no = j_day_no + 79;
    gy = (int)(1600 + 400 * (g_day_no / 146097));
    g_day_no %= 146097;

    bool leap = true;
    if (g_day_no >= 36525) {
        g_day_no--;
        gy += (int)(100 * (g_day_no / 36524));
        g_day_no %= 36524;
        if (g_day_no >= 365) {
            g_day_no++;
        } else {
            leap = false;
        }
    }

    gy += (int)(4 * (g_day_no / 1461));
    g_day_no %= 1461;

    if (g_day_no >= 366) {
        leap = false;
        g_day_no--;
        gy += (int)(g_day_no / 365);
        g_day_no %= 365;
    }

    int i = 0;
    while (g_day_no >= g_days_in_month[i] + ((i == 1 && leap) ? 1 : 0)) {
        g_day_no -= g_days_in_month[i] + ((i == 1 && leap) ? 1 : 0);
        i++;
    }
    gm = i + 1;
    gd = (int)g_day_no + 1;
}

void gregorian_to_jalali(int gy, int gm, int gd, int &jy, int &jm, int &jd) {
    static const int g_days_to_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int gy2 = (gm > 2) ? (gy + 1) : gy;
    long days = 355666L + (365L * gy) + ((gy2 + 3) / 4) - ((gy2 + 99) / 100) + ((gy2 + 399) / 400) + gd + g_days_to_month[gm - 1];

    jy = (int)(-1595 + 33 * (days / 12053));
    days %= 12053;
    jy += (int)(4 * (days / 1461));
    days %= 1461;

    if (days > 365) {
        jy += (int)((days - 1) / 365);
        days = (days - 1) % 365;
    }

    if (days < 186) {
        jm = (int)(1 + days / 31);
        jd = (int)(1 + (days % 31));
    } else {
        jm = (int)(7 + ((days - 186) / 30));
        jd = (int)(1 + ((days - 186) % 30));
    }
}

} // namespace date_utils
