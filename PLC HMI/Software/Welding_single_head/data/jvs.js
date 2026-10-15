
const pageTitles = {
    main: { FA: "صفحه اصلی", EN: "Dashboard", TU: "Ana Panel" },
    generalSettings: { FA: "تنظیمات عمومی", EN: "General Settings", TU: "Genel Ayarlar" },
    programSettings: { FA: "برنامه‌های جوشکاری", EN: "Welding Programs", TU: "Kaynak Programlari" },
    heatPrograms: { FA: "برنامه‌های جوشکاری", EN: "Welding Programs", TU: "Kaynak Programlari" },
    motionSettings: { FA: "زمان‌بندی جک‌ها", EN: "Jack Timings", TU: "Kriko Zamanlamalari" },
    hardwareDiagnostic: { FA: "تست سلامت و جک‌ها", EN: "Diagnostics", TU: "Diagnostik" },
    adminSettings: { FA: "تنظیمات مدیر", EN: "Admin Settings", TU: "Yonetici Ayarlari" },
    OTA: { FA: "بروزرسانی نرم‌افزار", EN: "Firmware Update", TU: "Yazilim Guncelleme" },
    otaSettings: { FA: "بروزرسانی نرم‌افزار", EN: "Firmware Update", TU: "Yazilim Guncelleme" },
    support: { FA: "پشتیبانی فنی", EN: "Support", TU: "Teknik Destek" },
    chatbot: { FA: "دستیار هوشمند", EN: "AI Assistant", TU: "Akilli Asistan" }
};
var connection, language = "FA", current_temp = 0, temp_setPoint = 0;
let currentJack = { id: 0 };
let currentProgram = { id: 1 };
let selectedProgram = { id: 1, h: 25, l: 30, t: 250 };
let currentTimer = { isEnable: false, time: 22 };
let passwordInput = "";
let pageToOpen = "none";
let currentModal = "none";
let isHandshaked = false;
let ONLINE_Flag = false;
let navHistory = ['main'];
let currentPage = "main";
let reconnectInterval = 1500;
let reconnectTimer = null;

var JStates = { init: 0, loading: 1, loaded: 2, unloading: 3, unloaded: 4 };
var jackState = JStates.init;

var programData = [
    { id: 1, l: 30, h: 25, t: 250 },
    { id: 2, l: 35, h: 28, t: 255 },
    { id: 3, l: 25, h: 22, t: 245 },
    { id: 4, l: 40, h: 30, t: 260 },
    { id: 5, l: 20, h: 20, t: 240 }
];

var jackData = [
    { id: 0, l: 1500, ul: 2500 },
    { id: 1, l: 1000, ul: 3000 },
    { id: 2, l: 2000, ul: 3500 },
    { id: 3, l: 1800, ul: 3200 },
    { id: 4, l: 12,   ul: 0 },
    { id: 5, l: 12,   ul: 0 }
];

const jackLabels = {
    FA: ["جک صفحه (میز جوش)", "جک شابلون صفر", "جک پروفیل (گیره فک‌ها)", "جک جوش (حرکت اتو)", "جک ۰.۲ (ضربه ۱)", "جک ۰.۲ (ضربه ۲)"],
    EN: ["Table Jack", "Shablon 0 Jack", "Profile Clamp Jack", "Heater Iron Jack", "0.2 Shocker 1", "0.2 Shocker 2"],
    TU: ["Masa Krikosu", "Sifir Kalip Krikosu", "Profil Sikma Krikosu", "Kaynak Utusu Krikosu", "0.2 Darbe 1", "0.2 Darbe 2"],
    RU: ["Домкрат стола", "Домкрат шаблона", "Прижим профиля", "Домкрат утюга", "0.2 Удар 1", "0.2 Удар 2"]
};

