

// void wifi_turnOff()
// {
//   Serial.println("wifi disabled");
//   delay(100);
//   Serial.println("wifi disabled");
//   delay(100);
//   Serial.println("wifi disabled");
//   delay(100);

//   WiFi.disconnect();
//   delay(100);
//   WiFi.mode(WIFI_OFF);
//   delay(100);
// }

// void wifi_loop()
// {
//   if (forceWifiSetup)
//   {
//     forceWifiSetup = false;
//     wifi_setup();
//     Serial.println("wifi setup Lunched");
//   }

//   server.handleClient();
//   dnsServer.processNextRequest();
//   webSocket.loop();

//   if (millis() - wifiTimer > 3000)
//   { // every 3 seconds
//     wifiTimer = millis();
//     Serial.println("WIFI IS ON");
//   }
// }

// void passReport(uint8_t report)
// {
//   char MSG[500];
//   JsonObject &msg = jsonBuffer.createObject();
//   msg["DID"] = 555;
//   msg["RQ"] = report;
//   if (report == AUTH)
//   {
//     msg["eUp"] = e_up;
//     msg["cUp"] = c_up;
//     msg["sUp"] = s_up;
//     msg["PID"] = PID;
//     msg["isJ"] = isJ;
//   }
//   msg.printTo(MSG);
//   // webSocket.broadcastTXT(MSG);
//   webSocket.sendTXT(TempCLNT, MSG);
//   jsonBuffer.clear();
// }

// void updateModemSTT()
// {
//   char MSG[500];

//   JsonObject &msg = jsonBuffer.createObject();
//   msg["DID"] = 555;
//   msg["RQ"] = MDM_STT;
//   msg["modem"] = WIFISSID;
//   msg["IP"] = "172.217.28.1";

//   msg.printTo(MSG);
//   webSocket.broadcastTXT(MSG);
//   jsonBuffer.clear();
// }

// void handshake()
// {
//   char MSG[500];
//   JsonObject &msg = jsonBuffer.createObject();
//   msg["DID"] = 555;
//   msg["RQ"] = HAND_SHAKE;
//   msg["mode"] = 1;
//   //  msg["Date"]     = ((now.year() - 2000) * 100000000) + (now.month() * 1000000) + (now.day() * 10000) + (now.hour() * 100) + now.minute();
//!-==-=----=-==-=-=-=--=-==--===-=--=
//   JsonArray &DUTL = msg.createNestedArray("DUTs");
//   for (int i = 0; i < 2; i++)
//   {
//     DUTL.add(1);
//   }
//!-==-=----=-==-=-=-=--=-==--===-=--=
//   //  byte Temptime_hot = EEPROM.read(0);
//   //  delay(10);
//   //  byte Temptime_cool = EEPROM.read(3);
//   //  uint32_t Times = Temptime_hot + (Temptime_cool * 1000);
//   //  msg["Times"]     = Times;

//   msg["eUp"] = e_up;
//   msg["cUp"] = c_up;
//   msg["sUp"] = s_up;
//   msg["PID"] = PID;
//   msg["isJ"] = isJ;
//   //  JsonArray& DayL = msg.createNestedArray("Days");
//   //  for (int i = 0; i < 7 ; i++) {
//   //    DayL.add(List.daysMask[i] ? 0 : 1);
//   //  }

//   msg.printTo(MSG);
//   webSocket.broadcastTXT(MSG);
//   jsonBuffer.clear();
// }
