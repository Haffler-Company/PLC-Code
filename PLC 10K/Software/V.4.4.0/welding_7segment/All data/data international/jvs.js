var language = "EN";
var current_temp = 0;
var temp_setPoint = 0;
let currentJack = {};

var JStates = {
  init: 0,
  loading: 1,
  loaded: 2,
  unloading: 3,
  unloaded: 4,
};
var jackState = JStates.init;
function openLoading() {
  document.getElementById("HelperImage").src = "logo.png";
  document.getElementById("HelperButton").style.display = "none";
  openModal("HelperModal");
}
function closeLoading() {
  closeModal("HelperModal");
}
openLoading();

const mapper = {
  FA: {
    home: "صفحه اصلی",
    "gnral-sett": "تنظیمات عمومی",
    "heat-sett": "زمان بندی جوش",
    "hyd-sett": "زمان بندی جک ها",
    "timer-sett": "تنظیمات مدیر",
    "ota-sett": "بروزرسانی برنامه",
    "supp-page": "پشتیبانی",
    active_program: "برنامه فعال",
    iron_temp: "دمای اتو",
    melting_time: "زمان گرم سازی",
    cooling_time: "زمان خنک سازی",
    change_program: "تغییر برنامه کاری",
    temp_calib: "کالیبره دما",
    pid_coeff: "ضریب PID",
    speaker: "اسپیکر",
    speaker_off: "غیرفعال",
    speaker_on: "فعال",
    autostart: "استارت خودکار",
    autostart_off: "غیرفعال",
    autostart_on: "فعال",
    end_mode: "جک پایانی",
    end_mode_step: "سریع",
    end_mode_fast: "پلکانی",
    emergency: "سوییچ اضطراری",
    emergency_open: "عادی بسته",
    emergency_closed: "باز",
    Jsensor: "سنسور جک",
    Jsensor_on: "فعال",
    Jsensor_off: "غیرفعال",
    pedalN: "تعداد پدال",
    Np3: "3",
    Np4: "4",
    temp_limit: "محدودیت دما",
    temp_limit_off: "غیرفعال",
    temp_limit_on: "فعال",
    saveStates: "ثبت وضعیت",
    op_mode: "حالت کار",
    op_mode_1: "جوش تک سر",
    op_mode_2: "جوش دو سر",
    termo: "ترموکوبل",
    rst_count: "ریست کردن شمارنده",
    save_defs: "ذخیره مقادیر پشتیبان",
    KSWCH: "تایمر معکوس",
    days_label: "روز",
    save_timer: "ذخیره",
    ch_pass: "تغییر رمز عبور",
    password_hint: "رمز انتخابی باید یک عدد ۶ رقمی باشد",
    sub_pass_ch: "تغییر رمز عبور",
    act_date_label: "تاریخ فعالسازی",
    os_updater_label: "آپدیت سیستم عامل:",
    asset_label: "اپدیت برنامه:",
    submit_update: "آپدیت",
    submit_os: "آپدیت",
    prg_os: "progress: 0%",
    prg_html: "progress: 0%",
    rst_factory: "بازیابی تنظیمات کارخانه",
    passwordError: "رمز اشتباه است",
    conf_pass_butt: "تایید",
    incoming: "رفت",
    outgoing: "برگشت",
    submitjack_butt: "تایید",
    heaterTitle: "گرم سازی",
    coolerTitle: "خنک سازی",
    tempTitle: "دمای اتو",
    submitProgram_butt: "تایید",
    confirm_modal_title: "تایید عملیات",
    confirmText: "آیا از انجام عملیات مطمئن هستید؟",
    confirmButton: "تایید",
    MessageButton: "باشه",
    HelperButton: "باشه",
    changePassInput: "رمز جدید",
    changePassConfirmInput: "تایید رمز",
    passwordInput: "رمز عبور",
  },
  TU: {
    home: "Ana Sayfa",
    "gnral-sett": "Genel Ayarlar",
    "heat-sett": "Isıtıcı Zamanlama",
    "hyd-sett": "Kriko Zamanlama",
    "timer-sett": "Yönetici Ayarları",
    "ota-sett": "Program Güncellemesi",
    "supp-page": "Destek",
    active_program: "Aktif Program",
    iron_temp: "Ütü Sıcaklığı",
    melting_time: "Eritme Süresi",
    cooling_time: "Soğutma Süresi",
    change_program: "Çalışma Programını Değiştir",
    temp_calib: "Sıcaklık Kalibrasyonu",
    pid_coeff: "PID Katsayısı",
    speaker: "Hoparlör",
    speaker_off: "Kapalı",
    speaker_on: "Açık",
    autostart: "Oto Başlat",
    autostart_off: "Kapalı",
    autostart_on: "Açık",
    end_mode: "Bitiş jakı",
    end_mode_step: "Hızlı",
    end_mode_fast: "Aşamalı",
    emergency: "Acil Durum",
    emergency_open: "NC",
    emergency_closed: "NO",
    Jsensor: "Kriko Sensörü",
    Jsensor_on: "Etkin",
    Jsensor_off: "Pasif",
    pedalN: "Pedal Sayımları",
    Np3: "3",
    Np4: "4",
    temp_limit: "Sıcaklık Sınırı",
    temp_limit_off: "Kapalı",
    temp_limit_on: "Açık",
    saveStates: "Durumu Kaydet",
    op_mode: "Çalışma Modu",
    op_mode_1: "Tek Taraf Kaynağı",
    op_mode_2: "Çift Taraf Kaynağı",
    termo: "Termokapıl",
    rst_count: "Sayaçları Sıfırla",
    save_defs: "Yedek Değerleri Kaydet",
    KSWCH: "Geri Sayım",
    days_label: "Gün",
    save_timer: "Kaydet",
    ch_pass: "Şifre Değiştir",
    password_hint: "Seçilen şifre 6 haneli bir sayı olmalıdır",
    sub_pass_ch: "Şifre Değiştir",
    act_date_label: "Aktivasyon",
    os_updater_label: "OS Güncelleyici:",
    asset_label: "Program Güncellemesi:",
    submit_update: "Güncelle",
    submit_os: "Güncelle",
    prg_os: "İlerleme: 0%",
    prg_html: "İlerleme: 0%",
    rst_factory: "Fabrika Ayarlarına Dön",
    passwordError: "Şifre Hatalı",
    conf_pass_butt: "Onayla",
    incoming: "Gidiş",
    outgoing: "Dönüş",
    submitjack_butt: "Onayla",
    heaterTitle: "Isıtma",
    coolerTitle: "Soğutma",
    tempTitle: "Ütü Sıcaklığı",
    submitProgram_butt: "Onayla",
    confirm_modal_title: "İşlem Onayı",
    confirmText: "Bu işlemi yapmak istediğinizden emin misiniz?",
    confirmButton: "Onayla",
    MessageButton: "Tamam",
    HelperButton: "Tamam",
    changePassInput: "Yeni Şifre",
    changePassConfirmInput: "Şifreyi Onayla",
    passwordInput: "Şifre",
  },
  EN: {
    home: "Home",
    "gnral-sett": "General Settings",
    "heat-sett": "Welding Timing",
    "hyd-sett": "Jack Timing",
    "timer-sett": "Admin Settings",
    "ota-sett": "Program Update",
    "supp-page": "Support",
    active_program: "Active Program",
    iron_temp: "Iron Temperature",
    melting_time: "Melting Time",
    cooling_time: "Cooling Time",
    change_program: "Change Working Program",
    temp_calib: "Temperature Calibration",
    pid_coeff: "PID Coefficient",
    speaker: "Speaker",
    speaker_off: "Off",
    speaker_on: "On",
    autostart: "Auto Start",
    autostart_off: "Off",
    autostart_on: "On",
    end_mode: "Ending Jack",
    end_mode_step: "Fast",
    end_mode_fast: "Stepwise",
    emergency: "Emergency Switch",
    emergency_open: "N-Closed",
    emergency_closed: "N-Open",
    Jsensor: "Jack Sensor",
    Jsensor_on: "On",
    Jsensor_off: "Off",
    pedalN: "Pedal Count",
    Np3: "3",
    Np4: "4",
    temp_limit: "Temperature Limit",
    temp_limit_off: "Off",
    temp_limit_on: "On",
    saveStates: "Save States",
    op_mode: "Operation Mode",
    op_mode_1: "Single Side Welding",
    op_mode_2: "Double Side Welding",
    termo: "Thermocouple",
    rst_count: "Reset Counters",
    save_defs: "Save Backup Values",
    KSWCH: "Countdown Timer",
    days_label: "Days",
    save_timer: "Save",
    ch_pass: "Change Password",
    password_hint: "Selected password must be a 6-digit number",
    sub_pass_ch: "Change Password",
    act_date_label: "Activation",
    os_updater_label: "OS Updater:",
    asset_label: "Program Update:",
    submit_update: "Update",
    submit_os:"Update",
    prg_os: "progress: 0%",
    prg_html: "progress: 0%",
    rst_factory: "Reset to Factory Settings",
    passwordError: "Password Incorrect",
    conf_pass_butt: "Confirm",
    incoming: "Incoming",
    outgoing: "Outgoing",
    submitjack_butt: "Confirm",
    heaterTitle: "Heating",
    coolerTitle: "Cooling",
    tempTitle: "Iron Temperature",
    submitProgram_butt: "Confirm",
    confirm_modal_title: "Operation Confirmation",
    confirmText: "Are you sure you want to proceed with this operation?",
    confirmButton: "Confirm",
    MessageButton: "OK",
    HelperButton: "OK",
    changePassInput: "New Password",
    changePassConfirmInput: "Confirm Password",
    passwordInput: "Password",
  },
  RU: {
    home: "Главная",
    "gnral-sett": "Основные настройки",
    "heat-sett": "Тайминг сварки",
    "hyd-sett": "Тайминг домкрата",
    "timer-sett": "Настройки администратора",
    "ota-sett": "Обновление программы",
    "supp-page": "Поддержка",
    active_program: "Активная программа",
    iron_temp: "Температура утюга",
    melting_time: "Время плавления",
    cooling_time: "Время охлаждения",
    change_program: "Изменить рабочую программу",
    temp_calib: "Калибровка температуры",
    pid_coeff: "Коэффициент PID",
    speaker: "Динамик",
    speaker_off: "Выкл",
    speaker_on: "Вкл",
    autostart: "Автозапуск",
    autostart_off: "Выкл",
    autostart_on: "Вкл",
    end_mode: "Завершение домкрата",
    end_mode_step: "Быстро",
    end_mode_fast: "Пошагово",
    emergency: "Аварийный выключатель",
    emergency_open: "N-Закрыт",
    emergency_closed: "N-Открыт",
    Jsensor: "Датчик домкрата",
    Jsensor_on: "Вкл",
    Jsensor_off: "Выкл",
    pedalN: "Количество педалей",
    Np3: "3",
    Np4: "4",
    temp_limit: "Лимит температуры",
    temp_limit_off: "Выкл",
    temp_limit_on: "Вкл",
    saveStates: "Сохранить состояния",
    op_mode: "Режим работы",
    op_mode_1: "Односторонняя сварка",
    op_mode_2: "Двусторонняя сварка",
    termo: "Термопара",
    rst_count: "Сбросить счетчики",
    save_defs: "Сохранить резервные значения",
    KSWCH: "Таймер обратного отсчета",
    days_label: "Дни",
    save_timer: "Сохранить",
    ch_pass: "Изменить пароль",
    password_hint: "Пароль должен быть 6-значным числом",
    sub_pass_ch: "Изменить пароль",
    act_date_label: "Активация",
    os_updater_label: "Обновление ОС:",
    asset_label: "Обновление программы:",
    submit_update: "Обновить",
    submit_os: "Обновить",
    prg_os: "прогресс: 0%",
    prg_html: "прогресс: 0%",
    rst_factory: "Сброс к заводским настройкам",
    passwordError: "Неверный пароль",
    conf_pass_butt: "Подтвердить",
    incoming: "Входящий",
    outgoing: "Исходящий",
    submitjack_butt: "Подтвердить",
    heaterTitle: "Нагрев",
    coolerTitle: "Охлаждение",
    tempTitle: "Температура утюга",
    submitProgram_butt: "Подтвердить",
    confirm_modal_title: "Подтверждение операции",
    confirmText: "Вы уверены, что хотите выполнить эту операцию?",
    confirmButton: "Подтвердить",
    MessageButton: "ОК",
    HelperButton: "ОК",
    changePassInput: "Новый пароль",
    changePassConfirmInput: "Подтвердите пароль",
    passwordInput: "Пароль",
  },
};
const mapper2 = {
  FA: {
    notifyTimerError: "خطای تایمر",
    notifyRequestSuccess: "درخواست با موفقیت ثبت شد",
    notifySaveDefaults: "تنظیمات پیش فرض به روز شد",
    notifyWrongPassword: "رمز وارد شده اشتباه است.",
    limitationError: "بازه وارد شده غیر مجاز",
    confirmResetAllCounters: "آیا از پاک کردن تمام شمارنده ها مطمئن هستید؟",
    confirmResetFactory: "بازگردانی تنظیمات به وضعیت پیشفرض",
    confirmResetCounter: "بازگردانی شمارنده به وضعیت پیشفرض",
    updateSucceed:"اپدیت موفق",
    updateFailed:"اپدیت ناموفق",
  },
  TU: {
    notifyTimerError: "Taymer Hatası",
    notifyRequestSuccess: "İstek başarıyla kaydedildi",
    notifySaveDefaults: "Varsayılan ayarlar güncellendi",
    notifyWrongPassword: "Girilen şifre yanlış.",
    limitationError: "Girilen aralık geçersiz",
    confirmResetAllCounters:
      "Tüm sayaçları sıfırlamak istediğinizden emin misiniz?",
    confirmResetFactory: "Ayarları varsayılan duruma döndür",
    confirmResetCounter: "Sayacı varsayılan duruma sıfırla",
    updateSucceed:"Güncelleme başarılı",
    updateFailed:"Güncelleme başarısız",
  },
  EN: {
    notifyTimerError: "Timer Error",
    notifyRequestSuccess: "Request registered successfully",
    notifySaveDefaults: "Default settings updated",
    notifyWrongPassword: "The entered password is incorrect.",
    limitationError: "The entered range is invalid",
    confirmResetAllCounters: "Are you sure you want to reset all counters?",
    confirmResetFactory: "Reset settings to default state",
    confirmResetCounter: "Reset counter to default state",
    updateSucceed:"Update successful",
    updateFailed:"Update failed",
  },
  RU: {
    notifyTimerError: "Ошибка таймера",
    notifyRequestSuccess: "Запрос успешно зарегистрирован",
    notifySaveDefaults: "Настройки по умолчанию обновлены",
    notifyWrongPassword: "Введенный пароль неверен.",
    limitationError: "Введенный диапазон недопустим",
    confirmResetAllCounters: "Вы уверены, что хотите сбросить все счетчики?",
    confirmResetFactory: "Сбросить настройки до состояния по умолчанию",
    confirmResetCounter: "Сбросить счетчик до состояния по умолчанию",
    updateSucceed: "Обновление успешно",
    updateFailed: "Обновление не удалось",
  },
};
const jackLabels = {
  EN: [
    "Sheet Jack",
    "Shablon Jack",
    "Profile Jack",
    "Welding Jack",
    "0.2 (Shocker1)",
    "0.2 (Shocker2)",
  ],
  FA: [
    "جک صفحه",
    "جک شابلون",
    "جک پروفیل",
    "جک جوش",
    "جک ۰.۲ (ضربه۱)",
    "جک ۰.۲ (ضربه۲)",
  ],
  TU: [
    "Sayfa Carkı",
    "Kalıp Carkı",
    "Profil Carkı",
    "Kaynak Carkı",
    "0.2 (Darbe1)",
    "0.2 (Darbe2)",
  ],
  RU: [
    "Листовой домкрат",
    "Шаблонный домкрат",
    "Профильный домкрат",
    "Сварочный домкрат",
    "0.2 (Шокер1)",
    "0.2 (Шокер2)",
  ],
};
const questions_responses = {
  FA: {
    1: {
      question: "چرا دما دستگاهم بیشتر از حد مشخص شده میره و بالا زدگی داره؟",
      response: "عدد PID باید کاهش پیدا کند.",
    },
    2: {
      question: "چرا دما دستگاهم به حد مشخص شده نمیرسه و پایین زدگی داره؟",
      response: "عدد PID باید افزایش پیدا کند.",
    },
    3: {
      question: "عدد PID باید چند باشه؟",
      response:
        "پیشفرض عدد PID عدد 15 میباشد. نسبت به دمای المنت این عدد باید مشخص شود و در صورت بالازدگی کاهش و در صورت پایین زدگی افزایش پیدا کند.",
    },
    4: {
      question: "دما المنت و زمان گرم سازی و خنک سازی باید چقدر باشد؟",
      response:
        "این مقادیر باید مطابق توان دستگاه و نوع پروفیل شما انتخاب شود. می توانید با بررسی مقادیر مختلف و مشاهده نتیجه کار به ایده آل ترین حالت برسید.",
    },
  },
  EN: {
    1: {
      question:
        "Why does the temperature of my device exceed the specified limit and overshoot?",
      response: "The PID value should be decreased.",
    },
    2: {
      question:
        "Why does the temperature of my device fail to reach the specified limit and undershoot?",
      response: "The PID value should be increased.",
    },
    3: {
      question: "What should the PID value be?",
      response:
        "The default PID value is 15. It should be adjusted based on the heating element's temperature. Reduce it for overshooting and increase it for undershooting.",
    },
    4: {
      question:
        "What should the heating element temperature, heating, and cooling times be?",
      response:
        "These values should match your device's power and profile type. You can find the optimal setting by testing different values and observing the results.",
    },
  },
  TU: {
    1: {
      question:
        "Cihazımın sıcaklığı neden belirtilen sınırı aşıyor ve fazla yükseliyor?",
      response: "PID değeri düşürülmelidir.",
    },
    2: {
      question:
        "Cihazımın sıcaklığı neden belirtilen sınırına ulaşamıyor ve düşük kalıyor?",
      response: "PID değeri artırılmalıdır.",
    },
    3: {
      question: "PID değeri ne olmalı?",
      response:
        "Varsayılan PID değeri 15'tir. Bu değer, ısıtıcı elemanın sıcaklığına göre ayarlanmalıdır. Aşma durumunda azaltılmalı, düşük kalma durumunda artırılmalıdır.",
    },
    4: {
      question:
        "Isıtıcı elemanın sıcaklığı, ısıtma ve soğutma süreleri ne olmalıdır?",
      response:
        "Bu değerler, cihazınızın gücüne ve profil tipine göre seçilmelidir. Farklı değerleri test ederek ve sonuçları gözlemleyerek ideal ayarı bulabilirsiniz.",
    },
  },
  RU: {
    1: {
      question: "Почему температура моего устройства превышает заданный лимит и происходит перегрев?",
      response: "Значение PID следует уменьшить.",
    },
    2: {
      question: "Почему температура моего устройства не достигает заданного лимита и происходит недогрев?",
      response: "Значение PID следует увеличить.",
    },
    3: {
      question: "Каким должно быть значение PID?",
      response: "Значение PID по умолчанию равно 15. Оно должно быть скорректировано в зависимости от температуры нагревательного элемента. Уменьшите его при перегреве и увеличьте при недогреве.",
    },
    4: {
      question: "Какими должны быть температура нагревательного элемента, время нагрева и охлаждения?",
      response: "Эти значения должны соответствовать мощности вашего устройства и типу профиля. Оптимальные настройки можно найти, тестируя различные значения и наблюдая за результатами.",
    },
  },
};