const mapper = {
    FA: {
        home: "صفحه اصلی", "gnral-sett": "تنظیمات عمومی", "heat-sett": "برنامه‌های جوشکاری",
        "hyd-sett": "زمان‌بندی جک‌ها", "timer-sett": "تنظیمات مدیر", "ota-sett": "بروزرسانی نرم‌افزار",
        "supp-page": "پشتیبانی فنی", "chatbot-menu": "دستیار هوشمند هافلر",
        active_program: "برنامه کاری فعال", iron_temp: "دمای اتو", melting_time: "زمان ذوب",
        cooling_time: "زمان پرس و خنک‌کاری", change_program: "تغییر و انتخاب برنامه (P1..P5)",
        temp_calib: "کالیبراسیون دمای اتو", pid_coeff: "ضریب هوشمند PID", speaker: "بوق / اسپیکر",
        autostart: "استارت خودکار", end_mode: "حالت جک پایانی", emergency: "سوئیچ استپ اضطراری",
        Jsensor: "سنسور موقعیت جک", pedalN: "تعداد پدال", temp_limit: "محدودیت دمای استارت",
        saveStates: "ثبت و ذخیره تنظیمات عمومی", op_mode: "حالت عملکرد دستگاه",
        termo: "نوع سنسور حرارتی", rst_count: "ریست شمارنده شیفت", save_defs: "ذخیره مقادیر پشتیبان",
        KSWCH: "تایمر معکوس کارکرد", days_label: "روز", save_timer: "ذخیره تایمر", ch_pass: "تغییر رمز عبور",
        act_date_label: "تاریخ فعال‌سازی", os_updater_label: "بروزرسانی فریمور OS:", asset_label: "بروزرسانی وب‌اپلیکیشن:",
        submit_update: "شروع آپدیت", submit_os: "شروع آپدیت", prg_os: "پیشرفت: ۰٪", prg_html: "پیشرفت: ۰٪",
        rst_factory: "بازگردانی به تنظیمات کارخانه", passwordError: "رمز عبور اشتباه است", conf_pass_butt: "ورود به بخش مدیریت",
        incoming: "زمان رفت (ms)", outgoing: "زمان برگشت (ms)", submitjack_butt: "ثبت زمان جک",
        heaterTitle: "زمان ذوب (ثانیه)", coolerTitle: "زمان خنک‌کاری (ثانیه)", tempTitle: "دمای هدف اتو (°C)",
        submitProgram_butt: "ثبت تغییرات برنامه", confirm_modal_title: "تایید عملیات",
        confirmText: "آیا از انجام این عملیات اطمینان دارید؟", confirmButton: "تایید و اجرا",
        MessageButton: "متوجه شدم", HelperButton: "بستن", changePassInput: "رمز جدید", changePassConfirmInput: "تکرار رمز جدید",
        passwordInput: "رمز عبور مدیر"
    },
    TU: {
        home: "Ana Panel", "gnral-sett": "Genel Ayarlar", "heat-sett": "Kaynak Programlari",
        "hyd-sett": "Kriko Zamanlamalari", "timer-sett": "Yonetici Ayarlari", "ota-sett": "Yazilim Guncelleme",
        "supp-page": "Teknik Destek", "chatbot-menu": "Haffler Akilli Asistan",
        active_program: "Aktif Program", iron_temp: "Utu Sicakligi", melting_time: "Eritme Suresi",
        cooling_time: "Baski ve Sogutma", change_program: "Program Sec (P1..P5)",
        temp_calib: "Sicaklik Kalibrasyonu", pid_coeff: "Akilli PID Katsayisi", speaker: "Hoparlor",
        autostart: "Otomatik Baslatma", end_mode: "Son Kriko Modu", emergency: "Acil Stop Butonu",
        Jsensor: "Kriko Sensoru", pedalN: "Pedal Sayisi", temp_limit: "Sicaklik Limit Kilidi",
        saveStates: "Genel Ayarlari Kaydet", op_mode: "Calisma Modu",
        termo: "Termokupl Tipi", rst_count: "Vardiya Sayacini Sifirla", save_defs: "Yedek Degerleri Kaydet",
        KSWCH: "Geri Sayim Zamanlayicisi", days_label: "Gun", save_timer: "Zamanlayiciyi Kaydet", ch_pass: "Sifreyi Degistir",
        act_date_label: "Aktivasyon Tarihi", os_updater_label: "OS Donanim Guncelleme:", asset_label: "Web Uygulamasi Guncelleme:",
        submit_update: "Guncelle", submit_os: "Guncelle", prg_os: "Ilerleme: %0", prg_html: "Ilerleme: %0",
        rst_factory: "Fabrika Ayarlarina Don", passwordError: "Gecersiz Sifre", conf_pass_butt: "Yonetici Girisi",
        incoming: "Acilma Suresi (ms)", outgoing: "Kapanma Suresi (ms)", submitjack_butt: "Kriko Suresini Kaydet",
        heaterTitle: "Eritme Suresi (sn)", coolerTitle: "Sogutma Suresi (sn)", tempTitle: "Hedef Sicaklik (°C)",
        submitProgram_butt: "Programi Kaydet", confirm_modal_title: "Islem Onayi",
        confirmText: "Bu islemi gerceklestirmek istediginizden emin misiniz?", confirmButton: "Onayla",
        MessageButton: "Anladim", HelperButton: "Kapat", changePassInput: "Yeni Sifre", changePassConfirmInput: "Sifre Tekrari",
        passwordInput: "Yonetici Sifresi"
    },
    EN: {
        home: "Dashboard", "gnral-sett": "General Settings", "heat-sett": "Welding Programs",
        "hyd-sett": "Jack Timings", "timer-sett": "Admin Settings", "ota-sett": "Firmware Update",
        "supp-page": "Technical Support", "chatbot-menu": "AI Assistant",
        active_program: "Active Program", iron_temp: "Iron Temp", melting_time: "Melting Time",
        cooling_time: "Cooling Time", change_program: "Select Program (P1..P5)",
        temp_calib: "Temp Calibration", pid_coeff: "PID Coefficient", speaker: "Buzzer",
        autostart: "Auto Start", end_mode: "End Jack Mode", emergency: "E-Stop Switch",
        Jsensor: "Jack Sensor", pedalN: "Pedal Count", temp_limit: "Temp Limit Interlock",
        saveStates: "Save General Settings", op_mode: "Operation Mode",
        termo: "Thermocouple Type", rst_count: "Reset Shift Counter", save_defs: "Save Backup",
        KSWCH: "Countdown Timer", days_label: "Days", save_timer: "Save Timer", ch_pass: "Change Password",
        act_date_label: "Activation Date", os_updater_label: "OS Firmware Update:", asset_label: "Web App Update:",
        submit_update: "Update", submit_os: "Update", prg_os: "Progress: 0%", prg_html: "Progress: 0%",
        rst_factory: "Factory Reset", passwordError: "Incorrect Password", conf_pass_butt: "Login",
        incoming: "Load Time (ms)", outgoing: "Unload Time (ms)", submitjack_butt: "Save Jack",
        heaterTitle: "Melting Time (s)", coolerTitle: "Cooling Time (s)", tempTitle: "Target Temp (°C)",
        submitProgram_butt: "Save Program", confirm_modal_title: "Confirmation",
        confirmText: "Are you sure you want to proceed?", confirmButton: "Confirm",
        MessageButton: "OK", HelperButton: "Close", changePassInput: "New Password", changePassConfirmInput: "Confirm Password",
        passwordInput: "Admin Password"
    }
};

const RQtype = {
    HAND_SHAKE: 1, SET_PROG: 2, SET_LANG: 3, RESET_COUNT: 4,
    UPD_PROG: 5, UPD_JACK: 6, UPD_KSWCH: 7, CHANGE_PASS: 8,
    RESET_FACTORY: 9, CONFIG: 10, NEWS: 11, AUTH: 12,
    WRNG_ADMIN_PASS: 13, RQ_DONE: 14, START_JACK: 20, STOP_JACK: 21,
    TEST_BUZZER: 25, RESET_COUNT_ALL: 30, SAVE_DEFS: 31, CNF_DVMODE: 32,
    CNF_JKMODE: 33, CNF_EMGNCYMODE: 34, OTA_MODE: 35, CNF_JSENSOR: 36,
    WIFI_SCAN: 50, WIFI_CONNECT: 51
};

// ── Splash Screen Control ───────────────────────────────────────────────────
function openLoading() {
    var splash = document.getElementById("appSplashScreen");
    if (splash) {
        splash.style.display = "flex";
        splash.style.opacity = "1";
        splash.style.pointerEvents = "all";
    }
}

function closeLoading() {
    var splash = document.getElementById("appSplashScreen");
    if (splash) {
        splash.style.opacity = "0";
        splash.style.pointerEvents = "none";
        setTimeout(() => { splash.style.display = "none"; }, 400);
    }
}
// Splash screen is handled with fast failsafe dismiss in index.html

// ── Notification Toast (Single-Instance, Anti-Flood, Zero-Overlap) ──────────
let activeToast = null;
let toastTimeout = null;
let lastToastText = "";
let lastToastTime = 0;

function notify(text, type) {
    if (!text) return;
    const now = Date.now();
    // Anti-flood: ignore identical message within 1.5 seconds
    if (text === lastToastText && (now - lastToastTime < 1500)) {
        return;
    }
    // Suppress generic "دستور با موفقیت انجام شد" if another toast was shown within the last 1.2 seconds
    if (text === "دستور با موفقیت انجام شد" && (now - lastToastTime < 1200)) {
        return;
    }
    lastToastText = text;
    lastToastTime = now;

    if (activeToast) {
        clearTimeout(toastTimeout);
        try { activeToast.remove(); } catch (e) {}
        activeToast = null;
    }

    let t = document.createElement("div");
    t.className = `toast ${type || "success"}`;
    t.textContent = text;
    document.body.appendChild(t);
    activeToast = t;

    requestAnimationFrame(() => {
        if (t) t.classList.add("show");
    });

    toastTimeout = setTimeout(() => {
        if (t) {
            t.classList.remove("show");
            setTimeout(() => {
                try { if (t.parentNode) t.parentNode.removeChild(t); } catch (e) {}
                if (activeToast === t) activeToast = null;
            }, 350);
        }
    }, 2800);
}
window.notify = notify;

