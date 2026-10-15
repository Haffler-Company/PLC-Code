var InAPP_Flag = false,
  SUCCES_Flag = false,
  SNDIP_Flag = false,
  SNDPSS_Flag = false,
  ONLINE_Flag = true,
  SKIP_Flag = false,
  CONFIGMODE = false;
var globalIP = "0";
var globalpass = "";
var Spassword = getCookie("secure_pass");
var APASS;
var GB_Days = [];

function DelPass() {
  setCookie("secure_pass", "", 9000);
  Spassword = "";
  SNDPSS_Flag = true;
  document.querySelector(".status").innerHTML =
    "با موفقیت خارج شدید <div class='status_led'></div>";
}

function DoTheStuff(mode, dutys, _eUp, _cUp, _sUp, _pid, _isJ = false,lang) {
  if (mode == 0) {
    CONFIGMODE = true;
    SKIP_Flag = true;
  }
  document.getElementById("E_up").value = _eUp;
  document.getElementById("C_up").value = _cUp;
  document.getElementById("S_up").value = _sUp;
  document.getElementById("PID").value = _pid;
  document.getElementById("isJ").checked = _isJ;
  document.getElementById("ISTURK").checked = lang==2?true:false;
}

function showModemStt(modem, ip) {
  document.querySelector(".right-bar").classList.add("right_bar_animation");
  globalIP = ip;
}

function sendParams() {
  var eUp = parseInt(document.getElementById("E_up").value);
  var cUp = parseInt(document.getElementById("C_up").value);
  var sUp = parseInt(document.getElementById("S_up").value);
  var PID = parseInt(document.getElementById("PID").value);
  var isJ = document.getElementById("isJ").checked;
  var lang = document.getElementById("ISTURK").checked==true?2:1;

  var motSelect=parseInt(document.getElementById("motorSelect").value);
  var devSelect=parseInt(document.getElementById("deviceAbility").value);
  var bladeSelect=parseInt(document.getElementById("bladeMode").value);



  if (isNaN(eUp) || isNaN(cUp) || isNaN(sUp)) {
    alert("لطفا مقادیر را وارد کنید!");
    return;
  }
  if (eUp > 99 || cUp > 999 || sUp > 99 || eUp <= 0 || cUp <= 0 || sUp <= 0) {
    alert(
      "زمان به درستی وارد نشده است \n\r e_up کوچکتر از 99 \n\r c_up کوچکتر از 999 \n\r s_up کوچکتر از 99"
    );
    return;
  }
  if (PID > 100 || PID < 1) {
    alert("ضریب کنترلر عددی بین ۱ تا ۱۰۰ میباشد.");
    return;
  }

  var secu_password = APASS;
  connection.send(
    JSON.stringify({
      RQ: 8,
      secu_password,
      eUp,
      cUp,
      sUp,
      PID,
      isJ,
      mot: motSelect,
      abil: devSelect,
      blade:bladeSelect,
      lang
    })
  );
}
function sendDeviceMode(){
  var devMode=parseInt(document.getElementById("deviceMode").value);

  var secu_password = APASS;
  connection.send(
    JSON.stringify({
      RQ: 9,
      secu_password,
      devMode
    })
  );
}