var jackData = [
  { l: 15, ul: 25 },
  { l: 10, ul: 30 },
  { l: 20, ul: 35 },
  { l: 18, ul: 32 },
  { id: 4, l: 12, ul: 28 },
  { id: 5, l: 12, ul: 28 },
];

const RQtype = {
  HAND_SHAKE: 1,
  SET_PROG: 2,
  SET_LANG: 3,
  RESET_COUNT: 4,
  UPD_PROG: 5,
  UPD_JACK: 6,
  UPD_KSWCH: 7,
  CHANGE_PASS: 8,
  RESET_FACTORY: 9,
  CONFIG: 10,
  NEWS: 11,
  AUTH: 12,
  WRNG_ADMIN_PASS: 13,
  RQ_DONE: 14,
  LOGINRPT: 15,
  DUTRPT: 16,
  SETTIME: 17,
  CNF_DVMODE: 32,
  CNF_JKMODE: 33,
  CNF_EMGNCYMODE: 34,
  CNF_JSENSOR: 36,
  START_JACK: 20,
  STOP_JACK: 21,
  RESET_COUNT_ALL: 30,
  SAVE_DEFS: 31,
  OTA_MODE: 35,
  newSTR: 60,
};


function appendQuestions(language) {
  const supportContent = document.getElementById("supportContent");

  supportContent.innerHTML = "";

  if (!questions_responses[language]) {
    console.error("Language not found in the data.");
    return;
  }

  const questions = questions_responses[language];
  for (const key in questions) {
    if (questions.hasOwnProperty(key)) {
      const question = questions[key].question;
      const response = questions[key].response;

      const card = document.createElement("div");
      card.className = `dashCard ${language === "FA" ? "rtl" : "ltr"}`;
      card.style.flexDirection = "column";
      card.setAttribute("onclick", `Message("${response}")`);

      const flexRow = document.createElement("div");
      flexRow.className = "flex-row";
      flexRow.style.justifyContent = "flex-start";

      const questionMark = document.createElement("span");
      questionMark.textContent = "❓"; // Unicode for red question mark
      questionMark.style.color = "red"; // Make it red
      questionMark.style.fontSize = "3vh"; // Adjust size as needed
      questionMark.style.width = "3vh";

      const splitter = document.createElement("div");
      splitter.className = "splitter";

      const questionTitle = document.createElement("p");
      questionTitle.className = "questionTitle";
      questionTitle.textContent = question;

      splitter.appendChild(questionTitle);
      flexRow.appendChild(questionMark);
      flexRow.appendChild(splitter);
      card.appendChild(flexRow);

      supportContent.appendChild(card);
    }
  }
}
function setLanguage(selectedLanguage) {
  language = selectedLanguage;
  document.getElementById("languageSelector").value = language;
  applyLanguage(language);
  appendQuestions(language);
  appendJackCard(jackData, jackLabels);
}
setLanguage(language);
function applyLanguage(lang) {
  if (!mapper[lang]) {
    console.error(`Language ${lang} not found in mapper.`);
    return;
  }
  const translations = mapper[lang];
  for (const key in translations) {
    if (translations.hasOwnProperty(key)) {
      const element = document.getElementById(key);

      if (element) {
        if (element.tagName === "INPUT" || element.tagName === "TEXTAREA") {
          element.placeholder = translations[key];
        } else if (element.tagName === "IMG") {
          element.alt = translations[key];
        } else {
          element.innerText = translations[key];
        }
      }
    }
  }
  const rtlElements = document.querySelectorAll(".rtl");
  if (lang !== "FA") {
    for (const element of rtlElements) {
      element.style.direction = "ltr";
    }
  } else {
    for (const element of rtlElements) {
      element.style.direction = "rtl";
    }
  }
}

