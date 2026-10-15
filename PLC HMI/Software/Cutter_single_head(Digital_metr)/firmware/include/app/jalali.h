#pragma once
// Jalali <-> Gregorian helpers for soft clock

static inline void jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd) {
  int jy2 = jy - 979;
  int jm2 = jm - 1;
  int jd2 = jd - 1;
  int j_day_no = 365 * jy2 + (jy2 / 33) * 8 + ((jy2 % 33) + 3) / 4;
  for (int i = 0; i < jm2; ++i) j_day_no += (i < 6) ? 31 : 30;
  j_day_no += jd2;
  int g_day_no = j_day_no + 79;
  int gy2 = 1600 + 400 * (g_day_no / 146097);
  g_day_no = g_day_no % 146097;
  bool leap = true;
  if (g_day_no >= 36525) {
    g_day_no--;
    gy2 += 100 * (g_day_no / 36524);
    g_day_no = g_day_no % 36524;
    if (g_day_no >= 365) g_day_no++;
    else leap = false;
  }
  gy2 += 4 * (g_day_no / 1461);
  g_day_no %= 1461;
  if (g_day_no >= 366) {
    leap = false;
    g_day_no--;
    gy2 += g_day_no / 365;
    g_day_no = g_day_no % 365;
  }
  static const int g_days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
  int gm2 = 0;
  for (gm2 = 0; gm2 < 13; gm2++) {
    int v = g_days[gm2] + ((gm2 > 1 && leap) ? 1 : 0);
    if (g_day_no < v) break;
  }
  int gd2 = g_day_no - g_days[gm2 - 1] - ((gm2 > 2 && leap) ? 1 : 0) + 1;
  *gy = gy2; *gm = gm2; *gd = gd2;
}

static inline void gregorian_to_jalali(int gy, int gm, int gd, int *jy, int *jm, int *jd) {
  static const int g_d_m[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  int gy2 = gy - 1600;
  int gm2 = gm - 1;
  int gd2 = gd - 1;
  int g_day_no = 365 * gy2 + (gy2 + 3) / 4 - (gy2 + 99) / 100 + (gy2 + 399) / 400;
  for (int i = 0; i < gm2; ++i) g_day_no += g_d_m[i + 1] - g_d_m[i];
  if (gm2 > 1 && ((gy % 4 == 0 && gy % 100 != 0) || (gy % 400 == 0))) g_day_no++;
  g_day_no += gd2;
  int j_day_no = g_day_no - 79;
  int j_np = j_day_no / 12053;
  j_day_no %= 12053;
  int jy2 = 979 + 33 * j_np + 4 * (j_day_no / 1461);
  j_day_no %= 1461;
  if (j_day_no >= 366) {
    jy2 += (j_day_no - 1) / 365;
    j_day_no = (j_day_no - 1) % 365;
  }
  int jm2, jd2;
  if (j_day_no < 186) {
    jm2 = 1 + j_day_no / 31;
    jd2 = 1 + (j_day_no % 31);
  } else {
    jm2 = 7 + (j_day_no - 186) / 30;
    jd2 = 1 + ((j_day_no - 186) % 30);
  }
  *jy = jy2; *jm = jm2; *jd = jd2;
}

static inline int64_t civil_to_epoch(int y, int m, int d, int hh, int mm, int ss) {
  y -= (m <= 2);
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days = (int64_t)era * 146097 + (int64_t)doe - 719468;
  return days * 86400LL + hh * 3600LL + mm * 60LL + ss;
}

static inline void epoch_to_civil(int64_t epoch, int *y, int *m, int *d, int *hh, int *mm, int *ss) {
  int64_t z = epoch / 86400LL;
  int64_t rem = epoch % 86400LL;
  if (rem < 0) { rem += 86400LL; z--; }
  *hh = (int)(rem / 3600);
  *mm = (int)((rem % 3600) / 60);
  *ss = (int)(rem % 60);
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y2 = (int)(yoe) + (int)era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned d2 = doy - (153 * mp + 2) / 5 + 1;
  const unsigned m2 = mp < 10 ? mp + 3 : mp - 9;
  y2 += (m2 <= 2);
  *y = y2; *m = (int)m2; *d = (int)d2;
}

static inline int64_t midnight_of(int64_t epoch) {
  int y, m, d, hh, mm, ss;
  epoch_to_civil(epoch, &y, &m, &d, &hh, &mm, &ss);
  return civil_to_epoch(y, m, d, 0, 0, 0);
}
