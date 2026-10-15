#pragma once

// F5 calendar records are keyed by Solar-Hijri date, not a calculated Hijri date.
// Lunar starts in Iran are published annually; a generic arithmetic/astronomical
// conversion can be one day wrong.  Records below were transcribed and cross-checked
// from the published Iranian calendar for 1405 on 2026-08-01.
// Keep this table year-bounded: add the next published official year before release.

enum F5IranCalendarEvent : uint8_t {
  F5_ICE_FITR, F5_ICE_FITR_2, F5_ICE_SADEGH_MARTYR, F5_ICE_MASOUMEH_BIRTH,
  F5_ICE_REZA_BIRTH, F5_ICE_JAVAD_MARTYR, F5_ICE_ALI_FATEMEH_MARRIAGE,
  F5_ICE_BAQIR_MARTYR, F5_ICE_ARAFAH, F5_ICE_QORBAN, F5_ICE_GHADIR,
  F5_ICE_MUBAHELA, F5_ICE_TASUA, F5_ICE_ASHURA, F5_ICE_SAJJAD_MARTYR,
  F5_ICE_ARBAEEN, F5_ICE_PROPHET_HASAN_MARTYR, F5_ICE_REZA_MARTYR,
  F5_ICE_ASKARI_MARTYR, F5_ICE_MAHDI_IMAMAT, F5_ICE_PROPHET_SADEGH_BIRTH,
  F5_ICE_ZAINAB_BIRTH, F5_ICE_FATEMEH_MARTYR, F5_ICE_FATEMEH_BIRTH,
  F5_ICE_HADI_MARTYR, F5_ICE_JAVAD_BIRTH, F5_ICE_ALI_BIRTH,
  F5_ICE_KAZIM_MARTYR, F5_ICE_MABATH, F5_ICE_HUSSEIN_BIRTH,
  F5_ICE_ABBAS_BIRTH, F5_ICE_AKBAR_BIRTH, F5_ICE_MAHDI_BIRTH,
  F5_ICE_HASAN_BIRTH, F5_ICE_LAYLAT_QADR_19, F5_ICE_ALI_STRUCK,
  F5_ICE_ALI_MARTYR, F5_ICE_LAYLAT_QADR_21, F5_ICE_LAYLAT_QADR_23,
  F5_ICE_QUDS, F5_ICE_ALI_MARTYR_NOWRUZ, F5_ICE_FITR_NOWRUZ,
  F5_ICE_FITR_REPUBLIC, F5_ICE_BAQIR_KHOMEINI, F5_ICE_GHADIR_KHOMEINI,
  F5_ICE_ASKARI_IMAMAT,
};

struct F5IranCalendarEntry {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  F5IranCalendarEvent event;
};

// Only religious/major calendar records are stored.  The list is deliberately
// compact and contains no movable local closures or unverified future dates.
static const F5IranCalendarEntry f5_iran_calendar_entries[] = {
  // 1405
  {1405, 1, 1, F5_ICE_FITR_NOWRUZ}, {1405, 1, 2, F5_ICE_FITR_2},
  {1405, 1, 25, F5_ICE_SADEGH_MARTYR}, {1405, 1, 30, F5_ICE_MASOUMEH_BIRTH},
  {1405, 2, 9, F5_ICE_REZA_BIRTH}, {1405, 2, 27, F5_ICE_JAVAD_MARTYR},
  {1405, 2, 28, F5_ICE_ALI_FATEMEH_MARRIAGE}, {1405, 3, 3, F5_ICE_BAQIR_MARTYR},
  {1405, 3, 5, F5_ICE_ARAFAH}, {1405, 3, 6, F5_ICE_QORBAN},
  {1405, 3, 14, F5_ICE_GHADIR_KHOMEINI}, {1405, 3, 20, F5_ICE_MUBAHELA},
  {1405, 4, 3, F5_ICE_TASUA}, {1405, 4, 4, F5_ICE_ASHURA},
  {1405, 4, 5, F5_ICE_SAJJAD_MARTYR}, {1405, 5, 13, F5_ICE_ARBAEEN},
  {1405, 5, 21, F5_ICE_PROPHET_HASAN_MARTYR}, {1405, 5, 22, F5_ICE_REZA_MARTYR},
  {1405, 5, 30, F5_ICE_ASKARI_IMAMAT},
  {1405, 6, 8, F5_ICE_PROPHET_SADEGH_BIRTH}, {1405, 7, 24, F5_ICE_ZAINAB_BIRTH},
  {1405, 8, 22, F5_ICE_FATEMEH_MARTYR}, {1405, 9, 9, F5_ICE_FATEMEH_BIRTH},
  {1405, 9, 22, F5_ICE_HADI_MARTYR}, {1405, 9, 29, F5_ICE_JAVAD_BIRTH},
  {1405, 10, 2, F5_ICE_ALI_BIRTH}, {1405, 10, 14, F5_ICE_KAZIM_MARTYR},
  {1405, 10, 16, F5_ICE_MABATH}, {1405, 10, 22, F5_ICE_HUSSEIN_BIRTH},
  {1405, 10, 23, F5_ICE_ABBAS_BIRTH}, {1405, 10, 30, F5_ICE_AKBAR_BIRTH},
  {1405, 11, 4, F5_ICE_MAHDI_BIRTH}, {1405, 12, 3, F5_ICE_HASAN_BIRTH},
  {1405, 12, 6, F5_ICE_LAYLAT_QADR_19}, {1405, 12, 7, F5_ICE_ALI_STRUCK},
  {1405, 12, 8, F5_ICE_LAYLAT_QADR_21}, {1405, 12, 9, F5_ICE_ALI_MARTYR},
  {1405, 12, 10, F5_ICE_LAYLAT_QADR_23}, {1405, 12, 14, F5_ICE_QUDS},
  {1405, 12, 19, F5_ICE_FITR}, {1405, 12, 20, F5_ICE_FITR_2},
};