function notify(msg, type) {
  const toast = document.createElement("div");
  toast.className = `toast ${type}`;
  toast.textContent = msg;

  document.body.appendChild(toast);
  requestAnimationFrame(() => {
    toast.classList.add("show");
  });
  setTimeout(() => {
    toast.classList.remove("show");
    toast.addEventListener("transitionend", () => {
      toast.remove();
    });
  }, 2000);
  if (type == "error") {
    window.navigator.vibrate([80, 50, 80, 80, 50, 80, 50, 100, 100]);
  } else {
    window.navigator.vibrate([80, 50, 80]);
  }
}

function onConfirm(msg, toSendObj) {
  const modal = document.getElementById("confirmModal");
  const confirmText = document.getElementById("confirmText");
  const confirmButton = document.getElementById("confirmButton");

  confirmText.innerHTML = msg;
  modal.style.display = "flex";

  confirmButton.onclick = () => {
    modal.style.display = "none";
    connection.send(JSON.stringify(toSendObj));
  };
}
let currentPage = "main";
function showPage(id) {
  let titles = {
    main: [
      "W",
      "E",
      "L",
      "D",
      "I",
      "N",
      "G",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
      "P",
      "L",
      "C",
    ],
    time: [
      "T",
      "i",
      "m",
      "e",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
      "S",
      "e",
      "t",
      "t",
      "i",
      "n",
      "g",
      "s",
    ],
    generalSettings: [
      "G",
      "e",
      "n",
      "e",
      "r",
      "a",
      "l",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
      "S",
      "e",
      "t",
      "t",
      "i",
      "n",
      "g",
      "s",
    ],
    programSettings: [
      "P",
      "r",
      "o",
      "g",
      "r",
      "a",
      "m",
      "s",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
    ],
    motionSettings: [
      "M",
      "o",
      "t",
      "i",
      "o",
      "n",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
      "S",
      "e",
      "t",
      "t",
      "i",
      "n",
      "g",
      "s",
    ],
    adminSettings: [
      "A",
      "d",
      "m",
      "i",
      "n",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
      "S",
      "e",
      "t",
      "t",
      "i",
      "n",
      "g",
      "s",
    ],
    OTA: [
      "U",
      "p",
      "d",
      "a",
      "t",
      "e",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
      "P",
      "r",
      "o",
      "g",
      "r",
      "a",
      "m",
    ],

    support: [
      "C",
      "o",
      "n",
      "t",
      "a",
      "c",
      "t",
      '<span style="--i:4; color: #ff0000; margin-left: 8px;">&#8203;</span>',
      "S",
      "u",
      "p",
      "p",
      "o",
      "r",
      "t",
    ],
  };

  document.querySelectorAll(".page").forEach(function (page) {
    page.style.display = "none";
  });

  document.querySelector("#" + id).style.display = "flex";
  document.querySelector(".menu").style.display = "none";
  document.getElementById("mainTitle").innerHTML = "";
  titles[id].forEach(function (letter, index) {
    document.getElementById("mainTitle").innerHTML += `<span style="--i:${
      index + 1
    }">${letter}</span>`;
  });
  currentPage = id;
  if (currentPage === "support" || currentPage === "main") {
    document.getElementById("languageSelector").style.display = "flex";
    document.getElementById("Helpper").style.display = "none";
  } else {
    document.getElementById("languageSelector").style.display = "none";
    document.getElementById("Helpper").style.display = "flex";
  }
  scrollToTop();
}
function scrollToTop() {
  document
    .getElementById("ModeCard")
    .scrollIntoView({ behavior: "smooth", block: "start", inline: "start" });
  document
    .getElementById("adminCard")
    .scrollIntoView({ behavior: "smooth", block: "start", inline: "start" });
  document
    .getElementById("supportContent")
    .scrollIntoView({ behavior: "smooth", block: "start", inline: "start" });
  document
    .getElementById("programSettingsContent")
    .scrollIntoView({ behavior: "smooth", block: "start", inline: "start" });
  document
    .getElementById("motionSettingsContent")
    .scrollIntoView({ behavior: "smooth", block: "start", inline: "start" });
  document
    .getElementById("generalContent")
    .scrollIntoView({ behavior: "smooth", block: "start", inline: "start" });

  window.scrollTo({
    top: 0,
    behavior: "smooth",
  });
}
function openCurretHelpper() {
  document.getElementById("HelperImage").src =
    currentPage + "-" + language + ".png";
  document.getElementById("HelperButton").style.display = "block";
  openModal("HelperModal");
}