function sendCPASS() {
  var Spassword = getCookie("secure_pass");
  connection.send(
    JSON.stringify({
      RQ: 1,
      password: Spassword,
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

connection.onopen = function () {
  sendCPASS();
  document.querySelector(".status").innerHTML =
    "متصل <div class='status_led'></div>";
  document.querySelector(".status_led").style.backgroundColor = "lightgreen";
};

connection.onerror = function (error) {
  ONLINE_Flag = false;
  console.log("WDCNTE");
  document.querySelector(".status").innerHTML =
    "خطایی رخ داد!<div class='status_led'></div>";
  document.querySelector(".status_led").style.backgroundColor = "red";
  if (!InAPP_Flag) {
    setTimeout(function () {
      location.reload();
    }, 2000);
  }
};

connection.onmessage = function (evt) {
  var jsonObject = JSON.parse(evt.data);
  var Did = jsonObject.DID;

  console.log(jsonObject);
  if (Did == 555) {
    var rgst = jsonObject.RQ;
    switch (rgst) {
      case 1:
        DoTheStuff(
          jsonObject.mode,
          jsonObject.DUTs,
          jsonObject.eUp,
          jsonObject.cUp,
          jsonObject.sUp,
          jsonObject.PID,
          jsonObject.isJ,
          jsonObject.lang
        );
        document.getElementById("motorSelect").value = jsonObject.motSelect;
        document.getElementById("deviceAbility").value = jsonObject.devSelect;
        document.getElementById("bladeMode").value = jsonObject.bladeSelect;
        document.getElementById("deviceMode").value = jsonObject.dvMode;

      
        _navShowMode(jsonObject.dvMode);
        break;
      case 2:
        adssids(jsonObject.ssids);
        break;
      case 3:
        SUCCES_Flag = true;
        if (jsonObject.Rtype == 1) showApRslt();
        else if (jsonObject.Rtype == 2)
          showModemRslt(jsonObject.STT, jsonObject.modem, jsonObject.IP);
        else showChPssRslt();
        break;
      case 4:
        Spassword = getCookie("secure_pass");
        if (SKIP_Flag != true) PassPopup(0);
        break;
      case 5:
        if (SKIP_Flag != true) PassPopup(1);
        break;
      case 6:
        if (jsonObject.hasOwnProperty("RSTT"))
          showDone2(jsonObject.Rnum, jsonObject.RSTT);
        else showDone(jsonObject.Rnum);
        break;
      case 7:
        showModemStt(jsonObject.modem, jsonObject.IP);
        break;
        case 8:
          alert("تنظیمات با موفقیت اعمال شد");
          break;
      case 11:
        // alert("رمز صحیح نیست.");
        break;
      case 13:
        sendIMhere();
        break;
      case 16:
        alert("زمان با موفقیت حذف شد");
        break;
      case 18:
        alert(
          "مشکلی پیش آمد لطفا دوباره امتحان کنید. ممکن است فرمت ورودی ها غلط باشند و یا فقط اشکال موقت در ذخیره اطلاعات اتفاق افتاده باشد."
        );
        break;
      case 19:
        var username = jsonObject.USRN;
        if (username == "رمز غیر فعال")
          document.querySelector(".status").innerHTML =
            "<div class='status_led'></div>" + username;
        else
          document.querySelector(".status").innerHTML =
            "<div class='status_led'></div>" +
            username +
            " <a onclick='DelPass()'style='color: #2980b9;'> [خروج]</a>";
        document.querySelector(".status_led").style.backgroundColor = "#2ecc71";
        break;
      case 20:
        alert("تنظیمات جدید ذخیره شد در حال بارگذاری مجدد...!");
        setTimeout(function () {
          location.reload();
        }, 500);
        break;
    }
  }
};

connection.onclose = function () {
  ONLINE_Flag = false;
  console.log("WDCNTC");
  window.navigator.vibrate([80, 50, 80, 80, 50, 80, 50, 100, 100]);
  document.querySelector(".status").innerHTML =
    "اتصال قطع شد!<div class='status_led'></div>";
  document.querySelector(".status_led").style.backgroundColor = "red";
  if (!InAPP_Flag) {
    setTimeout(function () {
      location.reload();
    }, 2000);
  }
};

function setCookie(cname, cvalue, exdays) {
  var d = new Date();
  d.setTime(d.getTime() + exdays * 24 * 60 * 60 * 1000);
  var expires = "expires=" + d.toGMTString();
  document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
}

function getCookie(cname) {
  var name = cname + "=";
  var decodedCookie = decodeURIComponent(document.cookie);
  var ca = decodedCookie.split(";");
  for (var i = 0; i < ca.length; i++) {
    var c = ca[i];
    while (c.charAt(0) == " ") {
      c = c.substring(1);
    }
    if (c.indexOf(name) == 0) {
      return c.substring(name.length, c.length);
    }
  }
  return "";
}

function PassPopup(val) {
  if (Spassword == "" || Spassword == null || val == 1) {
    document.querySelector(".popup_windows3").style.display = "block";
  }
}

function checkPass() {
  var pass = document.getElementById("req_password").value;
  if (pass != "" && pass != null) {
    setCookie("secure_pass", pass, 9000);
    Spassword = pass;
    SNDPSS_Flag = true;
    sendCPASS();
  }
  SKIP_Flag = true;
  document.getElementById("PW3").style.display = "none";
  document.getElementById("req_password").value = "";
}

function wrongPass() {
  window.navigator.vibrate([80, 50, 80, 80, 50, 80, 50, 100, 100]);
  var pass = prompt(
    "رمز نادرست! لطفا رمز صحیح را وارد نمایید و دوباره امتحان کنید. :",
    ""
  );
  if (pass != "" && pass != null) {
    setCookie("secure_pass", pass, 9000);
    Spassword = pass;
  }
}
var CRMODE = 1;
function _navShowMode(mode) {
  // document.getElementById("header").style.display='none'
  document.getElementById("modeOneTwo").style.display = "none";
  document.getElementById("modeTree").style.display = "none";
  document.getElementById("modeSetting").style.display = "none";

  document.getElementById("SettingIcon").style.display = "block";
  document.getElementById("HomeIcon").style.display = "none";
  console.log("set mode", mode);
  if (mode == 99) mode = CRMODE;

  switch (mode) {
    case 1:
      document.getElementById("modeOneTwo").style.display = "flex";
      CRMODE = mode;
      break;
    case 2:
      document.getElementById("modeOneTwo").style.display = "flex";
      CRMODE = mode;
      break;
    case 3:
      document.getElementById("modeTree").style.display = "flex";
      CRMODE = mode;
      break;
    case 4:
      document.getElementById("modeSetting").style.display = "flex";
      document.getElementById("SettingIcon").style.display = "none";
      document.getElementById("HomeIcon").style.display = "block";
      break;
  }
}
function _navShowModeCard() {
  document.getElementById("warningCard").style.display = "none";
  document.getElementById("ModeCard").style.display = "flex";
}










