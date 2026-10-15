
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

long _UploadedSizeBuffer = 5;
long _totalSizeBuffer = 0;
class WebApp
{
public:
  WebApp(int httpPort, int socketPort, bool OTAuploader, String SSID = "clickMe", String PASS = "");
  void wStart();
  void wStop();
  void Loop();
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
    newSTR = 60,
  };

  bool _isOTA = false;
  bool _forceHandShake = false;
  bool _forceSendtemp = false;

  uint32_t __lastHandShake;
  uint8_t _currentClient;
  String _SSID;
  String _PASS;
  bool _enableOTAUploader;
  File fsUploadFile; // a File variable to temporarily store the received file

  bool handleHttp(String path);
  String _getContentType(String filename);
  void _handleFileUpload();
  void _startSPIFFS();
  void _dumpSPIFFDirectory(fs::FS &fs, const char *dirname);

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
      _apIP(172, 217, 28, 1)
{
  this->_SSID = SSID;
  this->_PASS = PASS;
  this->_enableOTAUploader = OTAuploader;
}

void WebApp::handleWebSocket(uint8_t *payload)
{
  DynamicJsonDocument root(5200);
  deserializeJson(root, payload);
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
    lan_ = root["lan"];
    update_data = true;
    save_var();
    _sendRQ(RQ_DONE);
    reqHandShake();
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
  };
}
void WebApp::_handShake()
{
  uint64_t SID0 = ESP.getEfuseMac();
  uint32_t SID1 = (uint32_t)((SID0 & 0xFFFF0000) >> 16);
  uint32_t SID2 = (uint32_t)((SID0 & 0x0000FFFF));
  String IDS = (String(SID1) + String(SID2));

  char MSG[700];
  DynamicJsonDocument msg(5200);
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
      {tap_jack_2 * 1000, 0},
      {tap_jack_1 * 1000, 0}};

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

  msg["aut"] = auto_start;
  msg["end"] = finish_time;
  msg["emg"] = stat_stop;
  msg["tLmt"] = val_ok;
  msg["Tcal"] = temp_offset_int;
  msg["pedal"] = pedal_Count;
  msg["js"] = jackSensore_enabled;

  int unit_welding_c = (unit_welding - int(unit_welding)) * 4;

  msg["WM"] = String(int(unit_welding_p)); // مجموع پنجره
  msg["PM"] = String(unit_welding_c);      // مجموع جوش
  if (temp < 1000)
    msg["TM"] = temp_dis_2; // دمای محیط
  if (temp > 1000)
    msg["TM"] = "Error"; // دمای محیط

  msg["Ws"] = String(int(unit_welding)); // مجموع پنجره موقت
  msg["Ps"] = String(unit_welding_c);    // مجموع جوش موقت

  msg["KW"] = lock_en;
  msg["KWT"] = lock_time - calculateDays() < 0 ? 0 : lock_time - calculateDays();

  msg["Wt"] = ver;
  msg["ADT"] = String(year) + "/" + String(month) + "/" + String(day); //__DATE__;
  //////////////////// Cleaner machine //////////////////
  msg["motSelect"] = motor_type;
  msg["devSelect"] = ability;
  msg["bladeSelect"] = blade_mode;
  msg["OTA"] = _isOTA;
  _isOTA = false;
  ////////////////////////////////////
  serializeJson(msg, MSG);
  _webSocket.broadcastTXT(MSG);
}

void WebApp::_sendTemp()
{
  char MSG[700];
  DynamicJsonDocument msg(5200);
  msg["DID"] = 555;
  msg["RQ"] = NEWS;
  msg["T"] = temp_dis_2;
  serializeJson(msg, MSG);
  _webSocket.broadcastTXT(MSG);
}