var pageToOpen = "none";
var currentModal = "none";

function openModal(modalId, _pageToOpen = null) {
  document.getElementById(modalId).style.display = "flex";
  currentModal = modalId;
  if (_pageToOpen) pageToOpen = _pageToOpen;
  jackState = JStates.init;
  refrshJackrelatedUI();
}

function closeModal(modalId) {
  document.getElementById(modalId).style.display = "none";
  if (currentModal == "jackModal")
    connection.send(
      JSON.stringify({
        RQ: RQtype.STOP_JACK,
      })
    );
}

function onUploader() {
  connection.send(
    JSON.stringify({
      RQ: RQtype.OTA_MODE,
    })
  );
  connection.send(
    JSON.stringify({
      RQ: RQtype.OTA_MODE,
    })
  );
  setTimeout(() => {
    location.href = "intent://haffler.com#Intent;scheme=http;end";
  }, 1000);
}

var modals = document.querySelectorAll(".modal");
var closeButtons = document.querySelectorAll(".close-btn");
closeButtons.forEach(function (closeBtn, index) {
  closeBtn.addEventListener("click", function () {
    modals[index].style.display = "none";
  });
});
modals.forEach(function (modal) {
  modal.addEventListener("click", function (event) {
    if (event.target === modal) {
      if (jackState == JStates.loading || jackState == JStates.unloading) {
        notify(mapper2[language].notifyTimerError, "error");
        return;
      }
      modal.style.display = "none";
    }
  });
});
function hideAllModal() {
  const modals = document.querySelectorAll(".modal");
  modals.forEach((modal) => {
    modal.classList.add("hide");
  });
}
function updateTimerTime(value) {
  currentTimer.time = value;
  refreshTimer(currentTimer);
}
function updateCalibTemp(value) {
  document.getElementById("CalibValueTitle").value = value;
  if (isNaN(value) || !value) value = 0;
  if (value > 20) {
    value = 20;
    document.getElementById("CalibValueTitle").value = value;
  }
  if (value < -20) {
    value = -20;
    document.getElementById("CalibValueTitle").value = value;
  }
  document.getElementById("CalibSumTitle").innerHTML =
    parseInt(value) + parseInt(current_temp) - parseInt(temp_setPoint) + "°";
}

var passwordInput = "";
function submitPassword() {
  passwordInput = "1" + document.getElementById("passwordInput").value;
  connection.send(
    JSON.stringify({
      RQ: RQtype.AUTH,
      psw: passwordInput,
    })
  );
}

function setCurrentProgram(program, label) {
  currentProgram = { ...program, label };

  const heaterInput = document.getElementById("heaterInput2");
  const coolerInput = document.getElementById("coolerInput2");
  const tempInput = document.getElementById("tempInput2");
  const ProgramTitle = document.getElementById("programTitle");

  ProgramTitle.textContent = label;
  heaterInput.value = program.h;
  coolerInput.value = program.l;
  tempInput.value = program.t;
}

