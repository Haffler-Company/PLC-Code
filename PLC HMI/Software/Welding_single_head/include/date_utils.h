#ifndef DATE_UTILS_H
#define DATE_UTILS_H

namespace date_utils {

bool is_valid_jalali_date(int jy, int jm, int jd);
void jalali_to_gregorian(int jy, int jm, int jd, int &gy, int &gm, int &gd);
void gregorian_to_jalali(int gy, int gm, int gd, int &jy, int &jm, int &jd);

} // namespace date_utils

#endif // DATE_UTILS_H