// ── Navigation & History Stack (Bug-Free Single-Step Navigation) ────────────
function showPage(pageId, pushHistory = true) {
    if (!document.getElementById(pageId)) return;

    if (pushHistory && pageId !== currentPage) {
        navHistory.push(pageId);
        try {
            window.history.pushState({ page: pageId }, '', '#' + pageId);
        } catch (e) {}
    }
    currentPage = pageId;

    document.querySelectorAll(".page").forEach(p => { p.style.display = "none"; });
    document.getElementById(pageId).style.display = "flex";
    
    var m = document.querySelector(".menu");
    if (m) m.style.display = "none";

    var tSpan = document.getElementById("mainTitle");
    if (tSpan && pageTitles && pageTitles[pageId]) {
        tSpan.innerText = pageTitles[pageId][language] || pageTitles[pageId]["FA"];
    }

    updateHeaderUI();
    scrollToTop();
}

function goBack() {
    if (currentModal !== "none") {
        closeModal(currentModal);
        return;
    }
    if (navHistory.length > 1) {
        navHistory.pop(); // remove current
        let prev = navHistory[navHistory.length - 1] || 'main';
        showPage(prev, false);
    } else {
        showPage('main', false);
    }
}

window.onpopstate = function (e) {
    if (e.state && e.state.page) {
        showPage(e.state.page, false);
    } else {
        goBack();
    }
};

function updateHeaderUI() {
    var backBtn = document.getElementById("headerBackBtn");
    var langSel = document.getElementById("languageSelector");
    var helperIcon = document.getElementById("Helpper");

    if (currentPage === "main") {
        if (backBtn) backBtn.style.display = "none";
        if (langSel) langSel.style.display = "inline-block";
        if (helperIcon) helperIcon.style.display = "none";
    } else {
        if (backBtn) backBtn.style.display = "flex";
        if (langSel) langSel.style.display = "none";
        if (helperIcon) helperIcon.style.display = "flex";
    }
}

function scrollToTop() {
    window.scrollTo({ top: 0, behavior: "smooth" });
}

// ── Modals Management ───────────────────────────────────────────────────────
function updateHelperImage() {
    var wrap = document.getElementById("helperImgWrap");
    var img = document.getElementById("helperImg");
    if (!wrap || !img) return;

    function applyImg() {
        if (typeof IMG_MAP !== "undefined" && IMG_MAP) {
            var lang = language || "FA";
            var key = currentPage + "-" + lang;
            var b64 = IMG_MAP[key] || IMG_MAP[currentPage + "-FA"] || IMG_MAP[currentPage + "-EN"];
            if (b64) {
                img.src = b64;
                wrap.style.display = "block";
                return;
            }
        }
        wrap.style.display = "none";
        img.src = "";
    }

    if (typeof IMG_MAP === "undefined") {
        if (!window._loadingImgsJs) {
            window._loadingImgsJs = true;
            var s = document.createElement("script");
            s.src = "imgs.js";
            s.onload = function() { window._loadingImgsJs = false; applyImg(); };
            s.onerror = function() { window._loadingImgsJs = false; wrap.style.display = "none"; };
            document.head.appendChild(s);
        }
    } else {
        applyImg();
    }
}

function openModal(id, targetPage = null) {
    var m = document.getElementById(id);
    if (!m) return;
    if (id === "HelperModal") {
        updateHelperImage();
    }
    m.style.display = "flex";
    currentModal = id;
    if (targetPage) pageToOpen = targetPage;
}

function closeModal(id) {
    var m = document.getElementById(id);
    if (m) m.style.display = "none";
    if (currentModal === "jackModal" && connection && connection.readyState === 1) {
        connection.send(JSON.stringify({ RQ: RQtype.STOP_JACK }));
    }
    currentModal = "none";
}

document.addEventListener("DOMContentLoaded", () => {
    document.querySelectorAll(".close-btn").forEach(btn => {
        btn.addEventListener("click", () => {
            var m = btn.closest(".modal");
            if (m) closeModal(m.id);
        });
    });
    document.querySelectorAll(".modal").forEach(m => {
        m.addEventListener("click", (e) => {
            if (e.target === m) closeModal(m.id);
        });
    });
});

// ── Programs & Parameter Management ─────────────────────────────────────────
function appendProgramCard(programs, activeId = 1) {
    let mainList = document.getElementById("programSettingsContent");
    let modalList = document.getElementById("programSettingsContent2");
    if (mainList) mainList.innerHTML = "";
    if (modalList) modalList.innerHTML = "";

    programs.forEach((prog, idx) => {
        let pId = (prog.id !== undefined) ? prog.id : (idx + 1);
        prog.id = pId;

        // Card for Program Settings Page
        if (mainList) {
            let card = createProgramCardElement(prog, pId, activeId === pId, () => {
                setCurrentProgram(prog, `تنظیم برنامه P${pId}`, pId);
                openModal("programModal");
            });
            mainList.appendChild(card);
        }

        // Card for Quick-Select Modal
        if (modalList) {
            let mCard = createProgramCardElement(prog, pId, activeId === pId, () => {
                closeModal("programsModal");
                onChooseProgram(pId);
            });
            modalList.appendChild(mCard);
        }
    });
}

function createProgramCardElement(prog, pId, isSelected, onClickHandler) {
    let card = document.createElement("div");
    card.className = `dashCard slim ${isSelected ? "selected" : ""}`;
    card.onclick = onClickHandler;

    let row = document.createElement("div");
    row.className = "flex-row";

    let label = document.createElement("p");
    label.className = "counter";
    label.style.color = "var(--primary-teal)";
    label.textContent = `برنامه P${pId}`;
    row.appendChild(label);

    let infoDiv = document.createElement("div");
    infoDiv.style.display = "flex";
    infoDiv.style.gap = "14px";

    let tSpan = document.createElement("span");
    tSpan.style.color = "#b96900";
    tSpan.style.fontWeight = "700";
    tSpan.textContent = `🌡️ ${prog.t}°C`;
    infoDiv.appendChild(tSpan);

    let hSpan = document.createElement("span");
    hSpan.style.color = "#ff281d";
    hSpan.style.fontWeight = "700";
    hSpan.textContent = `🔥 ${prog.h}s`;
    infoDiv.appendChild(hSpan);

    let lSpan = document.createElement("span");
    lSpan.style.color = "#0d8d79";
    lSpan.style.fontWeight = "700";
    lSpan.textContent = `❄️ ${prog.l}s`;
    infoDiv.appendChild(lSpan);

    row.appendChild(infoDiv);
    card.appendChild(row);
    return card;
}

