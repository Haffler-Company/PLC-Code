
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
  void sendStr(String _str);

protected:
  WebServer _server;           // create a web server on port 80
  WebSocketsServer _webSocket; // create a websocket server on port 81

  IPAddress _apIP;
  const byte DNS_PORT = 53;
  DNSServer dnsServer;
  void _handShake();
  void handleWebSocket(uint8_t *payload);

private:
  enum RQtype
  {

    HAND_SHAKE = 1,
    IMPORT_SSID = 2,
    RESAULT = 3,
    FLAG_ON = 4,
    WRNG_PASS = 5,
    RQ_DONE = 6,
    MDM_STT = 7,
    CONFIG = 8,
    NEWS = 9,
    CHANGE_PASS = 10,
    WRNG_ADMIN_PASS = 11,
    WIFI_RECONNECT = 12,

    AUTH = 14,
    ADDUSR = 15,
    RMVUSR = 16,
    RMVAUSR = 17,
    A_RNDONE = 18,
    LOGINRPT = 19,
    DUTRPT = 20,
    SETTIME = 21,

    newSTR = 60,
    GETINFO = 247,
    SENDINFO = 248,

  };

  bool _forceHandShake = false;

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
    int _mot = root["mot"];
    int _abil = root["abil"];
    int _blade = root["blade"];
    int _lang = root["lang"];
    if (saveParams(root["eUp"], root["cUp"], root["sUp"], root["PID"], root["isJ"], _mot, _abil, _blade, _lang))
    {
      _sendRQ(CONFIG);
      reqHandShake();
    }
    else
      _sendRQ(A_RNDONE);
  }
  break;
  case NEWS:
  {
    if (saveDeviceMode(root["devMode"]))
    {
      _sendRQ(CONFIG);
      reqHandShake();
    }
    else
      _sendRQ(A_RNDONE);
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

  char MSG[500];
  DynamicJsonDocument msg(5200);
  msg["DID"] = 555;
  msg["ID"] = IDS;
  msg["RQ"] = HAND_SHAKE;
  msg["mode"] = 1;
  msg["dvMode"] = device_mode;

  msg["eUp"] = e_up;
  msg["cUp"] = c_up;
  msg["sUp"] = s_up;
  msg["PID"] = PID;
  msg["isJ"] = isJ;
  msg["lang"] = language;

  msg["motSelect"] = motor_type;
  msg["devSelect"] = ability;
  msg["bladeSelect"] = blade_mode;

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
        { _handleFileUpload(); });
    _server.on(
        "/edit", HTTP_POST, [&]() { // If a POST request is sent to the /edit.html address,
          _server.send(200, "text/plain", "");
        },
        [&]()
        { _handleFileUpload(); });

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

            if (!Update.begin(UPDATE_SIZE_UNKNOWN))
            { // start with max available size
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_WRITE)
          {
            /* flashing firmware to ESP*/
            _UploadedSizeBuffer++;
            long _progress = map(_UploadedSizeBuffer, 0, 700, 0, 100);
            if (_progress > 100)
              _progress = 100;

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

//  if (millis() - wifiTimer > 3000)
//  {
//    wifiTimer = millis();
//    if(on_wi_fi == true)Serial.println("WIFI IS ON");
//  }
}

void WebApp::webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{ // When a WebSocket message is received

  if (type == WStype_DISCONNECTED)
  {
    Serial.printf("[%u] Disconnected!\n", num);
  }
  else if (type == WStype_CONNECTED)
  {
    reqHandShake();
    vTaskDelay(100);
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