void WebApp::wStart()
{

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  vTaskDelay(50);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(_apIP, _apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(_SSID, _PASS);

  vTaskDelay(500);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  _server.onNotFound([&]() { // If a POST request is sent to the /edit.html address,
    if (!handleHttp(_server.uri()))
    {
      _server.send(404, "text/plain", "404: File Not Found");
    }
  });

  if (_enableOTAUploader)
  {
    _server.on(
        "/edit.html", HTTP_POST, [&]() { // If a POST request is sent to the /edit.html address,
          _server.send(200, "text/plain", "");
        },
        [&]()
        {
          _handleFileUpload();
        });
    _server.on(
        "/edit", HTTP_POST, [&]() { // If a POST request is sent to the /edit.html address,
          _server.send(200, "text/plain", "");
        },
        [&]()
        {
          _handleFileUpload();
        });

    _server.on("/edit.html", [&]() { // send constant data in case ESPIFF dosn't be there.
      _server.send(200, "text/html", UPLOADER_HTML_CODE);
    });

    _server.on("/jquery.min.js", HTTP_GET, [&]() { // PROGEEEM jquery!
      _server.setContentLength(jquery_min_js_v3_2_1_gz_len);
      _server.sendHeader(F("Content-Encoding"), F("gzip"));
      _server.send_P(200, "text/javascript", jquery_min_js_v3_2_1_gz, jquery_min_js_v3_2_1_gz_len);
    });

    _server.on("/hexup", HTTP_GET, [&]() { //? hex Uploader Html
      _server.sendHeader("Connection", "close");
      _server.send(200, "text/html", uploaderIndex);
    });
    /*handling uploading firmware file */
    _server.on(
        "/update", HTTP_POST, [&]() { //?
          _server.sendHeader("Connection", "close");
          _server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
          ESP.restart();
        },
        [&]() { //?
          HTTPUpload &upload = _server.upload();
          if (upload.status == UPLOAD_FILE_START)
          {

            _totalSizeBuffer = long(upload.totalSize);
            //! SmartState::set(MODE_RUN_EASY_UPDATE);
            esp_task_wdt_init(WDT_TIMEOUT, false); // enable panic so ESP32 restarts
            esp_task_wdt_add(NULL);                // add current thread to WDT
            if (!Update.begin(UPDATE_SIZE_UNKNOWN))
            { // start with max available size
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_WRITE)
          {
            /* flashing firmware to ESP*/
            _UploadedSizeBuffer++;
            long _progress = map(_UploadedSizeBuffer, 0, 660, 0, 100);
            if (_progress > 100)
              _progress = 100;

            if (_progress <= 1)
            {
              for (int i = 0; i < NUMPIXELS; i++)
              {
                pixels.setPixelColor(i, COLOR_RGB[0]);
              }
              pixels.show();
              sement_off();
            }
            up_seg(U_, P_, L_, O_, E_E, _progress / 100, _progress % 10, 7);
            for (int i = 10; i >= ((100 - _progress) / 8.4); i--)
            {
              pixels.setPixelColor(i, COLOR_RGB[5]);
            }
            pixels.show();

            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            {
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_END)
          {

            if (Update.end(true))
            { // true to set the size to the current progress
            }
            else
            {
              Update.printError(Serial);
            }
          }
        });

    _startSPIFFS();

    _webSocket.begin();           // start the websocket server
    _webSocket.onEvent(_wsEvent); // if there's an incomming websocket message, go to function 'webSocketEvent'

    _server.begin();
  }
}

void WebApp::wStop()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  _server.stop();
  _webSocket.close();
}

uint32_t wifiTimer = millis();
void WebApp::Loop()
{

  _server.handleClient();
  dnsServer.processNextRequest();
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
    rest_pin_out();
    buzzer_warning(2);
  }
  else if (type == WStype_CONNECTED)
  {
    reqHandShake();
    vTaskDelay(100);
    Serial.printf("[%u] Connected!\n", num);
    buzzer_warning(1);
  }
  else if (type == WStype_TEXT)
  {
    _currentClient = num;

    handleWebSocket(payload);
  }
}

bool WebApp::handleHttp(String path)
{
  if (path.endsWith("/"))
    path += "index.html";                     //? If a folder is requested, send the index file
  String contentType = _getContentType(path); //? Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) //? If the file exists, either as a compressed archive, or normal
  {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
  }
  else
  {
    path = "/index.html";
    contentType = _getContentType(path); //? Get the MIME type
    pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
  }

  File file = SPIFFS.open(path, "r");                  //? Open the file
  size_t sent = _server.streamFile(file, contentType); //? Send it to the client
  file.close();
  return true;
}

String WebApp::_getContentType(String filename)
{ // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
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
}

void WebApp::_startSPIFFS()
{
  if (SPIFFS.begin(true))
  {

#ifdef DEBUG
    _dumpSPIFFDirectory(SPIFFS, "/");
#endif
  }
  else
  {
    vTaskDelay(50);
    ESP.restart();
    vTaskDelay(50);
  }
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
  char MSG[500];

  DynamicJsonDocument msg(1500);
  msg["DID"] = 555;
  msg["RQ"] = _RQ;
  serializeJson(msg, MSG);
  // _webSocket.broadcastTXT(MSG);
  _webSocket.sendTXT(_currentClient, MSG);
}
void WebApp::sendStr(String _str)
{
  char MSG[500];

  DynamicJsonDocument msg(1500);
  msg["DID"] = 555;
  msg["RQ"] = newSTR;
  msg["str"] = _str;
  serializeJson(msg, MSG);
  _webSocket.broadcastTXT(MSG);
  // _webSocket.sendTXT(_currentClient, MSG);
}
