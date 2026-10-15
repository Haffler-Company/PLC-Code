
/*
[][][[]][[][][][][][][][][][]] WEB APP LIBRARY V3.6.0 By SOHEIL ASAMI []][][][[]][][[]][[]][][][][][]][[]][[][]]


  TODO-1--> You have to edit enum -> RQtype to define your Requset Types you want to handle
  TODO-2--> You have to edit function->WebApp::handleWebSocket to specify with RQNumber have to do What!
  TODO-3--> Main HTML5 APP is in ./Data/index.html if you dont know HTML5 you can call me for a custom App

initial:
  WebApp(int httpPort, int socketPort, bool OTAuploader, String SSID = "clickMe", String PASS = "");

  Start():
          1- starts an Software AccessPoint with given SSID name
          2- starts an Captive-DNS-portal it helps webAPP to start Automaticaly no need to enter IP manually
          ---//? NOTE-1  --> LOCAL IP IS http://172.217.28.1 and unChangable because of Captive-DNS-portal rulles!
          3- starts an http server that only passes the index.html on SPIFFF on every request in every path you Call from Browser
          4- starts an infinit RealTime webSocket server to handle requests that will be send using javaScript functions inside index.html
          ---//? NOTE-2  --> webSocket only accept JSON payloads like bellow see TODO-2 for more
             //*         --> { RQ:<String>, arg1:<type>,....}
          5- if OTAuploader is you can upload index.html file using adrress: http://172.217.28.1/edit.html

  WebApp::Stop():
          it Will turn off WIFI + all Callbacks will be removed, you can call Start()/Stop() functions anyTime you want to Enable/Disable webApp


  WebApp::Loop():
          You Should call Loop in your while(1);
          ---//? NOTE-3  --> Your Main Lopp should not Last more than 500ms.

  WebApp::sendStr(String _str)
              TODO WebApp::isStrAvilable()
              TODO WebApp::readStr(String _str)
        --//? WebSerial plugins


Reset_error_via_web == 1
  !NOTE --> this is not a free Code if you Read this you probably paid me someThing :)
  !     --> I'll be Glad to recive your suggestions in case of improve it for further updates
  XxXx if you need more information About this Library please contact soheilasami@gmail.com XxXx

[]][][][[][][][][]][[][][][][][][][]][[][][][]  SOHEILASAMI@GMAIL.COM [][][][][][][][][][]][][[][][][][]][[]][[][]][]][[]]
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <DNSServer.h>
#include <Arduino.h>
#include <Update.h>
#include <WebSocketsServer.h>
#include "webapp_plc_sync.h"
#include "ota_pre_erase.h"
#include "storage.h"

extern Storage storage;
extern TaskHandle_t buzzerTaskHandle;
extern "C" int fw_info_get_digital_in(int idx);
extern "C" int fw_info_get_digital_out(int idx);

long _UploadedSizeBuffer = 5;
long _totalSizeBuffer = 0;
class WebApp
{
public:
  WebApp(int httpPort, int socketPort, bool OTAuploader, String SSID = "clickMe", String PASS = "");
  bool wStart();              // true = SoftAP واقعاً بالا آمد
  void wStop();
  void wStopAp();  // خاموش کردن AP/سرور بدون WiFi.mode(OFF) — برای اسکن STA
  void Loop();
  bool isApUp() const
  {
    wifi_mode_t m = WiFi.getMode();
    return (m == WIFI_AP || m == WIFI_AP_STA) &&
           WiFi.softAPIP() != IPAddress(0, 0, 0, 0);
  }
  void reqHandShake()
  {
    _forceHandShake = true;
  };
  void reqTemp()
  {
    _forceSendtemp = true;
  };
  void sendStr(String _str);

protected:
  WebServer _server;           // create a web server on port 80
  WebSocketsServer _webSocket; // create a websocket server on port 81

  IPAddress _apIP;
  const byte DNS_PORT = 53;
  DNSServer dnsServer;
  void _handShake();
  void _sendTemp();
  void handleWebSocket(uint8_t *payload);

private:
  enum RQtype
  {

    HAND_SHAKE = 1,
    SET_PROG = 2,
    SET_LANG = 3,
    RESET_COUNT = 4,

    UPD_PROG = 5,
    UPD_JACK = 6,

    UPD_KSWCH = 7,
    CHANGE_PASS = 8,
    RESET_FACTORY = 9,

    CONFIG = 10,

    NEWS = 11,
    AUTH = 12,
    WRNG_ADMIN_PASS = 13,

    RQ_DONE = 14,
    LOGINRPT = 15,
    DUTRPT = 16,
    SETTIME = 17,
    CNF_DVMODE = 32,
    CNF_JKMODE = 33,
    CNF_EMGNCYMODE = 34,
    CNF_JSENSOR = 36,

    START_JACK = 20,
    STOP_JACK = 21,

    RESET_COUNT_ALL = 30,
    SAVE_DEFS = 31,
    OTA_MODE = 35,
    TEST_BUZZER = 25,
    WIFI_SCAN = 50,
    WIFI_CONNECT = 51,
    newSTR = 60,
  };

  bool _isOTA = false;
  bool _forceHandShake = false;
  bool _forceSendtemp = false;
  bool _httpRoutesMounted = false; // جلوی ثبتِ تکراریِ روت بعد از wStop/wStart

  uint32_t __lastHandShake;
  uint8_t _currentClient;
  String _SSID;
  String _PASS;
  bool _enableOTAUploader;
  String _captiveIp; // FIX-CP-1: member نه local — از dangling reference در lambda جلوگیری می‌کند
  File fsUploadFile; // a File variable to temporarily store the received file

  bool handleHttp(String path);
  String _getContentType(String filename);
  void _handleFileUpload();
  void _startSPIFFS();
  void _dumpSPIFFDirectory(fs::FS &fs, const char *dirname);
  void _serveCaptivePortal(); // FIX-CP-3: متد عضو برای serve کردن captive portal — نه local lambda با dangling ref

  void _sendRQ(RQtype _RQ);

  bool _checkPassword(String input)
  {

    if (input == "admin")
      return (true);
    if (input == "")
      return (false);
    return (false);
  }

  void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght);
  std::function<void(uint8_t num, WStype_t type, uint8_t *payload, size_t length)> _wsEvent = [&](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
  {
    this->webSocketEvent(num, type, payload, length);
  };
};

WebApp ::WebApp(int httpPort, int socketPort, bool OTAuploader, String SSID, String PASS)
    : _server(httpPort),
      _webSocket(socketPort),
      _apIP(192, 168, 4, 1)
{
  this->_SSID = SSID;
  this->_PASS = PASS;
  this->_enableOTAUploader = OTAuploader;
}

void WebApp::handleWebSocket(uint8_t *payload)
{
  // B4-FIX: DynamicJsonDocument(5200) روی استک 12KB => 43% stack در یک فریم → ریسک overflow
  // راهکار: StaticJsonDocument روی BSS (data segment) — webapp_task تک‌ترد است (Core 0)
  // پس static ایمن است؛ ArduinoJson v6 از custom void* allocator پشتیبانی نمی‌کند
  static StaticJsonDocument<5200> root;
  root.clear();
  DeserializationError err = deserializeJson(root, payload);
  if (err || !root.containsKey("RQ")) return;
  RQtype rq = root["RQ"];
  switch (rq)
  {
  case HAND_SHAKE:
  {
    reqHandShake();
  }
  break;
  case CONFIG:
  {
    Config config;
    config.PID = root["PID"];
    config.tempCalib = root["Tcal"];

    config.spk = root["spk"];
    config.spkVol = root.containsKey("spkVol") ? (uint8_t)root["spkVol"] : storage.getSpeakerVolume();
    config.aut = root["aut"];
    config.end = root["end"];
    config.emg = root["emg"];
    config.tLmt = root["tLmt"];
    config.pedalCount = root["pedal"];
    onUpdateConfig(config);
    _sendRQ(RQ_DONE);
    reqHandShake();
  }
  break;
  case RESET_COUNT:
  {
    onResetCounter();
    _sendRQ(RQ_DONE);
    reqHandShake();
  }
  break;
  case RESET_FACTORY:
  {
    onRessetFactory();
    _sendRQ(RQ_DONE);
    reqHandShake();
  }
  break;
  case SET_PROG:
  {
    setCurrentProgramById(root["ID"]);
    _sendRQ(RQ_DONE);
    reqHandShake();
  }
  break;
  case UPD_PROG:
  {
    Program program;
    program.id = static_cast<PROGRAMID>(root["ID"]);
    program.high = root["h"];
    program.low = root["l"];
    program.temp = root["t"];
    onUpdateProgram(program);
    _sendRQ(RQ_DONE);
    reqHandShake();
    break;
  }
  case UPD_JACK:
  {
    Jack jack;
    jack.id = static_cast<JACKID>(root["ID"]);
    jack.load = root["l"];
    jack.unload = root["ul"];
    onUpdateJack(jack);
    _sendRQ(RQ_DONE);
    reqHandShake();
  }
  break;
  case START_JACK:
  {
    onStartJack(root["ID"]);
    _sendRQ(RQ_DONE);
  }
  break;
  case STOP_JACK:
  {
    onStopJack();
    _sendRQ(RQ_DONE);
  }
  break;
  case SET_LANG:
  {
    // Web: FA=1 EN=2 TU=3 RU=4
    int lan = root.containsKey("lan") ? (int)root["lan"] : (int)root["L"];
    if (lan < 1) lan = 1;
    if (lan > 4) lan = 4;
    lan_ = lan;
    update_data = true;
  }
  break;

  case AUTH:
  {
    if (checkPassword(root["psw"]))
    {
      _sendRQ(AUTH);
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case UPD_KSWCH:
  {
    if (checkPassword(root["psw"]))
    {
      updateKillSwitch(root["isOn"], root["days"]);
      reqHandShake();
      _sendRQ(RQ_DONE);
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case CHANGE_PASS:
  {
    if (checkPassword(root["psw"]))
    {
      onChangePassword(root["newPass"]);
      reqHandShake();
      _sendRQ(RQ_DONE);
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case RESET_COUNT_ALL:
  {
    if (checkPassword(root["psw"]))
    {
      onResetCounterAll();
      reqHandShake();
      _sendRQ(RQ_DONE);
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case SAVE_DEFS:
  {
    if (checkPassword(root["psw"]))
    {
      onsaveDefaults();
      reqHandShake();
      _sendRQ(SAVE_DEFS);
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case CNF_DVMODE:
  {
    if (checkPassword(root["psw"]))
    {
      Mode dvMode = root["dvMode"];
      onUpdateDVmode(dvMode);
      _sendRQ(RQ_DONE);
      reqHandShake();
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case CNF_JKMODE:
  {
    if (checkPassword(root["psw"]))
    {
      bool isJ = root["isJ"];
      onUpdateJKmode(isJ);
      _sendRQ(RQ_DONE);
      reqHandShake();
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case CNF_EMGNCYMODE:
  {
    if (checkPassword(root["psw"]))
    {
      bool emg = root["emg"];
      onUpdateEmgncymode(emg);
      _sendRQ(RQ_DONE);
      reqHandShake();
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case CNF_JSENSOR:
  {
    if (checkPassword(root["psw"]))
    {
      bool en = root["js"];
      onUpdateJackSensore(en);
      _sendRQ(RQ_DONE);
      reqHandShake();
    }
    else
      _sendRQ(WRNG_ADMIN_PASS);
  }
  break;
  case OTA_MODE:
  {
    _isOTA = true;
  }
  break;
  case TEST_BUZZER:
  {
    if (root.containsKey("vol")) {
      uint8_t v = (uint8_t)root["vol"];
      if (v > 100) v = 100;
      storage.setSpeakerVolume(v);
    }
    if (buzzerTaskHandle) {
      xTaskNotify(buzzerTaskHandle, BUZZER_ACTION, eSetValueWithOverwrite);
    }
  }
  break;
  case WIFI_SCAN:
  {
    int n = WiFi.scanNetworks(false, false);
    DynamicJsonDocument doc(2048);
    doc["RQ"] = WIFI_SCAN;
    JsonArray arr = doc.createNestedArray("networks");
    for (int i = 0; i < n && i < 15; i++) {
      JsonObject net = arr.createNestedObject();
      net["ssid"] = WiFi.SSID(i);
      net["rssi"] = WiFi.RSSI(i);
      net["enc"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    String out;
    serializeJson(doc, out);
    _webSocket.sendTXT(_currentClient, out);
  }
  break;
  case WIFI_CONNECT:
  {
    String ssid = root["ssid"].as<String>();
    String pass = root.containsKey("pass") ? root["pass"].as<String>() : "";
    if (ssid.length() > 0) {
      storage.setWifiCredentials(ssid, pass);
      storage.saveWifi(ssid, pass);
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(ssid.c_str(), pass.c_str());
      _sendRQ(RQ_DONE);
      reqHandShake();
    }
  }
  break;
  };
}
void WebApp::_handShake()
{
  webapp_sync_vars_from_plc();
  uint64_t SID0 = ESP.getEfuseMac();
  uint32_t SID1 = (uint32_t)((SID0 & 0xFFFF0000) >> 16);
  uint32_t SID2 = (uint32_t)((SID0 & 0x0000FFFF));
  String IDS = (String(SID1) + String(SID2));

  static StaticJsonDocument<5200> msg;
  msg.clear();
  msg["DID"] = 555;
  msg["ID"] = IDS;
  msg["RQ"] = HAND_SHAKE;
  msg["mode"] = 1;
  msg["dvMode"] = device_mode;
  msg["lan"] = lan_;

  JsonArray programData = msg.createNestedArray("prg");
  for (int i = 1; i <= 5; i++) // Iterating through members 1 to 5
  {
    JsonObject data = programData.createNestedObject();
    data["id"] = i;
    data["h"] = e_up[i];
    data["l"] = s_up[i];
    data["t"] = c_up[i];
  }

  JsonArray jks = msg.createNestedArray("jks");

  int jackData[6][2] = {
      {table_ready, table_back},
      {ax_ready, ax_back},
      {press_ready, press_back},
      {heater_ready, heater_back},
      {tap_jack_1 * 1000, 0},
      {tap_jack_2 * 1000, 0}};

  for (int i = 0; i < 6; i++)
  {
    JsonObject jack = jks.createNestedObject();
    jack["id"] = i;
    jack["l"] = jackData[i][0];
    jack["ul"] = jackData[i][1];
  }

  msg["PU"] = program_no;
  msg["H"] = e_up[program_no];
  msg["T"] = c_up[program_no];
  msg["L"] = s_up[program_no];

  msg["PID"] = PID;
  msg["isJ"] = isJ;
  msg["spk"] = buzzer_en;
  msg["spkVol"] = storage.getSpeakerVolume();
  msg["wifi_sta"] = (WiFi.status() == WL_CONNECTED);
  msg["wifi_ssid"] = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : storage.getWifiSsid();
  msg["wifi_ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
  msg["wifi_rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

  msg["aut"] = auto_start;
  msg["end"] = finish_time;
  msg["emg"] = stat_stop;
  msg["tLmt"] = val_ok;
  msg["Tcal"] = temp_offset_int;
  msg["pedal"] = pedal_Count;
  msg["js"] = jackSensore_enabled;

  msg["WM"] = String((int)get_var_total_frame_numbers()); // مجموع چهارچوب کل دستگاه
  msg["PM"] = String((int)get_var_frame_numbers());       // مجموع جوش شیفت فعلی
  int cur_elem_t = (int)get_var_element_temperature();
  if (cur_elem_t < 0 || cur_elem_t > 800) cur_elem_t = 0;
  msg["TM"] = cur_elem_t; // دمای واقعی اتو / المنت

  msg["Ws"] = String((int)get_var_total_frame_numbers());
  msg["Ps"] = String((int)get_var_frame_numbers());

  msg["KW"] = lock_en;
  msg["KWT"] = lock_time  < 0 ? 0 : lock_time;

  msg["Wt"] = ver;
  msg["ADT"] = String(year) + "/" + String(month) + "/" + String(day); //__DATE__;
  //////////////////// Cleaner machine //////////////////
  msg["motSelect"] = motor_type;
  msg["devSelect"] = ability;
  msg["bladeSelect"] = blade_mode;
  msg["OTA"] = _isOTA;
  _isOTA = false;

  msg["SSR"] = (digitalRead(SSR_1) == ON) ? 1 : 0;
  msg["Q_PRS"] = (digitalRead(PRESS_LEFT) == ON || digitalRead(PRESS_RIGHT) == ON) ? 1 : 0;
  msg["Q_TAB"] = (digitalRead(TABLE) == ON) ? 1 : 0;
  msg["Q_ELM"] = (digitalRead(ELEMENT) == ON) ? 1 : 0;
  msg["Q_SHB"] = (digitalRead(SHABLON) == ON) ? 1 : 0;
  msg["EMG"] = (digitalRead(EMERGENCY) == ON) ? 1 : 0;
  msg["PED"] = (digitalRead(PEDAL) == ON) ? 1 : 0;

  uint8_t in_bits = 0;
  uint8_t out_bits = 0;
  for (int i = 0; i < 8; i++) {
    if (fw_info_get_digital_in(i)) in_bits |= (1 << i);
    if (fw_info_get_digital_out(i)) out_bits |= (1 << i);
  }
  msg["IN_BITS"] = in_bits;
  msg["OUT_BITS"] = out_bits;
  ////////////////////////////////////
  // مهم: قبلاً char MSG[700] بود؛ JSON واقعی اغلب >700 بایت است → truncate
  // → JSON.parse در گوشی می‌ترکد → closeLoading اجرا نمی‌شود → صفحه سفید/لودینگ.
  String MSG;
  serializeJson(msg, MSG);
  Serial.printf("[WIFI] HAND_SHAKE bytes=%u\n", (unsigned)MSG.length());
  _webSocket.broadcastTXT(MSG);
}

void WebApp::_sendTemp()
{
  StaticJsonDocument<512> msg;
  msg["DID"] = 555;
  msg["RQ"] = NEWS;
  int cur_elem_t = (int)get_var_element_temperature();
  if (cur_elem_t < 0 || cur_elem_t > 800) cur_elem_t = 0;
  msg["T"] = cur_elem_t;

  // Live frame/weld counts synchronized from NVS and vars
  msg["WM"] = (int)get_var_total_frame_numbers();
  msg["PM"] = (int)get_var_frame_numbers();

  msg["SSR"] = (digitalRead(SSR_1) == ON) ? 1 : 0;
  msg["Q_PRS"] = (digitalRead(PRESS_LEFT) == ON || digitalRead(PRESS_RIGHT) == ON) ? 1 : 0;
  msg["Q_TAB"] = (digitalRead(TABLE) == ON) ? 1 : 0;
  msg["Q_ELM"] = (digitalRead(ELEMENT) == ON) ? 1 : 0;
  msg["Q_SHB"] = (digitalRead(SHABLON) == ON) ? 1 : 0;
  msg["EMG"] = (digitalRead(EMERGENCY) == ON) ? 1 : 0;
  msg["PED"] = (digitalRead(PEDAL) == ON) ? 1 : 0;

  uint8_t in_bits = 0;
  uint8_t out_bits = 0;
  for (int i = 0; i < 8; i++) {
    if (fw_info_get_digital_in(i)) in_bits |= (1 << i);
    if (fw_info_get_digital_out(i)) out_bits |= (1 << i);
  }
  msg["IN_BITS"] = in_bits;
  msg["OUT_BITS"] = out_bits;

  char MSG[384];
  size_t len = serializeJson(msg, MSG, sizeof(MSG));
  _webSocket.broadcastTXT(MSG, len);
}

bool WebApp::wStart()
{
  // توقف تمیز رادیو — ۵۰ms برای ESP32-S3 کافی نیست و softAP گاهی بی‌صدا fail می‌شود.
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  vTaskDelay(pdMS_TO_TICKS(250));

  // فقط AP: برای وب‌اپ STA لازم نیست؛ AP+STA بعضی بوردها Beacon را ضعیف/پنهان می‌کند.
  WiFi.mode(WIFI_AP);
  vTaskDelay(pdMS_TO_TICKS(50));
  WiFi.softAPConfig(_apIP, _apIP, IPAddress(255, 255, 255, 0));

  // channel=1, نه مخفی، تا ۴ کلاینت — خروجی softAP را چک کن.
  bool ok = WiFi.softAP(_SSID.c_str(), _PASS.c_str(), 1, 0, 4);
  if (!ok) {
    Serial.printf("[WIFI] softAP() failed ssid='%s' — retry WIFI_AP_STA\n", _SSID.c_str());
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(200));
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(_apIP, _apIP, IPAddress(255, 255, 255, 0));
    ok = WiFi.softAP(_SSID.c_str(), _PASS.c_str(), 1, 0, 4);
  }

  // FIX-WEBAPP-1: حلقه انتظار تا IP معتبر — 400ms ثابت کافی نیست روی بعضی بوردها
  IPAddress ip(0, 0, 0, 0);
  for (int _wi = 0; _wi < 40 && ip == IPAddress(0, 0, 0, 0); _wi++) {
    vTaskDelay(pdMS_TO_TICKS(50));
    ip = WiFi.softAPIP();
  }
  if (!ok || ip == IPAddress(0, 0, 0, 0)) {
    Serial.printf("[WIFI] AP START FAIL ok=%d ip=%s mode=%d\n",
                  (int)ok, ip.toString().c_str(), (int)WiFi.getMode());
    return false;
  }

  dnsServer.stop();
  dnsServer.start(DNS_PORT, "*", ip);

  // FIX-CP-1: IP را در member ذخیره کن تا همیشه به‌روز باشد (حتی در ری‌استارت‌های AP)
  _captiveIp = ip.toString();

  // روت‌ها فقط یک‌بار؛ wStart دوباره باعث دو پاسخ/صفحه سفید می‌شد.
  if (!_httpRoutesMounted) {
    const char *headerkeys[] = {"Accept-Encoding", "User-Agent", "Host"};
    _server.collectHeaders(headerkeys, 3);

    // FIX-CP-3: به جای local lambda با body بزرگ، متد عضو را فراخوانی می‌کنیم
    auto captivePortalPage = [this]() { _serveCaptivePortal(); };

    auto captiveIndex = [this]() {
      // FIX-HOST-REDIRECT: اگر درخواست با نام دامنه آمده (مانند google.com یا probe)، مرورگر را با 302 به آدرس واقعی IP هدایت کن
      String host = _server.hostHeader();
      String hostLower = host;
      hostLower.toLowerCase();
      if (hostLower.length() > 0 && !hostLower.startsWith(_captiveIp) && hostLower != "192.168.4.1" && !hostLower.startsWith("192.168.4.1:") && hostLower != "172.217.28.1" && !hostLower.startsWith("172.217.28.1:") && !hostLower.startsWith("haffler.local")) {
        String redirectUrl = String("http://") + _captiveIp + "/";
        _server.sendHeader(F("Location"), redirectUrl);
        _server.send(302, "text/plain", "");
        return;
      }
      handleHttp("/index.html");
    };

    _server.onNotFound([this]() {
      String uri = _server.uri();
      // اگر فایل در SPIFFS باشد مستقیماً با متد handleHttp سرو کن
      if (handleHttp(uri)) {
        return;
      }
      // اگر فایل دارای پسوند باشد (.js, .css, .png, ...) و یافت نشود، 404 بده (هرگز ریدایرکت نکن تا لوپ نشود)
      if (uri.indexOf('.') >= 0) {
        _server.send(404, "text/plain", "404: Not Found");
        return;
      }
      // برای درخواست‌های آدرس وب ناشناس، صفحه اتصال خودکار کپتیو پورتال را ارسال کن
      _serveCaptivePortal();
    });

    // پروب‌های کپتیو پورتال سیستم‌های عامل (Android, iOS, Windows)
    _server.on("/generate_204", HTTP_GET, captivePortalPage);
    _server.on("/gen_204", HTTP_GET, captivePortalPage);
    _server.on("/connecttest.txt", HTTP_GET, captivePortalPage);
    _server.on("/ncsi.txt", HTTP_GET, captivePortalPage);
    _server.on("/canonical.html", HTTP_GET, captivePortalPage);
    _server.on("/success.txt", HTTP_GET, captivePortalPage);
    _server.on("/hotspot-detect.html", HTTP_GET, captivePortalPage);
    _server.on("/library/test/success.html", HTTP_GET, captivePortalPage);
    _server.on("/redirect", HTTP_GET, captivePortalPage);
    _server.on("/fwlink", HTTP_GET, captivePortalPage);
    _server.on("/", HTTP_GET, captiveIndex);
    _server.on("/index.html", HTTP_GET, captiveIndex);
    _server.on("/chat", HTTP_GET, [this]() { handleHttp("/chat.html"); });
    _server.on("/chat.html", HTTP_GET, [this]() { handleHttp("/chat.html"); });

    _server.on(
        "/edit.html", HTTP_POST, [this]() {
          _server.send(200, "text/plain", "");
        },
        [this]()
        {
          _handleFileUpload();
        });
    _server.on(
        "/edit", HTTP_POST, [this]() {
          _server.send(200, "text/plain", "");
        },
        [this]()
        {
          _handleFileUpload();
        });

    _server.on("/edit.html", [this]() {
      _server.send(200, "text/html", UPLOADER_HTML_CODE);
    });

    _server.on("/jquery.min.js", HTTP_GET, [this]() {
      _server.setContentLength(jquery_min_js_v3_2_1_gz_len);
      _server.sendHeader(F("Content-Encoding"), F("gzip"));
      _server.send_P(200, "text/javascript", jquery_min_js_v3_2_1_gz, jquery_min_js_v3_2_1_gz_len);
    });

    _server.on("/api/wifi_scan", HTTP_GET, [this]() {
      int n = WiFi.scanNetworks(false, false);
      DynamicJsonDocument doc(2048);
      doc["status"] = "ok";
      JsonArray arr = doc.createNestedArray("networks");
      for (int i = 0; i < n && i < 15; i++) {
        JsonObject net = arr.createNestedObject();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["enc"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      }
      String out;
      serializeJson(doc, out);
      _server.sendHeader("Access-Control-Allow-Origin", "*");
      _server.send(200, "application/json", out);
    });

    _server.on("/api/wifi_connect", HTTP_POST, [this]() {
      if (_server.hasArg("plain")) {
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, _server.arg("plain"));
        if (!err && doc.containsKey("ssid")) {
          String ssid = doc["ssid"].as<String>();
          String pass = doc.containsKey("pass") ? doc["pass"].as<String>() : "";
          storage.setWifiCredentials(ssid, pass);
          storage.saveWifi(ssid, pass);
          WiFi.mode(WIFI_AP_STA);
          WiFi.begin(ssid.c_str(), pass.c_str());
          _server.sendHeader("Access-Control-Allow-Origin", "*");
          _server.send(200, "application/json", "{\"status\":\"connecting\"}");
          return;
        }
      }
      _server.sendHeader("Access-Control-Allow-Origin", "*");
      _server.send(400, "application/json", "{\"status\":\"error\"}");
    });

    _server.on("/api/cmd", HTTP_POST, [this]() {
      if (_server.hasArg("plain")) {
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, _server.arg("plain"));
        if (!err) {
          String act = doc["action"].as<String>();
          if (act == "buzzer_test") {
            if (doc.containsKey("vol")) {
              uint8_t v = (uint8_t)doc["vol"];
              if (v > 100) v = 100;
              storage.setSpeakerVolume(v);
            }
            if (buzzerTaskHandle) {
              xTaskNotify(buzzerTaskHandle, BUZZER_ACTION, eSetValueWithOverwrite);
            }
            _server.sendHeader("Access-Control-Allow-Origin", "*");
            _server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"بوق با موفقیت تست شد\"}");
            return;
          }
        }
      }
      _server.sendHeader("Access-Control-Allow-Origin", "*");
      _server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    _server.on("/hexup", HTTP_GET, [this]() {
      _server.sendHeader("Connection", "close");
      _server.send(200, "text/html", uploaderIndex);
    });
    _server.on(
        "/update", HTTP_POST, [this]() {
          _server.sendHeader("Connection", "close");
          _server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
          delay(300);
          ESP.restart();
        },
        [this]() {
          HTTPUpload &upload = _server.upload();
          if (upload.status == UPLOAD_FILE_START)
          {
            _totalSizeBuffer = long(upload.totalSize);
            ota_pre_erase_ensure_clean_blocking();
            esp_task_wdt_init(WDT_TIMEOUT, false);
            esp_task_wdt_add(NULL);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN))
            {
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_WRITE)
          {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            {
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_END)
          {
            if (Update.end(true))
            {
            }
            else
            {
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_ABORTED)
          {
            Update.end();
            Serial.println("[WIFI] Firmware update aborted");
          }
        });
    _httpRoutesMounted = true;
  }

    _startSPIFFS();

    _webSocket.begin();           // start the websocket server
    _webSocket.onEvent(_wsEvent); // if there's an incomming websocket message, go to function 'webSocketEvent'

    _server.begin();
    if (MDNS.begin("haffler")) {
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("ws", "tcp", 81);
    }

    // مورد ۴ (تشخیصِ هنگِ وای‌فای): لاگِ وضعیتِ AP و حافظه هنگام استارت.
    Serial.printf("[WIFI] AP started SSID='%s' IP=%s ch=1 mode=%d heap=%u\n",
                  _SSID.c_str(), ip.toString().c_str(), (int)WiFi.getMode(),
                  (unsigned)ESP.getFreeHeap());
    return true;
}

void WebApp::wStop()
{
  MDNS.end();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  _server.stop();
  _webSocket.close();
}

void WebApp::wStopAp()
{
  MDNS.end();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  _server.stop();
  _webSocket.close();
}

void WebApp::Loop()
{
  dnsServer.processNextRequest();
  _server.handleClient();
  _webSocket.loop();
  if (_forceHandShake) //? one at a time
  {
    _forceHandShake = false;
    _handShake();
  }
  if (_forceSendtemp)
  {
    _forceSendtemp = false;
    _sendTemp();
  }

  // تله‌متری دوره‌ای زنده‌سازی وب‌اپ: نرخ 1.25Hz (هر 800 میلی‌ثانیه)
  static uint32_t lastTelemetryMs = 0;
  if (millis() - lastTelemetryMs >= 800)
  {
    lastTelemetryMs = millis();
    if (_webSocket.connectedClients() > 0)
    {
      _sendTemp();
    }
  }

  //  if ((millis() - wifiTimer > 3000)&&(on_wi_fi == true))
  //  {
  //    wifiTimer = millis();
  //    Serial.println("WIFI IS ON");
  //  }
}

void WebApp::webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{ // When a WebSocket message is received

  if (type == WStype_DISCONNECTED)
  {
    Serial.printf("[%u] Disconnected!\n", num);
    // rest_pin_out();
    // buzzer_warning(2);
  }
  else if (type == WStype_CONNECTED)
  {
    reqHandShake();
    // B7-FIX: vTaskDelay(100) حذف شد — این تأخیر کل ترد webapp_task را فریز می‌کرد
    Serial.printf("[%u] Connected!\n", num);
    // buzzer_warning(1);
  }
  else if (type == WStype_TEXT)
  {
    _currentClient = num;

    handleWebSocket(payload);
  }
}

// FIX-CP-3: پیاده‌سازی captive portal به عنوان متد عضو — نه local lambda با dangling reference
// _captiveIp در wStart() به عنوان member ذخیره شده، اینجا معتبر است
void WebApp::_serveCaptivePortal()
{
  String portalUrl = String("http://") + _captiveIp + "/";
  _server.sendHeader(F("Location"), portalUrl);
  _server.sendHeader(F("X-Captive-Portal"), F("Haffler"));
  _server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  _server.sendHeader(F("Pragma"), F("no-cache"));
  _server.sendHeader(F("Expires"), F("0"));
  _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));

  String html =
    String("<!doctype html><html><head><meta charset=utf-8>")
    + "<meta name=viewport content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=0\">"
    + "<meta http-equiv=refresh content=\"0;url=" + portalUrl + "\">"
    + "<title>Haffler PLC</title>"
    "<style>"
    "*{box-sizing:border-box;margin:0;padding:0}"
    "body{background:#090e17;color:#fff;font-family:sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;height:100dvh;user-select:none;text-align:center;padding:20px;}"
    ".card{background:rgba(255,255,255,0.05);border:1px solid rgba(0,210,180,0.3);border-radius:24px;padding:32px 24px;display:flex;flex-direction:column;align-items:center;max-width:320px;box-shadow:0 12px 40px rgba(0,0,0,0.5);}"
    ".spinner{width:46px;height:46px;border:4px solid rgba(0,210,180,0.15);border-top-color:#00d2b4;border-radius:50%;animation:sp 0.8s linear infinite;margin-bottom:18px;}"
    "@keyframes sp{to{transform:rotate(360deg)}}"
    ".title{color:#00d2b4;font-size:1.15rem;font-weight:bold;margin-bottom:8px;letter-spacing:0.5px;}"
    ".sub{color:#94a3b8;font-size:0.88rem;margin-bottom:20px;}"
    ".btn{background:linear-gradient(135deg,#00d2b4,#009688);color:#fff;text-decoration:none;font-size:0.95rem;font-weight:bold;padding:12px 28px;border-radius:24px;box-shadow:0 4px 16px rgba(0,210,180,0.4);display:inline-block;transition:transform 0.15s;}"
    ".btn:active{transform:scale(0.95);}"
    "</style>"
    + "<script>function go(){window.location.replace('" + portalUrl + "');}go();setTimeout(go,50);setTimeout(go,300);</script>"
    "</head><body>"
    "<div class=card>"
    "<div class=spinner></div>"
    "<div class=title>اتصال به کنترلر هافلر</div>"
    "<div class=sub>در حال بارگذاری داشبورد صنعتی...</div>"
    + "<a href=\"" + portalUrl + "\" class=btn onclick=\"go();return false;\">ورود به صفحه اصلی</a>"
    + "</div></body></html>";

  // FIX-CP-2: 302 Redirect — iOS CNA و Android captive detector نیاز به 3xx دارند نه 200+Location
  _server.send(302, "text/html; charset=UTF-8", html);
}

bool WebApp::handleHttp(String path)
{
  Serial.printf("[WIFI] HTTP req=%s heap=%u\n",
                path.c_str(), (unsigned)ESP.getFreeHeap());
  if (!path.startsWith("/"))
    path = "/" + path;
  if (path.endsWith("/"))
    path += "index.html";
  String contentType = _getContentType(path);
  String pathWithGz = path + ".gz";

  // FIX-LOAD: بررسی Accept-Encoding — ولی اگر فقط gz وجود داشت
  // (فایل اصلی نیست) آن را ارسال کن حتی بدون Accept-Encoding
  bool clientAcceptsGzip = _server.hasHeader("Accept-Encoding") &&
                           _server.header("Accept-Encoding").indexOf("gzip") >= 0;

  bool haveGz   = SPIFFS.exists(pathWithGz);
  bool haveOrig = SPIFFS.exists(path);

  if (haveGz && (clientAcceptsGzip || !haveOrig))
  {
    // ارسال نسخه gz — مرورگر از آن پشتیبانی می‌کند یا نسخه اصلی وجود ندارد
    path = pathWithGz;
  }
  else if (haveOrig)
  {
    // ارسال فایل اصلی بدون فشرده‌سازی
  }
  else if (haveGz)
  {
    // آخرین راه: gz بدون Accept-Encoding — مرورگر مدرن آن را decode می‌کند
    path = pathWithGz;
  }
  else
  {
    if (path == "/index.html") {
      Serial.println("[WIFI] HTTP 503: SPIFFS has no index.html");
      _server.send(503, "text/html",
                   "<!doctype html><meta charset=utf-8><title>SPIFFS empty</title>"
                   "<body style='font-family:sans-serif;padding:2rem'>"
                   "<h2>Web UI files missing</h2>"
                   "<p>SoftAP is up, but SPIFFS has no index.html / jvs.js.</p>"
                   "<p>Flash filesystem: <code>pio run -t uploadfs</code></p>"
                   "</body>");
      return true;
    }
    return false;
  }

  File file = SPIFFS.open(path, "r");
  if (!file) {
    Serial.printf("[WIFI] HTTP open fail %s\n", path.c_str());
    return false;
  }

  // FIX-CACHE: فایل‌های static (js/css/png/ico) را با cache طولانی ارسال کن
  // تا مرورگر موبایل آن‌ها را کش کند و دوباره دانلود نکند (ESP32 کُند می‌شود)
  // index.html و chat.html را بدون cache بده تا همیشه آخرین نسخه باشد
  bool isStaticAsset = path.endsWith(".js.gz") || path.endsWith(".js")   ||
                       path.endsWith(".css.gz") || path.endsWith(".css")  ||
                       path.endsWith(".png")    || path.endsWith(".png.gz")||
                       path.endsWith(".ico")    || path.endsWith(".svg");
  if (isStaticAsset) {
    _server.sendHeader(F("Cache-Control"), F("public, max-age=3600"));
  } else {
    _server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
    _server.sendHeader(F("Pragma"),        F("no-cache"));
    _server.sendHeader(F("Expires"),       F("0"));
  }
  _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  size_t sent = _server.streamFile(file, contentType);
  Serial.printf("[WIFI] HTTP sent %s bytes=%u\n", path.c_str(), (unsigned)sent);
  file.close();
  return true;
}


String WebApp::_getContentType(String filename)
{ // MIME from real extension — .gz assets use Content-Encoding: gzip in handleHttp()
  if (filename.endsWith(".gz"))
    filename.remove(filename.length() - 3);
  if (filename.endsWith(".html") || filename.endsWith(".htm"))
    return "text/html; charset=UTF-8";
  else if (filename.endsWith(".css"))
    return "text/css; charset=UTF-8";
  else if (filename.endsWith(".js"))
    return "application/javascript; charset=UTF-8";
  else if (filename.endsWith(".json"))
    return "application/json; charset=UTF-8";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".svg"))
    return "image/svg+xml; charset=UTF-8";
  return "text/plain; charset=UTF-8";
}
// bool EASYUPDATEMODE = false;
void WebApp::_handleFileUpload() //? upload a new file to the SPIFFS
{

  HTTPUpload &upload = _server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START)
  {

    _totalSizeBuffer = upload.totalSize;
    // SETUP_EASYUPDATE();
    //!  SmartState::set(MODE_RUN_EASY_UPDATE);
    esp_task_wdt_init(WDT_TIMEOUT, false); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);                // add current thread to WDT

    path = upload.filename;
    if (!path.startsWith("/"))
      path = "/" + path;
    if (!path.endsWith(".gz"))
    {                                   //? The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz"; //? So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))    //? version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }

    fsUploadFile = SPIFFS.open(path, "w+"); //? Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {

    if (fsUploadFile)
    {
      fsUploadFile.write(upload.buf, upload.currentSize); //? Write the received bytes to the file
    }
    _UploadedSizeBuffer += upload.currentSize;

    {
      long _progress = map(_UploadedSizeBuffer, 0, _totalSizeBuffer, 0, 100);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {

    if (fsUploadFile)
    { //? If the file was successfully created

      fsUploadFile.close(); //?  Close the file again

      _server.sendHeader("Location", "/success.html"); //?  Redirect the client to the success page
      _server.send(303);
    }
    else
    {

      _server.send(500, "text/plain", "500: couldn't create file");
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    // B5-FIX: بستن file descriptor هنگام قطع ناگهانی کلاینت — جلوگیری از File Handle Leak
    if (fsUploadFile) {
      fsUploadFile.close();
      Serial.println("[WIFI] Upload aborted — file handle closed safely");
    }
  }
}

void WebApp::_startSPIFFS()
{
  // مهم: begin(true) روی mount ناموفق پارتیشن را FORMAT می‌کند → بعد از فلشِ firmware
  // بدون uploadfs، FS خالی می‌ماند و گوشی صفحهٔ سفید می‌بیند. اول بدون فرمت سعی کن.
  bool ok = SPIFFS.begin(false);
  if (!ok) {
    Serial.println("[WIFI] SPIFFS mount fail — format once (will be empty until uploadfs)");
    ok = SPIFFS.begin(true);
  }
  if (!ok) {
    Serial.println("[WIFI] SPIFFS begin failed completely");
    return;
  }

  size_t used = SPIFFS.usedBytes();
  size_t total = SPIFFS.totalBytes();
  bool hasIndex = SPIFFS.exists("/index.html") || SPIFFS.exists("/index.html.gz");
  bool hasJs = SPIFFS.exists("/jvs.js") || SPIFFS.exists("/jvs.js.gz");
  Serial.printf("[WIFI] SPIFFS ok used=%u/%u index=%d jvs=%d\n",
                (unsigned)used, (unsigned)total, (int)hasIndex, (int)hasJs);
  if (!hasIndex || !hasJs) {
    Serial.println("[WIFI] SPIFFS missing web assets → white page until: pio run -t uploadfs");
  }
#ifdef DEBUG
  _dumpSPIFFDirectory(SPIFFS, "/");
#endif
}

void WebApp::_dumpSPIFFDirectory(fs::FS &fs, const char *dirname)
{

  File root = fs.open(dirname);
  if (!root)
  {

    return;
  }
  if (!root.isDirectory())
  {

    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
    }
    else
    {
    }
    file = root.openNextFile();
  }
}

void WebApp::_sendRQ(RQtype _RQ)
{
  DynamicJsonDocument msg(256);
  msg["DID"] = 555;
  msg["RQ"] = _RQ;
  String MSG;
  serializeJson(msg, MSG);
  _webSocket.sendTXT(_currentClient, MSG);
}
void WebApp::sendStr(String _str)
{
  DynamicJsonDocument msg(768);
  msg["DID"] = 555;
  msg["RQ"] = newSTR;
  msg["str"] = _str;
  String MSG;
  serializeJson(msg, MSG);
  _webSocket.broadcastTXT(MSG);
}