function setCurrentProgram(prog, label, pId) {
    currentProgram = { ...prog, id: (prog.id !== undefined ? prog.id : pId), label: label };
    let heatInp = document.getElementById("heaterInput2");
    let coolInp = document.getElementById("coolerInput2");
    let tempInp = document.getElementById("tempInput2");
    let titleEl = document.getElementById("programTitle");

    if (titleEl) titleEl.textContent = label || `برنامه P${currentProgram.id}`;
    if (heatInp) heatInp.value = currentProgram.h;
    if (coolInp) coolInp.value = currentProgram.l;
    if (tempInp) tempInp.value = currentProgram.t;
}

function onChooseProgram(progId) {
    progId = parseInt(progId, 10) || 1;
    selectedProgram.id = progId;
    for (let p of programData) {
        if (p.id === progId) {
            selectedProgram = { ...p };
            break;
        }
    }
    refreshselectedProgram(selectedProgram);
    appendProgramCard(programData, progId);
    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify({ RQ: RQtype.SET_PROG, ID: progId }));
    }
}

function submitProgram() {
    let heatVal = parseInt(document.getElementById("heaterInput2").value, 10);
    let coolVal = parseInt(document.getElementById("coolerInput2").value, 10);
    let tempVal = parseInt(document.getElementById("tempInput2").value, 10);

    if (isNaN(heatVal) || isNaN(coolVal) || isNaN(tempVal) || tempVal < 0 || tempVal > 350) {
        notify("مقادیر وارد شده نامعتبر هستند", "error");
        return;
    }

    closeModal("programModal");

    let progId = currentProgram.id || 1;
    for (let p of programData) {
        if (p.id === progId) {
            p.h = heatVal; p.l = coolVal; p.t = tempVal;
            break;
        }
    }
    if (selectedProgram.id === progId) {
        selectedProgram.h = heatVal; selectedProgram.l = coolVal; selectedProgram.t = tempVal;
        refreshselectedProgram(selectedProgram);
    }
    appendProgramCard(programData, selectedProgram.id);

    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify({
            RQ: RQtype.UPD_PROG,
            ID: progId,
            h: heatVal,
            l: coolVal,
            t: tempVal
        }));
    }
    notify("برنامه جوشکاری با موفقیت ذخیره شد", "success");
}

function refreshselectedProgram(prog) {
    if (!prog) return;
    selectedProgram = prog;
    window.selectedProgram = prog;
    let pEl = document.getElementById("current_P");
    let tEl = document.getElementById("current_T");
    let lEl = document.getElementById("current_L");
    let hEl = document.getElementById("current_H");

    if (pEl) pEl.innerHTML = "P" + prog.id;
    if (tEl) tEl.innerHTML = prog.t + "°C";
    if (lEl) lEl.innerHTML = prog.l + " ثانیه";
    if (hEl) hEl.innerHTML = prog.h + " ثانیه";
}

// ── Jack Timings Management ─────────────────────────────────────────────────
function appendJackCard(jacks, labels) {
    let container = document.getElementById("motionSettingsContent");
    if (!container) return;
    container.innerHTML = "";

    jacks.forEach((jack, idx) => {
        let jId = (jack.id !== undefined) ? jack.id : idx;
        jack.id = jId;
        let labelName = (labels[language] && labels[language][idx]) ? labels[language][idx] : `جک ${jId}`;

        let card = document.createElement("div");
        card.className = "dashCard slim";
        card.onclick = () => {
            setCurrentJack(jack, labelName, jId);
            let uloadSection = document.getElementById("uloadJack");
            if (uloadSection) {
                uloadSection.style.display = (jId === 4 || jId === 5) ? "none" : "flex";
            }
            openModal("jackModal");
        };

        let row = document.createElement("div");
        row.className = "flex-row";

        let p = document.createElement("p");
        p.className = "counter";
        p.textContent = labelName;
        row.appendChild(p);

        let timesDiv = document.createElement("div");
        timesDiv.style.display = "flex";
        timesDiv.style.gap = "12px";

        let inSpan = document.createElement("span");
        inSpan.style.color = "#ff281d";
        inSpan.style.fontWeight = "700";
        inSpan.textContent = `رفت: ${jack.l}ms`;
        timesDiv.appendChild(inSpan);

        if (jId < 4) {
            let outSpan = document.createElement("span");
            outSpan.style.color = "#0d8d79";
            outSpan.style.fontWeight = "700";
            outSpan.textContent = `برگشت: ${jack.ul}ms`;
            timesDiv.appendChild(outSpan);
        }

        row.appendChild(timesDiv);
        card.appendChild(row);
        container.appendChild(card);
    });
}

function setCurrentJack(jack, labelName, jId) {
    currentJack = { ...jack, id: (jack.id !== undefined ? jack.id : jId), label: labelName };
    let heatInp = document.getElementById("heaterInput");
    let coolInp = document.getElementById("coolerInput");
    let titleEl = document.getElementById("JackTitle");

    if (titleEl) titleEl.textContent = labelName;
    if (heatInp) heatInp.value = jack.l;
    if (coolInp) coolInp.value = jack.ul;
}

function submitJack() {
    let loadVal = parseInt(document.getElementById("heaterInput").value, 10);
    let unloadVal = parseInt(document.getElementById("coolerInput").value, 10);
    let jId = currentJack.id;

    closeModal("jackModal");

    let lValToSend = loadVal;
    let ulValToSend = (jId >= 4) ? 0 : unloadVal;
    if (jId === 4 || jId === 5) lValToSend = Math.round(loadVal / 1000);

    for (let j of jackData) {
        if (j.id === jId) {
            j.l = loadVal; j.ul = unloadVal;
            break;
        }
    }
    appendJackCard(jackData, jackLabels);

    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify({
            RQ: RQtype.UPD_JACK,
            ID: jId,
            l: lValToSend,
            ul: ulValToSend
        }));
    }
    notify("تایمر جک با موفقیت ثبت شد", "success");
}

// ── General Settings (PID, Calib, Toggles) ──────────────────────────────────
function onSaveStates() {
    let calibVal = parseInt(document.getElementById("CalibValueTitle").value, 10) || 0;
    let pidInp = document.getElementById("PID");
    let pidVal = (pidInp && pidInp.value !== "") ? parseInt(pidInp.value, 10) : 15;

    let spk = document.getElementById("spkToggle") ? document.getElementById("spkToggle").classList.contains("active") : true;
    let spkVol = document.getElementById("buzzer-vol-slider") ? parseInt(document.getElementById("buzzer-vol-slider").value, 10) : 80;
    let aut = document.getElementById("autoStartToggle") ? document.getElementById("autoStartToggle").classList.contains("active") : false;
    let end = document.getElementById("endModeToggle") ? document.getElementById("endModeToggle").classList.contains("active") : true;
    let emg = document.getElementById("EmgncyToggle") ? document.getElementById("EmgncyToggle").classList.contains("active") : false;
    let tLmt = document.getElementById("tLimitToggle") ? document.getElementById("tLimitToggle").classList.contains("active") : false;
    let pedal = document.getElementById("pedalToggle") && document.getElementById("pedalToggle").classList.contains("active") ? 4 : 3;

    let payload = {
        RQ: RQtype.CONFIG,
        PID: pidVal,
        Tcal: calibVal,
        spk: spk,
        spkVol: spkVol,
        aut: aut,
        end: end,
        emg: emg,
        tLmt: tLmt,
        pedal: pedal
    };

    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify(payload));
    }
    notify("تنظیمات عمومی با موفقیت ذخیره گردید", "success");
}