static inline bool f5_iran_calendar_has_year(int jy) { return jy == 1405; }

static inline F5IranCalendarEvent f5_iran_calendar_event(int jy, int jm, int jd) {
  for (const F5IranCalendarEntry &it : f5_iran_calendar_entries) {
    if (it.year == jy && it.month == jm && it.day == jd) return it.event;
  }
  return (F5IranCalendarEvent)0xFF;
}

static inline bool f5_iran_calendar_event_is_holiday(F5IranCalendarEvent event) {
  switch (event) {
    case F5_ICE_FITR:
    case F5_ICE_FITR_2:
    case F5_ICE_SADEGH_MARTYR:
    case F5_ICE_QORBAN:
    case F5_ICE_GHADIR:
    case F5_ICE_TASUA:
    case F5_ICE_ASHURA:
    case F5_ICE_ARBAEEN:
    case F5_ICE_PROPHET_HASAN_MARTYR:
    case F5_ICE_REZA_MARTYR:
    case F5_ICE_ASKARI_MARTYR:
    case F5_ICE_PROPHET_SADEGH_BIRTH:
    case F5_ICE_FATEMEH_MARTYR:
    case F5_ICE_ALI_BIRTH:
    case F5_ICE_MABATH:
    case F5_ICE_MAHDI_BIRTH:
    case F5_ICE_ALI_MARTYR:
    case F5_ICE_ALI_MARTYR_NOWRUZ:
    case F5_ICE_FITR_NOWRUZ:
    case F5_ICE_FITR_REPUBLIC:
    case F5_ICE_BAQIR_KHOMEINI:
    case F5_ICE_GHADIR_KHOMEINI:
    case F5_ICE_ASKARI_IMAMAT:
      return true;
    default:
      return false;
  }
}