function appendProgramCard(programData, _selectedID = 1) {
  const programSettingsContent = document.getElementById(
    "programSettingsContent"
  );
  const programSettingsContent2 = document.getElementById(
    "programSettingsContent2"
  );

  programSettingsContent.innerHTML = "";
  programSettingsContent2.innerHTML = "";

  programData.forEach((data, index) => {
    const card = document.createElement("div");
    var isSelected = _selectedID == index + 1;
    card.className = `dashCard slim ${isSelected ? "selected" : ""}`;
    card.onclick = () => {
      setCurrentProgram(data, "Program-" + (index + 1));
      openModal("programModal");
    };

    const flexRow = document.createElement("div");
    flexRow.className = "flex-row";

    const parameterSection = document.createElement("div");
    parameterSection.className = "parameter-section";

    const parameterText = document.createElement("p");
    parameterText.className = "counter";
    parameterText.textContent = `P-${index + 1}`;
    parameterSection.appendChild(parameterText);
    flexRow.appendChild(parameterSection);

    const heater = createSplitter(
      "heater",
      data.h,
      "M96 112c0-26.5 21.5-48 48-48s48 21.5 48 48V276.5c0 17.3 7.1 31.9 15.3 42.5C217.8 332.6 224 349.5 224 368c0 44.2-35.8 80-80 80s-80-35.8-80-80c0-18.5 6.2-35.4 16.7-48.9C88.9 308.4 96 293.8 96 276.5V112zM144 0C82.1 0 32 50.1 32 112V276.4c0 .1-.1 .3-.2 .6c-.2 .6-.8 1.6-1.7 2.8C11.2 304.2 0 334.8 0 368c0 79.5 64.5 144 144 144s144-64.5 144-144c0-33.2-11.3-63.8-30.1-88.1c-.9-1.2-1.5-2.2-1.7-2.8c-.1-.3-.2-.5-.2-.6V112C256 50.1 205.9 0 144 0zm0 416c26.5 0 48-21.5 48-48c0-20.9-13.4-38.7-32-45.3V112c0-8.8-7.2-16-16-16s-16 7.2-16 16V322.7c-18.6 6.6-32 24.4-32 45.3c0 26.5 21.5 48 48 48zM448 160h32c12.9 0 24.6-7.8 29.6-19.8s2.2-25.7-6.9-34.9l-64-64c-12.5-12.5-32.8-12.5-45.3 0l-64 64c-9.2 9.2-11.9 22.9-6.9 34.9s16.6 19.8 29.6 19.8h32V448c0 17.7 14.3 32 32 32s32-14.3 32-32V160z"
    );
    flexRow.appendChild(heater);

    const cooler = createSplitter(
      "cooler",
      data.l,
      "M96 112c0-26.5 21.5-48 48-48s48 21.5 48 48V276.5c0 17.3 7.1 31.9 15.3 42.5C217.8 332.6 224 349.5 224 368c0 44.2-35.8 80-80 80s-80-35.8-80-80c0-18.5 6.2-35.4 16.7-48.9C88.9 308.4 96 293.8 96 276.5V112zM144 0C82.1 0 32 50.1 32 112V276.4c0 .1-.1 .3-.2 .6c-.2 .6-.8 1.6-1.7 2.8C11.2 304.2 0 334.8 0 368c0 79.5 64.5 144 144 144s144-64.5 144-144c0-33.2-11.3-63.8-30.1-88.1c-.9-1.2-1.5-2.2-1.7-2.8c-.1-.3-.2-.5-.2-.6V112C256 50.1 205.9 0 144 0zm0 416c26.5 0 48-21.5 48-48c0-20.9-13.4-38.7-32-45.3V272c0-8.8-7.2-16-16-16s-16 7.2-16 16v50.7c-18.6 6.6-32 24.4-32 45.3c0 26.5 21.5 48 48 48zm336-64H448V64c0-17.7-14.3-32-32-32s-32 14.3-32 32V352H352c-12.9 0-24.6 7.8-29.6 19.8s-2.2 25.7 6.9 34.9l64 64c6 6 14.1 9.4 22.6 9.4s16.6-3.4 22.6-9.4l64-64c9.2-9.2 11.9-22.9 6.9-34.9s-16.6-19.8-29.6-19.8z"
    );
    flexRow.appendChild(cooler);

    const temp = createSplitter(
      "temp",
      `${data.t}°`,
      "M160 64c-26.5 0-48 21.5-48 48V276.5c0 17.3-7.1 31.9-15.3 42.5C86.2 332.6 80 349.5 80 368c0 44.2 35.8 80 80 80s80-35.8 80-80c0-18.5-6.2-35.4-16.7-48.9c-8.2-10.6-15.3-25.2-15.3-42.5V112c0-26.5-21.5-48-48-48zM48 112C48 50.2 98.1 0 160 0s112 50.1 112 112V276.5c0 .1 .1 .3 .2 .6c.2 .6 .8 1.6 1.7 2.8c18.9 24.4 30.1 55 30.1 88.1c0 79.5-64.5 144-144 144S16 447.5 16 368c0-33.2 11.2-63.8 30.1-88.1c.9-1.2 1.5-2.2 1.7-2.8c.1-.3 .2-.5 .2-.6V112zM208 368c0 26.5-21.5 48-48 48s-48-21.5-48-48c0-20.9 13.4-38.7 32-45.3V200c0-8.8 7.2-16 16-16s16 7.2 16 16V322.7c18.6 6.6 32 24.4 32 45.3z"
    );
    flexRow.appendChild(temp);

    card.appendChild(flexRow);

    programSettingsContent.appendChild(card);
    const cardClone = card.cloneNode(true);
    var isSelected = _selectedID == index + 1;
    cardClone.className = `dashCard slim ${isSelected ? "selected" : ""}`;
    cardClone.onclick = () => {
      closeModal("programsModal");
      onChooseProgram(index + 1);
    };
    programSettingsContent2.appendChild(cardClone);
  });
}

function createSplitter(type, value, svgPath) {
  const splitter = document.createElement("div");
  splitter.className = `splitter ${type}`;

  const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("viewBox", "0 0 512 512");

  const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
  path.setAttribute("d", svgPath);

  svg.appendChild(path);
  splitter.appendChild(svg);

  const counter = document.createElement("p");
  counter.className = `counter ${type}`;
  counter.textContent = value;
  splitter.appendChild(counter);

  return splitter;
}

var programData = [
  { l: 15, h: 25, t: 20 },
  { l: 10, h: 30, t: 25 },
  { l: 20, h: 35, t: 28 },
  { l: 18, h: 32, t: 26 },
  { l: 12, h: 28, t: 22 },
];
let currentProgram = { id: 1 };
let selectedProgram = {};
let currentTimer = {
  isEnable: false,
  time: 22,
};
function refreshselectedProgram(selectedProgram) {
  document.getElementById("current_P").innerHTML = "P" + selectedProgram.id;
  document.getElementById("current_T").innerHTML = selectedProgram.t + "°";
  document.getElementById("current_L").innerHTML = selectedProgram.l;
  document.getElementById("current_H").innerHTML = selectedProgram.h;
}
function refreshTimer(timer) {
  var TimerToggle = document.getElementById("TimerToggle");
  if (timer.isEnable) {
    TimerToggle.classList.add("active");
  } else {
    TimerToggle.classList.remove("active");
  }
  document.getElementById("TimerValueTitle").value = `${timer.time}`;
  document.getElementById("TimerInput").value = timer.time;
}

function toggleTimer() {
  currentTimer.isEnable = !currentTimer.isEnable;
  refreshTimer(currentTimer);
}
function toggleTermo() {
  var termoToggle = document.getElementById("TermoToggle");
  var isEnable = termoToggle.classList.contains("active");
  if (isEnable) {
    termoToggle.classList.remove("active");
  } else {
    termoToggle.classList.add("active");
  }
  onUpdateJKMode();
}
function toggleSpeaker() {
  var speakerToggle = document.getElementById("spkToggle");
  var isEnable = speakerToggle.classList.contains("active");
  if (isEnable) {
    speakerToggle.classList.remove("active");
  } else {
    speakerToggle.classList.add("active");
  }
}