function updateCalibTemp(val) {
    val = parseInt(val, 10) || 0;
    if (val > 20) val = 20;
    if (val < -20) val = -20;
    document.getElementById("CalibValueTitle").value = val;
    document.getElementById("CalibInput").value = val;
    document.getElementById("CalibSumTitle").innerHTML = (val + parseInt(current_temp) - parseInt(temp_setPoint)) + "°C";
}

function incrementInput(id, step) {
    let el = document.getElementById(id);
    if (!el) return;
    let v = parseInt(el.value, 10) || 0;
    let max = parseInt(el.max, 10) || 9999;
    if (v + step <= max) el.value = v + step;
}

function decrementInput(id, step) {
    let el = document.getElementById(id);
    if (!el) return;
    let v = parseInt(el.value, 10) || 0;
    let min = parseInt(el.min, 10) || 0;
    if (v - step >= min) el.value = v - step;
}

// ── Admin Password & Actions ────────────────────────────────────────────────
function submitPassword() {
    var rawPass = document.getElementById("passwordInput").value.trim();
    passwordInput = "1" + rawPass;
    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify({ RQ: RQtype.AUTH, psw: passwordInput }));
    }
}

function onResetCounter() {
    onConfirm("آیا از ریست کردن شمارنده شیفت اطمینان دارید؟", { RQ: RQtype.RESET_COUNT });
}

function onRessetFactory() {
    onConfirm("آیا از بازگردانی تمامی پارامترها به تنظیمات کارخانه اطمینان دارید؟", { RQ: RQtype.RESET_FACTORY });
}

function onConfirm(msg, payload) {
    var m = document.getElementById("confirmModal");
    var txt = document.getElementById("confirmText");
    var btn = document.getElementById("confirmButton");
    if (!m || !txt || !btn) return;
    txt.innerHTML = msg;
    m.style.display = "flex";
    btn.onclick = () => {
        m.style.display = "none";
        if (connection && connection.readyState === 1) {
            connection.send(JSON.stringify(payload));
        }
    };
}

// ── Toggles ─────────────────────────────────────────────────────────────────
function toggleSpeaker() {
    var t = document.getElementById("spkToggle");
    if (t) {
        t.classList.toggle("active");
        var isActive = t.classList.contains("active");
        var bWrap = document.getElementById("buzzer-vol-wrap");
        if (bWrap) bWrap.style.opacity = isActive ? "1" : "0.4";
        if (isActive) playWebAudioBeep(80, 2400, 80);
    }
}
function toggleAutoStart() { var t = document.getElementById("autoStartToggle"); if (t) t.classList.toggle("active"); }
function toggleEndMode() { var t = document.getElementById("endModeToggle"); if (t) t.classList.toggle("active"); }
function toggleTLimitter() { var t = document.getElementById("tLimitToggle"); if (t) t.classList.toggle("active"); }
function togglePedal() { var t = document.getElementById("pedalToggle"); if (t) t.classList.toggle("active"); }

// ── Buzzer & Audio Feedback (Ice Cream Style) ───────────────────────────────
function playWebAudioBeep(volPercent, freq, durationMs) {
    try {
        var AudioCtx = window.AudioContext || window.webkitAudioContext;
        if (!AudioCtx) return;
        if (!window._hafflerAudioCtx) {
            window._hafflerAudioCtx = new AudioCtx();
        }
        var ctx = window._hafflerAudioCtx;
        if (ctx.state === "suspended") ctx.resume();
        var osc = ctx.createOscillator();
        var gain = ctx.createGain();
        var v = (volPercent !== undefined && !isNaN(volPercent)) ? Number(volPercent) : 80;
        var normVol = Math.min(1, Math.max(0.01, (v / 100) * 0.35));
        gain.gain.setValueAtTime(normVol, ctx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.0001, ctx.currentTime + (durationMs || 100) / 1000);
        osc.type = "square";
        osc.frequency.setValueAtTime(freq || 2400, ctx.currentTime);
        osc.connect(gain);
        gain.connect(ctx.destination);
        osc.start();
        osc.stop(ctx.currentTime + (durationMs || 100) / 1000);
    } catch(e) {}
}

function onBuzzerVolumeChange(val) {
    var el = document.getElementById("val-buzzer-vol");
    if (el) el.innerText = val + "%";
    playWebAudioBeep(Number(val), 2400, 50);
    if (window._bzTimer) clearTimeout(window._bzTimer);
    window._bzTimer = setTimeout(function() {
        if (connection && connection.readyState === 1) {
            connection.send(JSON.stringify({ RQ: RQtype.TEST_BUZZER, vol: Number(val) }));
        } else {
            fetch("/api/cmd", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ action: "buzzer_test", vol: Number(val) })
            }).catch(function(){});
        }
    }, 120);
}

function testBuzzerWeb() {
    var slider = document.getElementById("buzzer-vol-slider");
    var vol = slider ? parseInt(slider.value, 10) : 80;
    if (isNaN(vol)) vol = 80;
    playWebAudioBeep(vol, 2400, 80);
    setTimeout(function() { playWebAudioBeep(vol, 2400, 80); }, 140);

    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify({ RQ: RQtype.TEST_BUZZER, vol: vol }));
    } else {
        fetch("/api/cmd", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ action: "buzzer_test", vol: vol })
        }).catch(function(){});
    }
    notify("🔊 تست صدای بازر به کنترلر ارسال شد", "info");
}

// ── Wi-Fi Manager Operations ────────────────────────────────────────────────
function scanWifiNetworks() {
    var btn = document.getElementById("btnWifiScan");
    var select = document.getElementById("wifi_scan_select");
    var status = document.getElementById("wifiScanStatus");
    if (btn) { btn.disabled = true; btn.innerText = "⏳ در حال جستجو..."; }
    if (status) { status.innerText = "در حال اسکن فرکانس ۲.۴ گیگاهرتز..."; status.style.color = "#0284c7"; }

    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify({ RQ: RQtype.WIFI_SCAN }));
    } else {
        fetch("/api/wifi_scan")
            .then(function(r) { return r.json(); })
            .then(function(data) { handleWifiScanResults(data.networks); })
            .catch(function() {
                if (btn) { btn.disabled = false; btn.innerText = "🔍 اسکن شبکه‌های وای‌فای"; }
                if (status) { status.innerText = "خطا در برقراری ارتباط با کنترلر."; status.style.color = "#dc2626"; }
            });
    }
}