static inline const char *f5_iran_calendar_event_text(F5IranCalendarEvent event, bool en) {
  switch (event) {
    case F5_ICE_FITR: return en ? "Eid al-Fitr" : "عید سعید فطر";
    case F5_ICE_FITR_2: return en ? "Eid al-Fitr holiday" : "تعطیل عید فطر";
    case F5_ICE_SADEGH_MARTYR: return en ? "Martyrdom of Imam Sadegh" : "شهادت امام جعفر صادق";
    case F5_ICE_MASOUMEH_BIRTH: return en ? "Birth of Lady Masoumeh" : "ولادت حضرت معصومه";
    case F5_ICE_REZA_BIRTH: return en ? "Birth of Imam Reza" : "ولادت امام رضا";
    case F5_ICE_JAVAD_MARTYR: return en ? "Martyrdom of Imam Javad" : "شهادت امام جواد";
    case F5_ICE_ALI_FATEMEH_MARRIAGE: return en ? "Marriage of Ali and Fatimah" : "سالروز ازدواج امام علی و حضرت فاطمه";
    case F5_ICE_BAQIR_MARTYR: return en ? "Martyrdom of Imam Baqir" : "شهادت امام باقر";
    case F5_ICE_ARAFAH: return en ? "Day of Arafah" : "روز عرفه";
    case F5_ICE_QORBAN: return en ? "Eid al-Adha" : "عید سعید قربان";
    case F5_ICE_GHADIR: return en ? "Eid al-Ghadir" : "عید سعید غدیر";
    case F5_ICE_MUBAHELA: return en ? "Day of Mubahala" : "روز مباهله";
    case F5_ICE_TASUA: return en ? "Tasua" : "تاسوعای حسینی";
    case F5_ICE_ASHURA: return en ? "Ashura" : "عاشورای حسینی";
    case F5_ICE_SAJJAD_MARTYR: return en ? "Martyrdom of Imam Sajjad" : "شهادت امام سجاد";
    case F5_ICE_ARBAEEN: return en ? "Arbaeen" : "اربعین حسینی";
    case F5_ICE_PROPHET_HASAN_MARTYR: return en ? "Demise of Prophet and Imam Hasan" : "رحلت پیامبر و شهادت امام حسن";
    case F5_ICE_REZA_MARTYR: return en ? "Martyrdom of Imam Reza" : "شهادت امام رضا";
    case F5_ICE_ASKARI_MARTYR: return en ? "Martyrdom of Imam Askari" : "شهادت امام حسن عسکری";
    case F5_ICE_MAHDI_IMAMAT: return en ? "Beginning of Imam Mahdi's Imamate" : "آغاز امامت امام زمان";
    case F5_ICE_PROPHET_SADEGH_BIRTH: return en ? "Birth of Prophet and Imam Sadegh" : "ولادت پیامبر و امام صادق";
    case F5_ICE_ZAINAB_BIRTH: return en ? "Birth of Lady Zainab" : "ولادت حضرت زینب";
    case F5_ICE_FATEMEH_MARTYR: return en ? "Martyrdom of Lady Fatimah" : "شهادت حضرت فاطمه زهرا";
    case F5_ICE_FATEMEH_BIRTH: return en ? "Birth of Lady Fatimah" : "ولادت حضرت فاطمه";
    case F5_ICE_HADI_MARTYR: return en ? "Martyrdom of Imam Hadi" : "شهادت امام هادی";
    case F5_ICE_JAVAD_BIRTH: return en ? "Birth of Imam Javad" : "ولادت امام جواد";
    case F5_ICE_ALI_BIRTH: return en ? "Birth of Imam Ali" : "ولادت امام علی";
    case F5_ICE_KAZIM_MARTYR: return en ? "Martyrdom of Imam Kazim" : "شهادت امام موسی کاظم";
    case F5_ICE_MABATH: return en ? "Prophet's Mission" : "مبعث پیامبر";
    case F5_ICE_HUSSEIN_BIRTH: return en ? "Birth of Imam Hussein" : "ولادت امام حسین";
    case F5_ICE_ABBAS_BIRTH: return en ? "Birth of Abbas" : "ولادت حضرت ابوالفضل";
    case F5_ICE_AKBAR_BIRTH: return en ? "Birth of Ali Akbar" : "ولادت حضرت علی اکبر";
    case F5_ICE_MAHDI_BIRTH: return en ? "Birth of Imam Mahdi" : "ولادت امام زمان";
    case F5_ICE_HASAN_BIRTH: return en ? "Birth of Imam Hasan" : "ولادت امام حسن";
    case F5_ICE_LAYLAT_QADR_19: return en ? "Night of Qadr (19 Ramadan)" : "شب قدر";
    case F5_ICE_ALI_STRUCK: return en ? "Imam Ali was struck" : "ضربت خوردن امام علی";
    case F5_ICE_ALI_MARTYR: return en ? "Martyrdom of Imam Ali" : "شهادت امام علی";
    case F5_ICE_LAYLAT_QADR_21: return en ? "Night of Qadr (21 Ramadan)" : "شب قدر";
    case F5_ICE_LAYLAT_QADR_23: return en ? "Night of Qadr (23 Ramadan)" : "شب قدر";
    case F5_ICE_QUDS: return en ? "International Quds Day" : "روز جهانی قدس";
    case F5_ICE_ALI_MARTYR_NOWRUZ: return en ? "Imam Ali martyrdom / Nowruz" : "شهادت امام علی و نوروز";
    case F5_ICE_FITR_NOWRUZ: return en ? "Eid al-Fitr / Nowruz" : "عید فطر و آغاز نوروز";
    case F5_ICE_FITR_REPUBLIC: return en ? "Eid al-Fitr / Republic Day" : "عید فطر و روز جمهوری اسلامی";
    case F5_ICE_BAQIR_KHOMEINI: return en ? "Imam Baqir martyrdom / Khomeini" : "شهادت امام باقر و رحلت امام خمینی";
    case F5_ICE_GHADIR_KHOMEINI: return en ? "Eid al-Ghadir / Khomeini" : "عید غدیر و رحلت امام خمینی";
    case F5_ICE_ASKARI_IMAMAT: return en ? "Imam Askari martyrdom / Imam Mahdi's Imamate"
                                          : "شهادت امام عسکری و آغاز امامت امام زمان";
    default: return NULL;
  }
}