function toggleAutoStart() {
  var autoStartToggle = document.getElementById("autoStartToggle");
  var isEnable = autoStartToggle.classList.contains("active");
  if (isEnable) {
    autoStartToggle.classList.remove("active");
  } else {
    autoStartToggle.classList.add("active");
  }
}
function toggleEndMode() {
  var endModeToggle = document.getElementById("endModeToggle");
  var isEnable = endModeToggle.classList.contains("active");
  if (isEnable) {
    endModeToggle.classList.remove("active");
  } else {
    endModeToggle.classList.add("active");
  }
}
function toggleEmgncy() {
  var EmgncyToggle = document.getElementById("EmgncyToggle");
  var isEnable = EmgncyToggle.classList.contains("active");
  if (isEnable) {
    EmgncyToggle.classList.remove("active");
  } else {
    EmgncyToggle.classList.add("active");
  }
}
function toggleTLimitter() {
  var tLimitToggle = document.getElementById("tLimitToggle");
  var isEnable = tLimitToggle.classList.contains("active");
  if (isEnable) {
    tLimitToggle.classList.remove("active");
  } else {
    tLimitToggle.classList.add("active");
  }
}
function togglePedal() {
  var pedalToggle = document.getElementById("pedalToggle");
  var isEnable = pedalToggle.classList.contains("active");
  if (isEnable) {
    pedalToggle.classList.remove("active");
  } else {
    pedalToggle.classList.add("active");
  }
  
}
appendJackCard(jackData, jackLabels);
appendProgramCard(programData);

function appendJackCard(jackData, jackLabels) {
  const motionSettingsContent = document.getElementById(
    "motionSettingsContent"
  );
  motionSettingsContent.innerHTML = "";
  jackData.forEach((data, index) => {
    const card = document.createElement("div");
    card.className = "dashCard slim";
    card.onclick = () => {
      setCurrentJack(data, jackLabels[language][index]);
      if (index == 4 || index == 5) {
        document.getElementById("uloadJack").style.display = "none";
      } else {
        document.getElementById("uloadJack").style.display = "flex";
      }
      openModal("jackModal");
    };

    const flexRow = document.createElement("div");
    flexRow.className = "flex-row";

    const label = document.createElement("div");
    label.className = `splitter`;
    label.style = " justify-content: flex-start;";
    const counter = document.createElement("p");
    counter.className = "counter";
    counter.textContent = jackLabels[language][index];
    label.appendChild(counter);
    flexRow.appendChild(label);

    const cooler = createSplitter(
      "cooler",
      index == 4 || index == 5 ? "--" : data.ul,
      "M441.9 250.1l-19.8-19.8c-4.7-4.7-12.3-4.7-17 0L250 385.4V44c0-6.6-5.4-12-12-12h-28c-6.6 0-12 5.4-12 12v341.4L42.9 230.3c-4.7-4.7-12.3-4.7-17 0L6.1 250.1c-4.7 4.7-4.7 12.3 0 17l209.4 209.4c4.7 4.7 12.3 4.7 17 0l209.4-209.4c4.7-4.7 4.7-12.3 0-17z"
    );
    flexRow.appendChild(cooler);

    const heater = createSplitter(
      "heater",
      data.l,
      "M214.6 41.4c-12.5-12.5-32.8-12.5-45.3 0l-160 160c-12.5 12.5-12.5 32.8 0 45.3s32.8 12.5 45.3 0L160 141.2V448c0 17.7 14.3 32 32 32s32-14.3 32-32V141.2L329.4 246.6c12.5 12.5 32.8 12.5 45.3 0s12.5-32.8 0-45.3l-160-160z"
    );
    flexRow.appendChild(heater);

    card.appendChild(flexRow);

    motionSettingsContent.appendChild(card);
  });
}

function setCurrentJack(jack, label) {
  currentJack = { ...jack, label };

  const heaterInput = document.getElementById("heaterInput");
  const coolerInput = document.getElementById("coolerInput");
  const JackTitle = document.getElementById("JackTitle");

  JackTitle.textContent = label;
  heaterInput.value = jack.l;
  coolerInput.value = jack.ul;

  const heaterCounter = document.querySelector(
    ".modal-content .splitter.heater .counter"
  );
  const coolerCounter = document.querySelector(
    ".modal-content .splitter.cooler .counter"
  );
  if (heaterCounter) heaterCounter.textContent = mapper[language]["incoming"];
  if (coolerCounter) coolerCounter.textContent = mapper[language]["outgoing"];
}

function incrementInput(inputId, step) {
  console.log("sss", currentJack);
  if (currentJack.id == 4 || currentJack.id == 5) {
    step = 1000;
  }
  const input = document.getElementById(inputId);
  const currentValue = parseInt(input.value);
  if (!isNaN(currentValue)) {
    const newValue = currentValue + step;
    input.value = isNaN(parseInt(currentValue)) ? newValue : newValue;
  }
}

function decrementInput(inputId, step) {
  console.log("sss", currentJack);

  if (currentJack.id == 4 || currentJack.id == 5) {
    step = 1000;
  }
  const input = document.getElementById(inputId);
  const currentValue = parseInt(input.value);
  if (!isNaN(currentValue)) {
    const newValue = currentValue - step;
    input.value = newValue > parseInt(input.min) ? newValue : input.min;
  }
}

let timers = {};
let stopClicks = {};

function refrshJackrelatedUI() {
  switch (jackState) {
    case JStates.init:
    case JStates.unloaded:
      document.getElementById("heaterInputPlayIcon").classList.remove("hide");
      document.getElementById("heaterInputStopIcon").classList.add("hide");
      document.getElementById("coolerInputPlayIcon").classList.add("hide");
      document.getElementById("coolerInputStopIcon").classList.add("hide");
      break;
    case JStates.loading:
      document.getElementById("heaterInputPlayIcon").classList.add("hide");
      document.getElementById("heaterInputStopIcon").classList.remove("hide");
      document.getElementById("coolerInputPlayIcon").classList.add("hide");
      document.getElementById("coolerInputStopIcon").classList.add("hide");
      break;
    case JStates.loaded:
      document.getElementById("heaterInputPlayIcon").classList.add("hide");
      document.getElementById("heaterInputStopIcon").classList.add("hide");
      document.getElementById("coolerInputPlayIcon").classList.remove("hide");
      document.getElementById("coolerInputStopIcon").classList.add("hide");
      break;
    case JStates.unloading:
      document.getElementById("heaterInputPlayIcon").classList.add("hide");
      document.getElementById("heaterInputStopIcon").classList.add("hide");
      document.getElementById("coolerInputPlayIcon").classList.add("hide");
      document.getElementById("coolerInputStopIcon").classList.remove("hide");
      break;
  }
}
function onJackInputChanged() {
  jackState = JStates.init;
  refrshJackrelatedUI();
}
function playTimer(inputId, _jackState) {
  if (timers[inputId]) {
    clearInterval(timers[inputId]);
  }

  const input = document.getElementById(inputId);
  input.value = "0";

  timers[inputId] = setInterval(
    () => {
      incrementInput(
        inputId,
        currentJack.id == 4 || currentJack.id == 5 ? 1000 : 100
      );
    },
    currentJack.id == 4 || currentJack.id == 5 ? 1000 : 100
  );

  stopClicks[inputId] = 0;

  jackState = _jackState;
  refrshJackrelatedUI();

  connection.send(
    JSON.stringify({
      RQ: inputId.includes("cooler") ? RQtype.STOP_JACK : RQtype.START_JACK,
      ID: currentJack.id,
    })
  );
}