function handleWifiScanResults(networks) {
    var btn = document.getElementById("btnWifiScan");
    var select = document.getElementById("wifi_scan_select");
    var status = document.getElementById("wifiScanStatus");
    if (btn) { btn.disabled = false; btn.innerText = "🔍 اسکن شبکه‌های وای‌فای"; }
    if (!select) return;
    select.innerHTML = '<option value="">-- انتخاب شبکه یافت شده --</option>';
    if (networks && networks.length > 0) {
        networks.forEach(function(net) {
            var opt = document.createElement("option");
            opt.value = net.ssid;
            var lock = net.enc ? "🔒" : "🔓";
            opt.innerText = `${lock} ${net.ssid} (${net.rssi} dBm)`;
            select.appendChild(opt);
        });
        if (status) { status.innerText = `تعداد ${networks.length} شبکه در دسترس یافت شد.`; status.style.color = "#16a34a"; }
    } else {
        if (status) { status.innerText = "هیچ شبکه‌ای یافت نشد. فاصله با مودم را بررسی کنید."; status.style.color = "#dc2626"; }
    }
}

function onWifiSelectChange(val) {
    if (!val) return;
    var ssidInp = document.getElementById("wifiSsidInput");
    if (ssidInp) ssidInp.value = val;
    var pwdInp = document.getElementById("wifiPassInput");
    if (pwdInp) pwdInp.focus();
}

function connectToWifiNetwork() {
    var ssidInp = document.getElementById("wifiSsidInput");
    var passInp = document.getElementById("wifiPassInput");
    var ssid = ssidInp ? ssidInp.value.trim() : "";
    var pass = passInp ? passInp.value : "";
    var status = document.getElementById("wifiConnStatus");

    if (!ssid) {
        notify("لطفاً نام شبکه (SSID) را وارد یا انتخاب کنید", "error");
        return;
    }

    if (status) {
        status.innerText = "در حال ارسال اطلاعات و اتصال به مودم... لطفاً چند لحظه شکیبا باشید";
        status.style.color = "#0284c7";
    }

    var payload = { RQ: RQtype.WIFI_CONNECT, ssid: ssid, pass: pass };
    if (connection && connection.readyState === 1) {
        connection.send(JSON.stringify(payload));
    } else {
        fetch("/api/wifi_connect", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ ssid: ssid, pass: pass })
        }).catch(function(){});
    }
    notify("دستور اتصال به مودم کارگاه ارسال شد", "info");
}

function toggleWifiPassVisibility() {
    var passInp = document.getElementById("wifiPassInput");
    var eyeBtn = document.getElementById("wifiEyeBtn");
    if (!passInp) return;
    if (passInp.type === "password") {
        passInp.type = "text";
        if (eyeBtn) eyeBtn.innerText = "🙈";
    } else {
        passInp.type = "password";
        if (eyeBtn) eyeBtn.innerText = "👁️";
    }
}

function updateWifiUIState(connected, ssid, ip, rssi) {
    var badge = document.getElementById("wifiStateBadge");
    var ssidEl = document.getElementById("wifiCurSsid");
    var ipEl = document.getElementById("wifiCurIp");
    var rssiEl = document.getElementById("wifiCurRssi");
    if (badge) {
        if (connected) {
            badge.style.background = "#dcfce7";
            badge.style.color = "#15803d";
            badge.innerText = "متصل به مودم کارگاه";
        } else {
            badge.style.background = "#fee2e2";
            badge.style.color = "#b91c1c";
            badge.innerText = "قطع (هات‌اسپات داخلی)";
        }
    }
    if (ssidEl) ssidEl.innerText = ssid || "---";
    if (ipEl) ipEl.innerText = ip || "---";
    if (rssiEl) rssiEl.innerText = (rssi ? (rssi + " dBm") : "---");
}

// ── Interactive AJAX OTA Firmware Upload ────────────────────────────────────
function onOtaFileSelected(input) {
    var nameEl = document.getElementById("otaSelectedFileName");
    var sizeEl = document.getElementById("otaSelectedFileSize");
    var btnSubmit = document.getElementById("otaSubmitBtn");
    if (input.files && input.files.length > 0) {
        var file = input.files[0];
        if (nameEl) nameEl.innerText = file.name;
        if (sizeEl) sizeEl.innerText = (file.size / (1024 * 1024)).toFixed(2) + " MB (" + file.size.toLocaleString() + " بایت)";
        if (btnSubmit) btnSubmit.disabled = false;
    } else {
        if (nameEl) nameEl.innerText = "برای انتخاب فایل فریمور (bin.) کلیک کنید";
        if (sizeEl) sizeEl.innerText = "";
        if (btnSubmit) btnSubmit.disabled = true;
    }
}

function handleFirmwareUpload(e) {
    e.preventDefault();
    var fileInput = document.getElementById("otaFileInput");
    if (!fileInput || !fileInput.files || fileInput.files.length === 0) {
        notify("لطفاً ابتدا فایل فریمور (bin.) را انتخاب نمایید", "error");
        return;
    }
    var file = fileInput.files[0];
    if (!file.name.toLowerCase().endsWith(".bin")) {
        notify("فرمت فایل نامعتبر است! تنها فایل‌های bin. پذیرفته می‌شوند", "error");
        return;
    }

    var progWrap = document.getElementById("otaProgressWrap");
    var progBar = document.getElementById("otaProgressBar");
    var progTxt = document.getElementById("otaProgressPercent");
    var progStats = document.getElementById("otaProgressStats");
    var btnSubmit = document.getElementById("otaSubmitBtn");
    var resultBox = document.getElementById("otaResultBox");

    if (progWrap) progWrap.style.display = "block";
    if (btnSubmit) btnSubmit.disabled = true;
    if (resultBox) resultBox.style.display = "none";

    var startTime = Date.now();
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/update", true);

    xhr.upload.onprogress = function(event) {
        if (event.lengthComputable) {
            var percent = Math.round((event.loaded / event.total) * 100);
            if (progBar) progBar.style.width = percent + "%";
            if (progTxt) progTxt.innerText = percent + "%";
            var elapsedSec = (Date.now() - startTime) / 1000;
            var speedKb = elapsedSec > 0 ? Math.round((event.loaded / 1024) / elapsedSec) : 0;
            var loadedMb = (event.loaded / (1024 * 1024)).toFixed(2);
            var totalMb = (event.total / (1024 * 1024)).toFixed(2);
            if (progStats) {
                progStats.innerText = `${loadedMb} MB از ${totalMb} MB (سرعت انتقال: ${speedKb} KB/s)`;
            }
        }
    };

    function startRestartCountdown(title, subtitle, color) {
        if (progBar) progBar.style.width = "100%";
        if (progTxt) progTxt.innerText = "۱۰۰%";
        if (resultBox) {
            resultBox.style.display = "block";
            resultBox.className = "dashCard";
            resultBox.style.borderTop = "3px solid " + color;
            resultBox.innerHTML = `
                <div style="text-align:center; padding:12px; width:100%;">
                    <div style="font-size:32px;">✅</div>
                    <h4 style="color:${color}; margin:6px 0;">${title}</h4>
                    <p style="color:#64748b; font-size:13px; margin:4px 0 12px;">${subtitle}</p>
                    <div id="otaCountdown" style="font-size:18px; font-weight:700; color:#0284c7;">اتصال مجدد خودکار تا: ۱۰ ثانیه</div>
                </div>
            `;
        }
        var countdown = 10;
        var timer = setInterval(function() {
            countdown--;
            var cdEl = document.getElementById("otaCountdown");
            if (cdEl) cdEl.innerText = "اتصال مجدد خودکار تا: " + countdown + " ثانیه";
            if (countdown <= 0) {
                clearInterval(timer);
                window.location.reload();
            }
        }, 1000);
    }

    xhr.onload = function() {
        if (xhr.status === 200 && (xhr.responseText.trim() === "OK" || xhr.responseText.trim() === "")) {
            startRestartCountdown("فریمور با موفقیت بارگذاری شد", "کنترلر هم‌اکنون در حال ری‌استارت و اعمال فریمور جدید است.", "#16a34a");
        } else {
            notify("خطا در بارگذاری فریمور: " + (xhr.responseText || "پاسخ نامعتبر"), "error");
            if (btnSubmit) btnSubmit.disabled = false;
        }
    };

    xhr.onerror = function() {
        startRestartCountdown("کنترلر در حال بازنشانی است", "ارسال فریمور تکمیل شد. دستگاه در حال راه‌اندازی مجدد است...", "#0284c7");
    };

    var formData = new FormData();
    formData.append("update", file);
    xhr.send(formData);
}

