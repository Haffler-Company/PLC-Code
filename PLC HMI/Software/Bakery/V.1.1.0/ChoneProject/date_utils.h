#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool is_valid_jalali_date(int jy, int jm, int jd);
void jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd);
void gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd);

#ifdef __cplusplus
}
#endif

#endif // DATE_UTILS_H