function stopTimer(inputId, _jackState) {
  const input = document.getElementById(inputId);
  stopClicks[inputId] = (stopClicks[inputId] || 0) + 1;

  if (stopClicks[inputId] === 1) {
    if (timers[inputId]) {
      clearInterval(timers[inputId]);
      delete timers[inputId];
    }
  } else if (stopClicks[inputId] === 2) {
    input.value = "100";
    stopClicks[inputId] = 0;
  }
  jackState = _jackState;
  refrshJackrelatedUI();
  if (currentJack.id == 4 || currentJack.id == 5) {
    jackState = JStates.init;
    refrshJackrelatedUI();
    connection.send(
      JSON.stringify({
        RQ: RQtype.STOP_JACK,
      })
    );
  }
}

function onChooseProgram(index) {
  console.log("Choose Program", index);
  connection.send(
    JSON.stringify({
      RQ: RQtype.SET_PROG,
      ID: index,
    })
  );
}

function submitJack() {
  if (jackState === JStates.loading || jackState === JStates.unloading) {
    notify(mapper2[language].notifyTimerError, "error");
    return;
  }
  console.log("Submit Jack modal");
  currentJack.l = document.getElementById("heaterInput").value;
  currentJack.ul = document.getElementById("coolerInput").value;
  if (
    currentJack.l < 100 ||
    currentJack.l > 6000 ||
    (currentJack.id < 4 && currentJack.ul < 100) ||
    currentJack.ul > 6000
  ) {
    notify(mapper2[language].limitationError, "error");
    return;
  }
  if (currentJack.id == 4 || currentJack.id == 5) {
    currentJack.l = currentJack.l / 1000;
    currentJack.ul = currentJack.ul / 1000;
  }
  closeModal("jackModal");
  connection.send(
    JSON.stringify({
      RQ: RQtype.UPD_JACK,
      ID: currentJack.id,
      l: currentJack.l,
      ul: currentJack.ul,
    })
  );
}

function submitProgram() {
  const heaterTime = document.getElementById("heaterInput2").value;
  const coolerTime = document.getElementById("coolerInput2").value;
  const temperature = document.getElementById("tempInput2").value;
  closeModal("programModal");
  const payload = {
    RQ: RQtype.UPD_PROG,
    ID: currentProgram.id,
    h: parseInt(heaterTime, 10),
    l: parseInt(coolerTime, 10),
    t: parseInt(temperature, 10),
  };
  connection.send(JSON.stringify(payload));
}

function onSaveStates() {
  const CalibValue = document.getElementById("CalibValueTitle").value;
  const pidValue = document.getElementById("PID").value;
  if (isNaN(CalibValue) || CalibValue < -20 || CalibValue > 20) {
    notify(mapper2[language].limitationError, "error");
    return;
  }
  if (isNaN(pidValue) || pidValue < 0 || pidValue > 30) {
    notify(mapper2[language].limitationError, "error");
    return;
  }

  const speakerActive = document
    .getElementById("spkToggle")
    .classList.contains("active")
    ? true
    : false;
  const autoStartToggleActive = document
    .getElementById("autoStartToggle")
    .classList.contains("active")
    ? true
    : false;
  const endModeToggleActive = document
    .getElementById("endModeToggle")
    .classList.contains("active")
    ? true
    : false;
  const EmgncyToggleActive = document
    .getElementById("EmgncyToggle")
    .classList.contains("active")
    ? true
    : false;
  const tLimitToggleActive = document
    .getElementById("tLimitToggle")
    .classList.contains("active")
    ? true
    : false;

    const pedalCount = document.getElementById("pedalToggle").classList.contains("active") ? 4 : 3;

  const config = {
    PID: parseInt(pidValue, 10),
    Tcal: parseInt(CalibValue, 10),
    spk: speakerActive,
    aut: autoStartToggleActive,
    end: endModeToggleActive,
    emg: EmgncyToggleActive,
    tLmt: tLimitToggleActive,
    pedal: pedalCount
  };
  connection.send(
    JSON.stringify({
      RQ: RQtype.CONFIG,
      ...config,
    })
  );
}
function onUpdateDVMode() {
  const deviceMode = document.getElementById("deviceMode").value;
  connection.send(
    JSON.stringify({
      RQ: RQtype.CNF_DVMODE,
      dvMode: parseInt(deviceMode, 10),
      psw: passwordInput,
    })
  );
}
function onUpdateJKMode() {
  const isJ = document
    .getElementById("TermoToggle")
    .classList.contains("active")
    ? true
    : false;
  connection.send(
    JSON.stringify({
      RQ: RQtype.CNF_JKMODE,
      isJ: !isJ,
      psw: passwordInput,
    })
  );
}
function onUpdateEmgncy() {
  const isActive = document
    .getElementById("EmgncyToggle")
    .classList.contains("active")
    ? true
    : false;
  connection.send(
    JSON.stringify({
      RQ: RQtype.CNF_EMGNCYMODE,
      emg: !isActive,
      psw: passwordInput,
    })
  );
}
function onUpdateJsensor() {
  const isActive = document
    .getElementById("JsensorToggle")
    .classList.contains("active")
    ? true
    : false;
  connection.send(
    JSON.stringify({
      RQ: RQtype.CNF_JSENSOR,
      psw: passwordInput,
      js: !isActive,
    })
  );
}

function onUpdateKillSwitch() {
  let timerValue = document.getElementById("TimerValueTitle").value;

  let isToggleOn = document
    .getElementById("TimerToggle")
    .classList.contains("active");

  if (timerValue > 200 || timerValue < 1) {
    notify(mapper2[language].limitationError, "error");
    return;
  }

  connection.send(
    JSON.stringify({
      RQ: RQtype.UPD_KSWCH,
      isOn: isToggleOn,
      days: parseInt(timerValue, 10),
      psw: passwordInput,
    })
  );
}
function onSaveDefaultValues() {
  connection.send(
    JSON.stringify({
      RQ: RQtype.SAVE_DEFS,
      psw: passwordInput,
    })
  );
}
function onchangePass() {
  let newPass = "1" + document.getElementById("changePassInput").value;
  let confirmPass =
    "1" + document.getElementById("changePassConfirmInput").value;
  if (newPass !== confirmPass) {
    alert("رمز عبور و تایید رمز با هم مطابقت ندارند.");
    return;
  }
  if (
    newPass.length !== 7 ||
    confirmPass.length !== 7 ||
    isNaN(newPass) ||
    isNaN(confirmPass)
  ) {
    alert("رمز عبور باید یک عدد ۶ رقمی باشد.");
    return;
  }
  connection.send(
    JSON.stringify({
      RQ: RQtype.CHANGE_PASS,
      psw: passwordInput,
      newPass: newPass,
    })
  );
}
function onRessetCountersAll() {
  onConfirm(mapper2[language].confirmResetAllCounters, {
    psw: passwordInput,
    RQ: RQtype.RESET_COUNT_ALL,
  });
}

function onRessetFactory() {
  onConfirm(mapper2[language].confirmResetFactory, {
    RQ: RQtype.RESET_FACTORY,
  });
}
function onResetCounter() {
  onConfirm(mapper2[language].confirmResetCounter, {
    RQ: RQtype.RESET_COUNT,
  });
}

function Message(_msg) {
  document.getElementById("messageText").innerHTML = _msg;
  document.getElementById("messageText").style.direction =
    language == "FA" ? "rtl" : "ltr";
  openModal("MessageModal");
}

function togglePasswordVisibilty() {
  var changePassInput = document.getElementById("changePassInput");
  var changePassConfirmInput = document.getElementById(
    "changePassConfirmInput"
  );
  if (changePassInput.type === "password") {
    changePassInput.type = "text";
    changePassConfirmInput.type = "text";
  } else {
    changePassInput.type = "password";
    changePassConfirmInput.type = "password";
  }
}