// ── WebSocket Architecture with Live Sync ───────────────────────────────────
var wsAttemptCount = 0;
function initWebSocket() {
    let host = window.location.hostname;
    // پشتیبانی کامل از نام دامنه haffler.local، آدرس IP و فال‌بک ایمن به 192.168.4.1 در صورت خطای mDNS موبایل
    if (!host) {
        host = "192.168.4.1";
    } else if (host === "localhost" || host === "127.0.0.1") {
        // Keep localhost/127.0.0.1 for local testing/simulator
    } else if (wsAttemptCount > 0 && host.endsWith(".local")) {
        // فال‌بک قطعی: اندروید در وب‌سوکت نام دامنه mDNS (.local) را پشتیبانی نمی‌کند
        host = "192.168.4.1";
    } else if (!/^\d+\.\d+\.\d+\.\d+$/.test(host) && !host.endsWith(".local")) {
        host = "192.168.4.1";
    }
    let wsUrl = `ws://${host}:81/`;

    try {
        connection = new WebSocket(wsUrl, ["arduino"]);
        window.connection = connection;
    } catch (e) {
        wsAttemptCount++;
        scheduleReconnect();
        return;
    }

    connection.onopen = function () {
        ONLINE_Flag = true;
        wsAttemptCount = 0;
        reconnectInterval = 1500;
        if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null; }
        var badge = document.getElementById("liveConnectionBadge");
        if (badge) { badge.style.background = "#00e676"; badge.title = "متصل به کنترلر"; }
    };

    connection.onmessage = function (e) {
        try {
            var msg = JSON.parse(e.data);
            handlePlcMessage(msg);
        } catch (err) {}
    };

    connection.onerror = function () {
        ONLINE_Flag = false;
        wsAttemptCount++;
        var badge = document.getElementById("liveConnectionBadge");
        if (badge) { badge.style.background = "#ff1744"; badge.title = "قطع ارتباط"; }
        scheduleReconnect();
    };

    connection.onclose = function () {
        ONLINE_Flag = false;
        wsAttemptCount++;
        var badge = document.getElementById("liveConnectionBadge");
        if (badge) { badge.style.background = "#ff1744"; badge.title = "تلاش مجدد..."; }
        scheduleReconnect();
    };
}

function scheduleReconnect() {
    if (reconnectTimer) return;
    reconnectTimer = setTimeout(() => {
        reconnectTimer = null;
        reconnectInterval = Math.min(reconnectInterval * 1.5, 6000);
        initWebSocket();
    }, reconnectInterval);
}

function handlePlcMessage(msg) {
    if (msg.DID !== 555) return;

    switch (msg.RQ) {
        case RQtype.HAND_SHAKE:
            applyHandshakeData(msg);
            if (!isHandshaked) {
                closeLoading();
                isHandshaked = true;
            }
            break;

        case RQtype.AUTH:
            closeModal("passwordModal");
            showPage(pageToOpen);
            break;

        case RQtype.WRNG_ADMIN_PASS:
            openModal("passwordModal");
            var errEl = document.getElementById("passwordError");
            if (errEl) errEl.style.display = "block";
            notify("رمز عبور مدیر نادرست است", "error");
            break;

        case RQtype.RQ_DONE:
            notify("دستور با موفقیت انجام شد", "success");
            break;

        case RQtype.WIFI_SCAN:
            handleWifiScanResults(msg.networks);
            break;

        case RQtype.NEWS:
            var tNum = (msg.T === "Error" || isNaN(parseInt(msg.T, 10))) ? 0 : parseInt(msg.T, 10);
            current_temp = tNum;
            window.current_temp = tNum;
            var tmEl = document.getElementById("TM");
            if (tmEl) tmEl.innerHTML = tNum + "°C";
            if (msg.WM !== undefined && document.getElementById("WM")) document.getElementById("WM").innerHTML = msg.WM;
            if (msg.PM !== undefined && document.getElementById("PM")) {
                document.getElementById("PM").innerHTML = msg.PM;
                if (typeof updateTeflonUI === "function") updateTeflonUI();
            }

            // Live I/O Badges
            if (msg.SSR !== undefined) {
                var dSSR = document.getElementById("dotSSR");
                if (dSSR) dSSR.className = (msg.SSR === 1) ? "io-dot ssr-active" : "io-dot";
            }
            if (msg.Q_ELM !== undefined) {
                var dElm = document.getElementById("dotElm");
                if (dElm) dElm.className = (msg.Q_ELM === 1) ? "io-dot active" : "io-dot";
            }
            if (msg.Q_SHB !== undefined) {
                var dShb = document.getElementById("dotShb");
                if (dShb) dShb.className = (msg.Q_SHB === 1) ? "io-dot active" : "io-dot";
            }
            if (msg.Q_PRS !== undefined) {
                var dPrs = document.getElementById("dotPress");
                if (dPrs) dPrs.className = (msg.Q_PRS === 1) ? "io-dot active" : "io-dot";
            }
            if (msg.Q_TAB !== undefined) {
                var dTab = document.getElementById("dotTable");
                if (dTab) dTab.className = (msg.Q_TAB === 1) ? "io-dot active" : "io-dot";
            }
            if (msg.EMG !== undefined) {
                var dEmg = document.getElementById("dotEmg");
                if (dEmg) dEmg.className = (msg.EMG === 1) ? "io-dot active" : "io-dot";
            }

            // Live 16-channel IO Dashboard
            if (msg.IN_BITS !== undefined) {
                for (var i = 0; i < 8; i++) {
                    var inDot = document.getElementById("io_in_dot_" + i);
                    if (inDot) {
                        var isOn = (msg.IN_BITS & (1 << i)) !== 0;
                        inDot.className = isOn ? "io-dash-dot on" : "io-dash-dot off";
                    }
                }
            }
            if (msg.OUT_BITS !== undefined) {
                for (var j = 0; j < 8; j++) {
                    var outDot = document.getElementById("io_out_dot_" + j);
                    if (outDot) {
                        var isOutOn = (msg.OUT_BITS & (1 << j)) !== 0;
                        outDot.className = isOutOn ? "io-dash-dot on" : "io-dash-dot off";
                    }
                }
            }
            break;
    }
}

function applyHandshakeData(t) {
    if (t.OTA) showPage("OTA");

    selectedProgram = { id: t.PU || 1, h: t.H || 25, l: t.L || 30, t: t.T || 250 };
    if (t.prg) programData = t.prg;
    if (t.jks) jackData = t.jks;

    appendJackCard(jackData, jackLabels);
    appendProgramCard(programData, t.PU || 1);
    refreshselectedProgram(selectedProgram);

    var _tnum = (t.TM === "Error" || isNaN(parseInt(t.TM, 10))) ? 0 : parseInt(t.TM, 10);
    current_temp = _tnum;
    if (document.getElementById("TM")) document.getElementById("TM").innerHTML = _tnum + "°C";
    if (document.getElementById("WM")) document.getElementById("WM").innerHTML = t.WM || 0;
    if (document.getElementById("PM")) document.getElementById("PM").innerHTML = t.PM || 0;
    if (document.getElementById("PID")) document.getElementById("PID").value = (t.PID !== undefined) ? t.PID : 15;

    temp_setPoint = t.Tcal || 0;
    if (document.getElementById("CalibValueTitle")) document.getElementById("CalibValueTitle").value = temp_setPoint;
    if (document.getElementById("CalibInput")) document.getElementById("CalibInput").value = temp_setPoint;
    if (document.getElementById("CalibSumTitle")) document.getElementById("CalibSumTitle").innerHTML = _tnum + "°C";

    // Initialize live I/O badges from handshake
    if (t.SSR !== undefined) {
        var dSSR = document.getElementById("dotSSR");
        if (dSSR) dSSR.className = (t.SSR === 1) ? "io-dot ssr-active" : "io-dot";
    }
    if (t.Q_ELM !== undefined) {
        var dElm = document.getElementById("dotElm");
        if (dElm) dElm.className = (t.Q_ELM === 1) ? "io-dot active" : "io-dot";
    }
    if (t.Q_SHB !== undefined) {
        var dShb = document.getElementById("dotShb");
        if (dShb) dShb.className = (t.Q_SHB === 1) ? "io-dot active" : "io-dot";
    }
    if (t.Q_PRS !== undefined) {
        var dPrs = document.getElementById("dotPress");
        if (dPrs) dPrs.className = (t.Q_PRS === 1) ? "io-dot active" : "io-dot";
    }
    if (t.Q_TAB !== undefined) {
        var dTab = document.getElementById("dotTable");
        if (dTab) dTab.className = (t.Q_TAB === 1) ? "io-dot active" : "io-dot";
    }
    if (t.EMG !== undefined) {
        var dEmg = document.getElementById("dotEmg");
        if (dEmg) dEmg.className = (t.EMG === 1) ? "io-dot active" : "io-dot";
    }

    // Synchronize General Settings toggles from PLC handshake
    function setToggle(id, active) {
        var el = document.getElementById(id);
        if (el) {
            if (active) el.classList.add("active");
            else el.classList.remove("active");
        }
    }
    if (t.spk !== undefined) {
        setToggle("spkToggle", !!t.spk);
        var bWrap = document.getElementById("buzzer-vol-wrap");
        if (bWrap) bWrap.style.opacity = t.spk ? "1" : "0.4";
    }
    if (t.spkVol !== undefined) {
        var sld = document.getElementById("buzzer-vol-slider");
        var valTxt = document.getElementById("val-buzzer-vol");
        if (sld && document.activeElement !== sld) sld.value = t.spkVol;
        if (valTxt) valTxt.innerText = t.spkVol + "%";
    }
    if (t.wifi_sta !== undefined) {
        updateWifiUIState(!!t.wifi_sta, t.wifi_ssid, t.wifi_ip, t.wifi_rssi);
    }
    if (t.aut !== undefined) setToggle("autoStartToggle", !!t.aut);
    if (t.end !== undefined) setToggle("endModeToggle", !!t.end);
    if (t.emg !== undefined) setToggle("EmgncyToggle", !!t.emg);
    if (t.tLmt !== undefined) setToggle("tLimitToggle", !!t.tLmt);
    if (t.pedal !== undefined) setToggle("pedalToggle", t.pedal === 4);
    if (t.lan !== undefined) {
        var hLang = (t.lan === 2) ? "EN" : ((t.lan === 3) ? "TU" : "FA");
        applyLanguage(hLang, false); // false = DO NOT echo back to PLC (breaks infinite loop)
    }

    // Expose runtime state globally for charts and diagnostic widgets
    window.selectedProgram = selectedProgram;
    window.current_temp = current_temp;
    window.programData = programData;
    window.jackData = jackData;
}

// Native initialization
initWebSocket();

// Language change listener (User explicit trigger)
document.addEventListener("DOMContentLoaded", function () {
    var langSel = document.getElementById("languageSelector");
    if (langSel) {
        langSel.addEventListener("change", function () {
            applyLanguage(this.value, true); // true = user explicitly changed it, sync to PLC
        });
    }
});

function applyLanguage(lang, syncToPlc = false) {
    if (lang !== "FA" && lang !== "EN" && lang !== "TU") lang = "FA";
    let langChanged = (language !== lang);
    language = lang;
    document.documentElement.dir = (lang === "FA") ? "rtl" : "ltr";
    document.documentElement.lang = (lang === "FA") ? "fa" : ((lang === "TU") ? "tr" : "en");

    var sel = document.getElementById("languageSelector");
    if (sel && sel.value !== lang) sel.value = lang;

    if (mapper[language]) {
        for (var key in mapper[language]) {
            var el = document.getElementById(key);
            if (el) {
                if (el.tagName === "INPUT" && (el.type === "button" || el.type === "submit")) {
                    el.value = mapper[language][key];
                } else {
                    el.innerText = mapper[language][key];
                }
            }
        }
    }

    if (typeof appendJackCard === "function") appendJackCard(jackData, jackLabels);
    if (typeof appendProgramCard === "function") appendProgramCard(programData, (selectedProgram && selectedProgram.id) || 1);
    updateHeaderUI();
    if (currentModal === "HelperModal") updateHelperImage();

    // ONLY send SET_LANG to PLC if user explicitly changed the language and it actually changed
    if (syncToPlc && langChanged && connection && connection.readyState === 1) {
        var langIdx = (lang === "FA") ? 1 : ((lang === "EN") ? 2 : 3);
        connection.send(JSON.stringify({ DID: 555, RQ: RQtype.SET_LANG, lan: langIdx, L: langIdx }));
    }
}