function onSaveLanguage() {
  var language = document.getElementById("languageSelector").value;
  var toSend = language == "FA" ? 1 : language == "EN" ? 2 : language == "TU" ? 3 : 4;
  setLanguage(language);
  connection.send(
    JSON.stringify({
      RQ: RQtype.SET_LANG,
      lan: toSend,
    })
  );
}

var connection;
if (
  location.hostname == "captive.apple.com" ||
  location.hostname.includes("apple")
)
  connection = new WebSocket("ws://172.217.28.1:81/", ["arduino"]);
else
  connection = new WebSocket("ws://" + location.hostname + ":81/", ["arduino"]);

connection.onopen = function () {};

connection.onerror = function (error) {
  ONLINE_Flag = false;
  console.log("WDCNTE");
  if (!InAPP_Flag) {
    setTimeout(function () {
      location.reload();
    }, 500);
  }
};

let isAdjusted = false;
var isHandshaked = false;
connection.onmessage = function (evt) {
  var jsonObject = JSON.parse(evt.data);
  console.log("jsonObject", jsonObject);
  var Did = jsonObject.DID;

  if (Did == 555) {
    var rgst = jsonObject.RQ;
    switch (rgst) {
      case RQtype.HAND_SHAKE:
        if (jsonObject.OTA) showPage("OTA");
        var dvMode = jsonObject.dvMode;
        var PID = jsonObject.PID;
        var isJ = jsonObject.isJ;
        var isSpk = jsonObject.spk;
        var isaut = jsonObject.aut;
        var isend = jsonObject.end;
        var isemg = jsonObject.emg;
        var istLmt = jsonObject.tLmt;
        var Tcal = jsonObject.Tcal;
        var WM = jsonObject.WM;
        var PM = jsonObject.PM;
        var TM = jsonObject.TM + "°";
        current_temp = jsonObject.TM;
        var Ws = jsonObject.Ws;
        var Ps = jsonObject.Ps;
        var isTimer = jsonObject.KW;
        var TimerTime = jsonObject.KWT;
        var lang = jsonObject.lan;
        var pedalCount = jsonObject.pedal;
        var jSensorEnable = jsonObject.js;

        language = lang == 1 ? "FA" : lang == 2 ? "EN" : lang == 3 ? "TU" : "RU";
        setLanguage(language);
        currentTimer = { isEnable: isTimer, time: TimerTime };

        selectedProgram = {
          id: jsonObject.PU,
          h: jsonObject.H,
          l: jsonObject.L,
          t: jsonObject.T,
        };
        programData = jsonObject.prg;

        jackData = jsonObject.jks;
        appendJackCard(jackData, jackLabels);
        appendProgramCard(programData, jsonObject.PU);
        refreshselectedProgram(selectedProgram);
        refreshTimer(currentTimer);

        document.getElementById("deviceMode").value = dvMode;
        document.getElementById("PID").value = PID;
        document.getElementById("WM").innerHTML = WM;
        document.getElementById("PM").innerHTML = PM;
        document.getElementById("TM").innerHTML = TM;
        document.getElementById("Ws").innerHTML = Ws;
        document.getElementById("Ps").innerHTML = Ps;
        document.getElementById("CalibInput").value = Tcal;
        document.getElementById("CalibValueTitle").value = Tcal;
        temp_setPoint = Tcal;
        document.getElementById("CalibSumTitle").innerHTML =
          parseInt(current_temp) + "°";

        var termoToggle = document.getElementById("TermoToggle");
        if (isJ) {
          termoToggle.classList.add("active");
        } else {
          termoToggle.classList.remove("active");
        }
        var spkToggle = document.getElementById("spkToggle");
        if (isSpk) {
          spkToggle.classList.add("active");
        } else {
          spkToggle.classList.remove("active");
        }
        var autoStartToggle = document.getElementById("autoStartToggle");
        if (isaut) {
          autoStartToggle.classList.add("active");
        } else {
          autoStartToggle.classList.remove("active");
        }
        var endToggle = document.getElementById("endModeToggle");
        if (isend) {
          endToggle.classList.add("active");
        } else {
          endToggle.classList.remove("active");
        }
        var emgncyToggle = document.getElementById("EmgncyToggle");
        if (isemg) {
          emgncyToggle.classList.add("active");
        } else {
          emgncyToggle.classList.remove("active");
        }
        var tLimitToggle = document.getElementById("tLimitToggle");
        if (istLmt) {
          tLimitToggle.classList.add("active");
        } else {
          tLimitToggle.classList.remove("active");
        }

        var jSensorToggle = document.getElementById("JsensorToggle");
        if (jSensorEnable) {
          jSensorToggle.classList.add("active");
        } else {
          jSensorToggle.classList.remove("active");
        }

        var pedalToggle = document.getElementById("pedalToggle");
        if (pedalCount == 4) {
          pedalToggle.classList.add("active");
        } else {
          pedalToggle.classList.remove("active");
        }

        document.getElementById("activ_date").innerHTML = jsonObject.ADT;
        if (!isHandshaked) {
          closeLoading();
        }
        isHandshaked = true;
        break;

      case RQtype.AUTH:
        closeModal("passwordModal");
        showPage(pageToOpen);
        document.getElementById("passwordError").style.display = "none";
        break;
      case RQtype.WRNG_ADMIN_PASS:
        openModal("passwordModal");
        document.getElementById("passwordError").style.display = "block";
        notify(mapper2[language].notifyWrongPassword, "error");

        break;
      case RQtype.RQ_DONE:
        notify(mapper2[language].notifyRequestSuccess, "success");
        break;
      case RQtype.SAVE_DEFS:
        notify(mapper2[language].notifySaveDefaults, "success");
        break;
      case RQtype.NEWS:
        {
          var TM = jsonObject.T + "°";
          current_temp = jsonObject.T;
          document.getElementById("TM").innerHTML = TM;
          document.getElementById("CalibSumTitle").innerHTML =
            parseInt(current_temp) + "°";
        }
        break;
    }
  }
};
connection.onclose = function () {
  ONLINE_Flag = false;
  console.log("WDCNTC");
  if (!InAPP_Flag) {
    setTimeout(function () {
      location.reload();
    }, 200);
  }
};

$('#upload_os_form').submit(function (e) {
  e.preventDefault();
  var form = $(this)[0];
  var data = new FormData(form);
  $.ajax({
    url: '/update',  // Ensure this is the correct endpoint for OS update
    type: 'POST',
    data: data,
    contentType: false,
    processData: false,
    xhr: function () {
      var xhr = new window.XMLHttpRequest();
      xhr.upload.addEventListener('progress', function (evt) {
        if (evt.lengthComputable) {
          var per = evt.loaded / evt.total;
          $('#prg_os').html('progress: ' + Math.round(per * 100) + '%');
        }
      }, false);
      return xhr;
    },
    success: function (d, s) {
      notify(mapper2[language].updateSucceed, "success");

    },
    error: function (a, b, c) {
      notify(mapper2[language].updateFailed, "error");

    }
  });
});

$('#upload_html_form').submit(function (e) {
  e.preventDefault();
  var form = $(this)[0];
  var data = new FormData(form);
  $.ajax({
    url: '/edit', // Ensure this is the correct endpoint for HTML upload
    type: 'POST',
    data: data,
    contentType: false,
    processData: false,
    xhr: function () {
      var xhr = new window.XMLHttpRequest();
      xhr.upload.addEventListener('progress', function (evt) {
        if (evt.lengthComputable) {
          var per = evt.loaded / evt.total;
          $('#prg_html').html('Updating...');
        }
      }, false);
      return xhr;
    },
    success: function (d, s) {

      $('#prg_html').html('Done!');
      console.log('Asset upload success!');
      notify(mapper2[language].updateSucceed, "success");
    },
    error: function (a, b, c) {
      $('#prg_html').html('Failed!');
      console.log('Asset upload failed'); 
      notify(mapper2[language].updateFailed, "error");
    }
  });
});
