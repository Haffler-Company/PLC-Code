#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>
#include "defines.h"
#include "storage.h"
#include "shift_manager.h"
#include "sound.h"
#include <lvgl.h>
#include "ui.h"
#include "ui_notification.h"
#include "day_lock.h"
#include "ota_manager.h"

//? ============================================================================
//? EMBEDDED MOBILE WIFI WEBAPP & OTA PORTAL (Apple HIG Multi-Tab Industrial)
//? Inspired by Haffler Industrial Gas-Filling & Config-Test Standards
//? ============================================================================

extern Storage storage;
extern bool cycleStarted;
extern bool processActive;
extern volatile bool device_locked;
extern int day_lock_remain;
extern bool rtc_available;
extern SemaphoreHandle_t lv_mutex;

static inline bool web_lv_lock(TickType_t timeout = pdMS_TO_TICKS(50))
{
    if (lv_mutex == NULL) return true;
    return (xSemaphoreTakeRecursive(lv_mutex, timeout) == pdTRUE);
}

static inline void web_lv_unlock()
{
    if (lv_mutex != NULL)
    {
        xSemaphoreGiveRecursive(lv_mutex);
    }
}

class WebPortal {
private:
    WebServer server{80};
    DNSServer dnsServer;
    const byte DNS_PORT = 53;
    bool isRunning = false;
    char apSSID[32];

public:
    void begin() {
        uint64_t mac = ESP.getEfuseMac();
        uint16_t macSuffix = (uint16_t)(mac & 0xFFFF);
        snprintf(apSSID, sizeof(apSSID), "Haffler_Bakery_%04X", macSuffix);

        IPAddress apIP(192, 168, 4, 1);
        IPAddress netMsk(255, 255, 255, 0);

        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(apIP, apIP, netMsk);
        WiFi.softAP(apSSID, "haffler1234");

        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", apIP);

        setupRoutes();
        server.begin();
        isRunning = true;
        Serial.printf("[WEB-PORTAL] Started: %s (IP: 192.168.4.1)\n", apSSID);
    }

    void stop() {
        if (!isRunning) return;
        server.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        isRunning = false;
        Serial.println("[WEB-PORTAL] Stopped");
    }

    bool getIsRunning() const { return isRunning; }

    void handle() {
        if (!isRunning) return;
        dnsServer.processNextRequest();
        server.handleClient();
    }

    const char* getSSID() const { return apSSID; }

private:
    void setupRoutes() {
        // Main Single Page WebApp
        server.on("/", HTTP_GET, [this]() { sendDashboardHtml(); });
        server.on("/generate_204", HTTP_GET, [this]() { sendDashboardHtml(); });
        server.on("/hotspot-detect.html", HTTP_GET, [this]() { sendDashboardHtml(); });
        server.on("/canonical.html", HTTP_GET, [this]() { sendDashboardHtml(); });

        // REST API: Live Status Readout
        server.on("/api/status", HTTP_GET, [this]() {
            String json = "{";
            json += "\"running\":" + String(cycleStarted ? "true" : "false") + ",";
            json += "\"portions_total\":" + String(storage.getChoneCounter()) + ",";
            json += "\"portions_shift\":" + String(shiftManager.getShiftPortions()) + ",";
            json += "\"shift_name\":\"" + String(shiftManager.getCurrentShiftName()) + "\",";
            json += "\"flour_kg\":" + String(shiftManager.getEstimatedFlourKg(storage.getAutoWeight()), 2) + ",";
            json += "\"auto_weight\":" + String(storage.getAutoWeight()) + ",";
            json += "\"auto_speed\":" + String(storage.getAutoSpeed()) + ",";
            json += "\"max_piston\":" + String(storage.getMaxPiston()) + ",";
            json += "\"max_funnel\":" + String(storage.get_setting_max_funnle()) + ",";
            json += "\"sensor_funnel\":" + String(digitalRead(FUNNELE_SENSOR) == LOW ? "true" : "false") + ",";
            json += "\"sensor_piston\":" + String(digitalRead(PISTON_SENSOR) == LOW ? "true" : "false") + ",";
            json += "\"motor_piston\":" + String(digitalRead(PISTON_MOTOR) == LOW ? "true" : "false") + ",";
            json += "\"motor_funnel\":" + String(digitalRead(FUNNELE_MOTOR) == LOW ? "true" : "false") + ",";
            json += "\"inverter_fwd\":" + String(digitalRead(INVERTER_FORWARD) == LOW ? "true" : "false") + ",";
            json += "\"inverter_rev\":" + String(digitalRead(INVERTER_REVERSE) == LOW ? "true" : "false") + ",";
            json += "\"device_locked\":" + String(device_locked ? "true" : "false") + ",";
            json += "\"lock_remain\":" + String(day_lock_remain) + ",";
            json += "\"uptime_sec\":" + String(millis() / 1000UL) + ",";
            json += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
            json += "\"wifi_ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "192.168.4.1") + "\",";
            json += "\"version\":\"" + storage.getVersion() + "\"";
            json += "}";
            server.send(200, "application/json", json);
        });

        // REST API: Day-Lock Status Readout
        server.on("/api/daylock/status", HTTP_GET, [this]() {
            String stateStr = "FACTORY_FREE";
            switch (day_lock_get_state()) {
                case LOCK_STAGE_ACTIVE_LEASE: stateStr = "ACTIVE_LEASE"; break;
                case LOCK_STAGE_GRACE_10:     stateStr = "GRACE_10"; break;
                case LOCK_STAGE_SOFT_LOCK:    stateStr = "SOFT_LOCK"; break;
                case LOCK_STAGE_HARD_LOCK:    stateStr = "HARD_LOCK"; break;
                case LOCK_STAGE_TAMPER:       stateStr = "TAMPER_LOCKDOWN"; break;
                default:                      stateStr = "FACTORY_FREE"; break;
            }
            String json = "{";
            json += "\"state\":\"" + stateStr + "\",";
            json += "\"stage\":" + String(day_lock_active_stage) + ",";
            json += "\"remain_days\":" + String(day_lock_remain) + ",";
            json += "\"grace_used\":" + String(day_lock_grace10_used ? "true" : "false") + ",";
            json += "\"challenge_code\":" + String(dev_lock_challenge) + ",";
            json += "\"device_locked\":" + String(device_locked ? "true" : "false") + ",";
            json += "\"rtc_healthy\":" + String(rtc_available ? "true" : "false");
            json += "}";
            server.send(200, "application/json", json);
        });

        // REST API: Day-Lock Remote Unlock
        server.on("/api/daylock/unlock", HTTP_POST, [this]() {
            String pin = "";
            if (server.hasArg("pin")) {
                pin = server.arg("pin");
            } else if (server.hasArg("plain")) {
                String body = server.arg("plain");
                int idx = body.indexOf("\"pin\"");
                if (idx >= 0) {
                    int c1 = body.indexOf(':', idx);
                    int q1 = body.indexOf('"', c1);
                    int q2 = body.indexOf('"', q1 + 1);
                    if (q1 >= 0 && q2 > q1) pin = body.substring(q1 + 1, q2);
                }
            }
            if (pin.length() > 0 && day_lock_try_unlock(pin.c_str())) {
                String json = "{\"success\":true,\"message\":\"کد با موفقیت اعمال شد\",\"active_stage\":" + 
                              String(day_lock_active_stage) + ",\"new_remain_days\":" + String(day_lock_remain) + "}";
                server.send(200, "application/json", json);
            } else {
                server.send(400, "application/json", "{\"success\":false,\"message\":\"کد بازگشایی نامعتبر است\"}");
            }
        });

        // REST API: Parameter Tuning
        server.on("/api/set", HTTP_POST, [this]() {
            bool changed = false;
            char msg[64] = {0};
            if (server.hasArg("weight")) {
                int w = server.arg("weight").toInt();
                if (w >= 50 && w <= 2500) {
                    storage.setAutoWeight(w);
                    changed = true;
                    snprintf(msg, sizeof(msg), "وزن: %d ms از پنل وب", w);
                }
            }
            if (server.hasArg("speed")) {
                int s = server.arg("speed").toInt();
                if (s >= 50 && s <= 3000) {
                    storage.setAutoSpeed(s);
                    changed = true;
                    snprintf(msg, sizeof(msg), "وقفه: %d ms از پنل وب", s);
                }
            }
            if (server.hasArg("max_piston")) {
                int mp = server.arg("max_piston").toInt();
                if (mp >= 500 && mp <= 5000) {
                    storage.set_setting_max_piston(mp);
                    changed = true;
                }
            }
            if (server.hasArg("max_funnel")) {
                int mf = server.arg("max_funnel").toInt();
                if (mf >= 500 && mf <= 5000) {
                    storage.set_setting_max_funnle(mf);
                    changed = true;
                }
            }
            if (changed && web_lv_lock()) {
                lv_label_set_text(ui_AutomaticMode_WeightValue, String(storage.getAutoWeight()).c_str());
                lv_label_set_text(ui_AutomaticMode_SpeedValue, String(storage.getAutoSpeed()).c_str());
                ui_show_notification(NOTIF_INFO, "تنظیمات وب اپلیکیشن", msg);
                web_lv_unlock();
            }
            server.send(200, "application/json", "{\"status\":\"ok\"}");
        });

        // REST API: Shift Reset
        server.on("/api/shift/reset", HTTP_POST, [this]() {
            shiftManager.resetShiftPortions();
            server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"شمارنده شیفت صفر شد\"}");
        });

        // REST API: Machine Start / Stop / Manual IO Test
        server.on("/api/control", HTTP_POST, [this]() {
            if (server.hasArg("cmd")) {
                String cmd = server.arg("cmd");
                if (cmd == "start" && !cycleStarted) {
                    if (device_locked) {
                        server.send(403, "application/json", "{\"error\":\"دستگاه قفل است\"}");
                        return;
                    }
                    processActive = true;
                    cycleStarted = true;
                    sound_play(SOUND_START_CYCLE);
                    if (web_lv_lock()) {
                        if (ui_AutomaticMode_StartStopBTT && lv_obj_is_valid(ui_AutomaticMode_StartStopBTT)) {
                            lv_label_set_text(ui_AutomaticMode_StartStopBTT, "توقف");
                            lv_obj_add_state(ui_AutomaticMode_StartStopButton, LV_STATE_USER_1);
                            lv_obj_add_state(ui_AutomaticMode_BackButton, LV_STATE_DISABLED);
                            lv_obj_add_state(ui_AutomaticMode_SetupHolder, LV_STATE_USER_1);
                        }
                        ui_show_notification(NOTIF_INFO, "کنترل وای‌فای", "دستور شروع فرآیند از پنل وب صادر شد");
                        web_lv_unlock();
                    }
                } else if (cmd == "stop" && cycleStarted) {
                    processActive = false;
                    cycleStarted = false;
                    sound_play(SOUND_STOP_CYCLE);
                    if (web_lv_lock()) {
                        if (ui_AutomaticMode_StartStopBTT && lv_obj_is_valid(ui_AutomaticMode_StartStopBTT)) {
                            lv_label_set_text(ui_AutomaticMode_StartStopBTT, "شروع");
                            lv_obj_clear_state(ui_AutomaticMode_StartStopButton, LV_STATE_USER_1);
                            lv_obj_clear_state(ui_AutomaticMode_BackButton, LV_STATE_DISABLED);
                            lv_obj_clear_state(ui_AutomaticMode_SetupHolder, LV_STATE_USER_1);
                        }
                        ui_show_notification(NOTIF_INFO, "کنترل وای‌فای", "دستور توقف فرآیند از پنل وب صادر شد");
                        web_lv_unlock();
                    }
                }
                // Manual Relay Testing (Only allowed when auto cycle is stopped!)
                else if (!cycleStarted) {
                    if (cmd == "piston_on") {
                        digitalWrite(PISTON_MOTOR, LOW);
                    } else if (cmd == "piston_off") {
                        digitalWrite(PISTON_MOTOR, HIGH);
                    } else if (cmd == "funnel_on") {
                        digitalWrite(FUNNELE_MOTOR, LOW);
                    } else if (cmd == "funnel_off") {
                        digitalWrite(FUNNELE_MOTOR, HIGH);
                    }
                    // STRICT INVERTER INTERLOCK
                    else if (cmd == "fwd_on") {
                        digitalWrite(INVERTER_REVERSE, HIGH);
                        delay(INVERTER_DEADTIME_MS);
                        digitalWrite(INVERTER_FORWARD, LOW);
                    } else if (cmd == "rev_on") {
                        digitalWrite(INVERTER_FORWARD, HIGH);
                        delay(INVERTER_DEADTIME_MS);
                        digitalWrite(INVERTER_REVERSE, LOW);
                    } else if (cmd == "inverter_off") {
                        digitalWrite(INVERTER_FORWARD, HIGH);
                        digitalWrite(INVERTER_REVERSE, HIGH);
                    }
                }
            }
            server.send(200, "application/json", "{\"status\":\"ok\"}");
        });

        // REST API: Trigger Server OTA Download
        server.on("/api/ota/start", HTTP_POST, [this]() {
            if (WiFi.status() != WL_CONNECTED) {
                server.send(400, "application/json", "{\"error\":\"دستگاه به اینترنت متصل نیست\"}");
                return;
            }
            int prog = server.hasArg("prog") ? server.arg("prog").toInt() : 0;
            String customUrl = server.hasArg("url") ? server.arg("url") : "";
            if (prog == 2 && customUrl.length() > 0) {
                otaManager.startUpdateAsync(2, customUrl.c_str());
            } else {
                otaManager.startUpdateAsync(prog);
            }
            server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"فرآیند دانلود آغاز شد\"}");
        });

        // REST API: Check OTA Download Progress
        server.on("/api/ota/status", HTTP_GET, [this]() {
            String json = "{";
            json += "\"is_downloading\":" + String(otaManager.is_downloading ? "true" : "false") + ",";
            json += "\"percent\":" + String(otaManager.download_percent) + ",";
            json += "\"success\":" + String(otaManager.download_success ? "true" : "false") + ",";
            json += "\"error\":" + String(otaManager.download_error ? "true" : "false");
            json += "}";
            server.send(200, "application/json", json);
        });

        // OTA Direct File Upload Handler (.bin file)
        server.on("/update", HTTP_POST, [this]() {
            server.sendHeader("Connection", "close");
            server.send(200, "text/plain", (Update.hasError()) ? "UPDATE_FAIL" : "UPDATE_OK");
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP.restart();
        }, [this]() {
            HTTPUpload& upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("[OTA-UPLOAD] Starting: %s\n", upload.filename.c_str());
                if (web_lv_lock()) {
                    ui_show_notification(NOTIF_WARNING, "بروزرسانی OTA", "در حال دریافت فایل فریم‌ور... برق دستگاه را قطع نکنید");
                    web_lv_unlock();
                }
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {
                    Serial.printf("[OTA-UPLOAD] Success: %u bytes\n", upload.totalSize);
                    if (web_lv_lock()) {
                        ui_show_notification(NOTIF_SUCCESS, "بروزرسانی موفق", "فریم‌ور نصب شد - ریبوت سیستم...");
                        web_lv_unlock();
                    }
                } else {
                    Update.printError(Serial);
                    if (web_lv_lock()) {
                        ui_show_notification(NOTIF_ERROR, "خطای بروزرسانی", "نصب فایل فریم‌ور با خطا مواجه شد");
                        web_lv_unlock();
                    }
                }
            }
        });
    }

    void sendDashboardHtml() {
        static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no">
<title>هافلر - سامانه وب اپلیکیشن چانه‌گیر نانوایی</title>
<style>
:root{--bg:#090d16;--card:#182234;--card-border:#2a3b53;--accent:#0284c7;--green:#10b981;--red:#ef4444;--amber:#f59e0b;--text:#f8fafc;--sub:#94a3b8}
*{box-sizing:border-box;margin:0;padding:0;font-family:Tahoma,-apple-system,BlinkMacSystemFont,sans-serif;-webkit-tap-highlight-color:transparent}
body{background:var(--bg);color:var(--text);padding:12px;display:flex;flex-direction:column;gap:12px;min-height:100vh;max-width:540px;margin:auto}
.header{background:linear-gradient(135deg,#182234,#0e1524);border:1px solid var(--card-border);border-radius:16px;padding:14px 16px;display:flex;justify-content:space-between;align-items:center;box-shadow:0 4px 20px #0006}
.brand{font-size:18px;font-weight:900;color:#38bdf8;letter-spacing:.5px}
.brand small{display:block;font-size:11px;color:var(--sub);font-weight:normal;margin-top:2px}
.conn-badge{display:flex;align-items:center;gap:6px;font-size:11px;font-weight:bold;padding:6px 12px;border-radius:20px;background:#090d16;border:1px solid var(--card-border)}
.dot{width:8px;height:8px;border-radius:50%;background:#475569}
.dot.on{background:#10b981;box-shadow:0 0 10px #10b981}
.nav-tabs{display:grid;grid-template-columns:repeat(7,1fr);gap:4px;background:#182234;padding:4px;border-radius:12px;border:1px solid var(--card-border)}
.nav-btn{padding:8px 2px;border:none;background:transparent;color:var(--sub);font-size:10px;font-weight:bold;border-radius:8px;cursor:pointer;transition:.15s;text-align:center}
.nav-btn.active{background:var(--accent);color:#fff;box-shadow:0 2px 8px #0284c766}
.page{display:none;flex-direction:column;gap:12px}.page.active{display:flex}
.card{background:var(--card);border:1px solid var(--card-border);border-radius:16px;padding:14px;display:flex;flex-direction:column;gap:10px;box-shadow:0 4px 14px #0003}
.card-title{font-size:13px;color:#38bdf8;font-weight:bold;display:flex;justify-content:space-between;align-items:center}
.grid-2{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.kpi{background:#0e1524;border:1px solid var(--card-border);border-radius:12px;padding:10px;text-align:center}
.kpi-val{font-size:22px;font-weight:900;color:#fff}
.kpi-lbl{font-size:10px;color:var(--sub);margin-top:2px}
.pbar-bg{height:12px;background:#0e1524;border:1px solid var(--card-border);border-radius:6px;overflow:hidden}
.pbar-fill{height:100%;background:linear-gradient(90deg,#0284c7,#10b981);width:0%;transition:width .3s}
.btn{padding:10px;border:none;border-radius:10px;font-size:13px;font-weight:bold;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:6px;color:#fff;transition:.15s}
.btn-green{background:var(--green)}.btn-red{background:var(--red)}.btn-blue{background:var(--accent)}.btn-amber{background:var(--amber)}.btn-slate{background:#334155}
.btn:active{transform:scale(.98);opacity:.9}
.form-grp{display:flex;flex-direction:column;gap:4px}
.form-grp label{font-size:11px;color:var(--sub);font-weight:bold}
input,select{width:100%;padding:8px 10px;background:#0e1524;border:1px solid var(--card-border);border-radius:8px;color:#fff;font-size:13px;text-align:center;direction:ltr}
input:focus{outline:2px solid var(--accent);border-color:transparent}
table{width:100%;border-collapse:collapse;font-size:11px}
th,td{padding:6px 8px;border:1px solid var(--card-border);text-align:center}
th{background:#0e1524;color:#38bdf8}
.badge{padding:3px 8px;border-radius:12px;font-size:10px;font-weight:bold}
.badge-ok{background:#10b98133;color:#34d399;border:1px solid #10b981}
.badge-warn{background:#f59e0b33;color:#fbbf24;border:1px solid #f59e0b}
.badge-err{background:#ef444433;color:#f87171;border:1px solid #ef4444}
.sensor-row{display:flex;align-items:center;justify-content:space-between;padding:6px 10px;background:#0e1524;border:1px solid var(--card-border);border-radius:8px;font-size:12px}
.sensor-dot{width:10px;height:10px;border-radius:50%;display:inline-block}
.sensor-on{background:#10b981;box-shadow:0 0 8px #10b981}
.sensor-off{background:#475569}
.footer{text-align:center;font-size:11px;color:var(--sub);margin-top:auto;padding:8px}
</style>
</head>
<body>
<header class="header">
<div>
  <div class="brand">HAFFLER <span style="font-size:13px;color:#94a3b8">BAKERY PLC</span></div>
  <small>سامانه هوشمند مانیتورینگ و کنترل چانه‌گیر خمیر</small>
</div>
<div class="conn-badge"><span id="dot-conn" class="dot on"></span><span id="txt-ver">V1.1.0</span></div>
</header>

<nav class="nav-tabs">
<button class="nav-btn active" onclick="tab('t-dash',this)">📊 داشبورد</button>
<button class="nav-btn" onclick="tab('t-set',this)">⚙️ فرآیند</button>
<button class="nav-btn" onclick="tab('t-io',this)">🔌 پایه‌ها</button>
<button class="nav-btn" onclick="tab('t-lock',this)">⏳ روزشمار</button>
<button class="nav-btn" onclick="tab('t-ota',this)">🚀 ارتقا</button>
<button class="nav-btn" onclick="tab('t-stats',this)">📈 آمار</button>
<button class="nav-btn" onclick="tab('t-help',this)">📘 راهنما</button>
</nav>

<!-- TAB 1: DASHBOARD -->
<main id="t-dash" class="page active">
  <div class="card">
    <div class="card-title"><span>وضعیت عملیاتی ماشین</span><span id="m-badge" class="badge badge-err">متوقف</span></div>
    <div class="grid-2">
      <div class="kpi"><div class="kpi-val" id="k-total">0</div><div class="kpi-lbl">مجموع کل چانه‌ها</div></div>
      <div class="kpi"><div class="kpi-val" id="k-shift" style="color:#34d399">0</div><div class="kpi-lbl">تولید شیفت فعلی</div></div>
    </div>
    <div class="grid-2">
      <div class="kpi"><div class="kpi-val" id="k-weight" style="color:#38bdf8">--</div><div class="kpi-lbl">زمان وزن (پیستون ms)</div></div>
      <div class="kpi"><div class="kpi-val" id="k-speed" style="color:#fbbf24">--</div><div class="kpi-lbl">وقفه سرعت (قیف ms)</div></div>
    </div>
    <div class="grid-2">
      <button class="btn btn-green" id="btn-start" onclick="postCmd('/api/control',{cmd:'start'})">▶ شروع اتوماتیک</button>
      <button class="btn btn-red" id="btn-stop" onclick="postCmd('/api/control',{cmd:'stop'})">⏹ توقف اضطراری</button>
    </div>
  </div>

  <div class="card">
    <div class="card-title"><span>وضعیت سنسورهای هوم مکانیکی</span></div>
    <div class="sensor-row">
      <span>سنسور موقعیت هوم پیستون (GPIO 7)</span>
      <span id="s-piston" class="sensor-dot sensor-off"></span>
    </div>
    <div class="sensor-row">
      <span>سنسور موقعیت هوم قیف (GPIO 3)</span>
      <span id="s-funnel" class="sensor-dot sensor-off"></span>
    </div>
  </div>
</main>

<!-- TAB 2: PROCESS & SETTINGS -->
<main id="t-set" class="page">
  <div class="card">
    <div class="card-title"><span>تنظیم پارامترهای برشی چانه‌گیر</span></div>
    <div class="form-grp">
      <label>وزن چانه / زمان رفت و برگشت پیستون (50 - 2500 میلی‌ثانیه):</label>
      <div style="display:flex;gap:6px">
        <button class="btn btn-slate" style="width:44px" onclick="adj('weight',-20)">-</button>
        <input type="number" id="inp-weight" min="50" max="2500">
        <button class="btn btn-slate" style="width:44px" onclick="adj('weight',20)">+</button>
      </div>
    </div>
    <div class="form-grp">
      <label>سرعت تولید / وقفه استراحت قیف (50 - 3000 میلی‌ثانیه):</label>
      <div style="display:flex;gap:6px">
        <button class="btn btn-slate" style="width:44px" onclick="adj('speed',-20)">-</button>
        <input type="number" id="inp-speed" min="50" max="3000">
        <button class="btn btn-slate" style="width:44px" onclick="adj('speed',20)">+</button>
      </div>
    </div>
    <div class="grid-2">
      <div class="form-grp">
        <label>حداکثر زمان پیستون (ms):</label>
        <input type="number" id="inp-max-piston" min="500" max="5000">
      </div>
      <div class="form-grp">
        <label>حداکثر زمان قیف (ms):</label>
        <input type="number" id="inp-max-funnel" min="500" max="5000">
      </div>
    </div>
    <button class="btn btn-blue" onclick="saveParams()">💾 ذخیره و همگام‌سازی با HMI</button>
  </div>
</main>

<!-- TAB 3: HARDWARE IO & MANUAL TEST -->
<main id="t-io" class="page">
  <div class="card">
    <div class="card-title"><span>تست دستی خروجی‌ها (صرفاً در حالت توقف ماشین)</span></div>
    <div class="grid-2">
      <button class="btn btn-slate" onclick="postCmd('/api/control',{cmd:'piston_on'})">روشن پیستون</button>
      <button class="btn btn-slate" onclick="postCmd('/api/control',{cmd:'piston_off'})">خاموش پیستون</button>
    </div>
    <div class="grid-2">
      <button class="btn btn-slate" onclick="postCmd('/api/control',{cmd:'funnel_on'})">روشن قیف</button>
      <button class="btn btn-slate" onclick="postCmd('/api/control',{cmd:'funnel_off'})">خاموش قیف</button>
    </div>
    <div class="card-title" style="margin-top:6px"><span>کنترل اینورتر (با اینترلاک سخت‌افزاری 100ms)</span></div>
    <div class="grid-2">
      <button class="btn btn-blue" onclick="postCmd('/api/control',{cmd:'fwd_on'})">اینورتر جلو (FWD)</button>
      <button class="btn btn-amber" onclick="postCmd('/api/control',{cmd:'rev_on'})">اینورتر معکوس (REV)</button>
    </div>
    <button class="btn btn-red" onclick="postCmd('/api/control',{cmd:'inverter_off'})">خاموشی کامل اینورتر</button>
  </div>

  <div class="card">
    <div class="card-title"><span>جدول وضعیت زنده پایه‌ها (بدون پین‌های اضافی M)</span></div>
    <table>
      <tr><th>نام سیگنال</th><th>پایه ESP32</th><th>نوع</th><th>وضعیت زنده</th></tr>
      <tr><td>سنسور هوم پیستون</td><td>GPIO 7</td><td>ورودی سنسور</td><td id="td-s-piston">OFF</td></tr>
      <tr><td>سنسور هوم قیف</td><td>GPIO 3</td><td>ورودی سنسور</td><td id="td-s-funnel">OFF</td></tr>
      <tr><td>رله موتور پیستون</td><td>GPIO 36</td><td>خروجی رله</td><td id="td-m-piston">OFF</td></tr>
      <tr><td>رله موتور قیف</td><td>GPIO 42</td><td>خروجی رله</td><td id="td-m-funnel">OFF</td></tr>
      <tr><td>اینورتر جلو FWD</td><td>GPIO 39</td><td>خروجی اینورتر</td><td id="td-inv-fwd">OFF</td></tr>
      <tr><td>اینورتر معکوس REV</td><td>GPIO 48</td><td>خروجی اینورتر</td><td id="td-inv-rev">OFF</td></tr>
    </table>
  </div>
</main>

<!-- TAB 4: DAY-LOCK LEASE -->
<main id="t-lock" class="page">
  <div class="card">
    <div class="card-title"><span>مدیریت قفل امنیتی روزشمار</span><span id="l-badge" class="badge badge-ok">فعال</span></div>
    <div class="grid-2">
      <div class="kpi"><div class="kpi-val" id="l-remain" style="color:#38bdf8">--</div><div class="kpi-lbl">روزهای باقیمانده کارکرد</div></div>
      <div class="kpi"><div class="kpi-val" id="l-chal" style="color:#fbbf24;font-size:18px">--</div><div class="kpi-lbl">کد شناسه دستگاه</div></div>
    </div>
    <div class="form-grp">
      <label>کد بازگشایی دریافتی از شرکت هافلر:</label>
      <input type="text" id="inp-unlock" placeholder="کد ۸ رقمی">
    </div>
    <button class="btn btn-green" onclick="submitUnlock()">🔓 تمدید اعتبار و بازگشایی</button>
  </div>
</main>

<!-- TAB 5: OTA LAUNCHER & FIRMWARE UPDATE -->
<main id="t-ota" class="page">
  <div class="card">
    <div class="card-title"><span>دریافت مستقیم از مخزن سرور هافلر</span></div>
    <div class="grid-2">
      <button class="btn btn-green" onclick="startOta(0)">نان چانه‌گیر هافلر</button>
      <button class="btn btn-amber" onclick="startOta(1)">کانفیگ و تست سخت‌افزار</button>
    </div>
  </div>

  <div class="card">
    <div class="card-title"><span>ورود آدرس لینک دلخواه (Custom URL)</span></div>
    <div class="form-grp">
      <input type="text" id="inp-custom-url" placeholder="https://.../firmware.bin">
    </div>
    <button class="btn btn-blue" onclick="startCustomOta()">🚀 دانلود و نصب از این لینک</button>
  </div>

  <div class="card">
    <div class="card-title"><span>ارتقای مستقیم از فایل باینری (.bin)</span></div>
    <form method="POST" action="/update" enctype="multipart/form-data" style="display:flex;flex-direction:column;gap:8px">
      <input type="file" name="update" accept=".bin">
      <button type="submit" class="btn btn-slate">بارگذاری و فلش دستی</button>
    </form>
  </div>

  <div class="card" id="ota-prog-card" style="display:none">
    <div class="card-title"><span>پیشرفت دانلود و نوشتن فریم‌ور</span><span id="ota-pct-lbl">0%</span></div>
    <div class="pbar-bg"><div id="ota-pbar" class="pbar-fill"></div></div>
    <small id="ota-sub-lbl" style="color:#94a3b8">لطفاً تا پایان عملیات و ریبوت دستگاه، برق را قطع نکنید.</small>
  </div>
</main>

<!-- TAB 6: SHIFT & PRODUCTION LOGS -->
<main id="t-stats" class="page">
  <div class="card">
    <div class="card-title"><span>آمار تفکیکی شیفت‌های کاری</span></div>
    <div class="grid-2">
      <div class="kpi"><div class="kpi-val" id="s-name" style="color:#38bdf8">--</div><div class="kpi-lbl">شیفت فعال</div></div>
      <div class="kpi"><div class="kpi-val" id="s-flour" style="color:#fbbf24">0</div><div class="kpi-lbl">آرد مصرفی (کیلوگرم)</div></div>
    </div>
    <button class="btn btn-red" onclick="resetShift()">🔄 صفر کردن شمارنده شیفت فعلی</button>
  </div>
</main>

<!-- TAB 7: USER MANUAL & PINOUT -->
<main id="t-help" class="page">
  <div class="card">
    <div class="card-title"><span>راهنمای کاربری و نکات ایمنی</span></div>
    <p style="font-size:12px;line-height:1.8;color:#cbd5e1">
      • برای تنظیم وزن خمیر، زمان برگشت پیستون را کم یا زیاد کنید.<br>
      • سنسورهای هوم پیستون و قیف باید پیش از استارت سیکل اتوماتیک فعال باشند.<br>
      • در صورت بروز گیر مکانیکی، موتورها پس از ۵ ثانیه به صورت خودکار متوقف می‌شوند.<br>
      • کلید قرمز رنگ در صفحه HMI همیشه به معنای بازگشت ایمن به صفحه قبل است.
    </p>
  </div>
</main>

<footer class="footer">
  HAFFLER Industrial Bakery Machinery &bull; Embedded Web Control &bull; V1.1.0
</footer>

<script>
let curWeight=0,curSpeed=0,isRunning=false;

function tab(id,btn){
  document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
  document.querySelectorAll('.nav-btn').forEach(b=>b.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  btn.classList.add('active');
}

function fetchStatus(){
  fetch('/api/status')
    .then(r=>r.json())
    .then(d=>{
      document.getElementById('txt-ver').innerText = d.version;
      document.getElementById('k-total').innerText = d.portions_total.toLocaleString('fa-IR');
      document.getElementById('k-shift').innerText = d.portions_shift.toLocaleString('fa-IR');
      document.getElementById('k-weight').innerText = d.auto_weight + ' ms';
      document.getElementById('k-speed').innerText = d.auto_speed + ' ms';
      document.getElementById('inp-weight').value = d.auto_weight;
      document.getElementById('inp-speed').value = d.auto_speed;
      document.getElementById('inp-max-piston').value = d.max_piston;
      document.getElementById('inp-max-funnel').value = d.max_funnel;
      curWeight = d.auto_weight; curSpeed = d.auto_speed;
      isRunning = d.running;

      let b = document.getElementById('m-badge');
      if(isRunning){
        b.innerText = 'در حال کار'; b.className = 'badge badge-ok';
      } else {
        b.innerText = 'متوقف'; b.className = 'badge badge-err';
      }

      document.getElementById('s-piston').className = 'sensor-dot ' + (d.sensor_piston ? 'sensor-on' : 'sensor-off');
      document.getElementById('s-funnel').className = 'sensor-dot ' + (d.sensor_funnel ? 'sensor-on' : 'sensor-off');

      document.getElementById('td-s-piston').innerText = d.sensor_piston ? 'فعال (ON)' : 'غیرفعال';
      document.getElementById('td-s-funnel').innerText = d.sensor_funnel ? 'فعال (ON)' : 'غیرفعال';
      document.getElementById('td-m-piston').innerText = d.motor_piston ? 'روشن (ON)' : 'خاموش';
      document.getElementById('td-m-funnel').innerText = d.motor_funnel ? 'روشن (ON)' : 'خاموش';
      document.getElementById('td-inv-fwd').innerText = d.inverter_fwd ? 'روشن (FWD)' : 'خاموش';
      document.getElementById('td-inv-rev').innerText = d.inverter_rev ? 'روشن (REV)' : 'خاموش';

      document.getElementById('l-remain').innerText = d.lock_remain + ' روز';
      document.getElementById('s-name').innerText = d.shift_name;
      document.getElementById('s-flour').innerText = d.flour_kg;
    }).catch(()=>{});
}

function fetchDayLock(){
  fetch('/api/daylock/status')
    .then(r=>r.json())
    .then(d=>{
      document.getElementById('l-chal').innerText = d.challenge_code;
      let lb = document.getElementById('l-badge');
      if(d.device_locked){
        lb.innerText = 'قفل امنیتی'; lb.className = 'badge badge-err';
      } else {
        lb.innerText = d.state; lb.className = 'badge badge-ok';
      }
    }).catch(()=>{});
}

function postCmd(url, body){
  fetch(url, {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:new URLSearchParams(body)
  }).then(()=>fetchStatus());
}

function adj(field, delta){
  if(field === 'weight'){
    let w = Math.max(50, Math.min(2500, curWeight + delta));
    postCmd('/api/set', {weight:w});
  } else if(field === 'speed'){
    let s = Math.max(50, Math.min(3000, curSpeed + delta));
    postCmd('/api/set', {speed:s});
  }
}

function saveParams(){
  let w = document.getElementById('inp-weight').value;
  let s = document.getElementById('inp-speed').value;
  let mp = document.getElementById('inp-max-piston').value;
  let mf = document.getElementById('inp-max-funnel').value;
  postCmd('/api/set', {weight:w, speed:s, max_piston:mp, max_funnel:mf});
  alert('پارامترها ذخیره و با HMI همگام شدند.');
}

function submitUnlock(){
  let pin = document.getElementById('inp-unlock').value;
  if(!pin){ alert('لطفاً کد را وارد کنید'); return; }
  fetch('/api/daylock/unlock', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:new URLSearchParams({pin:pin})
  }).then(r=>r.json()).then(d=>{
    alert(d.message || (d.success ? 'کد با موفقیت اعمال شد' : 'کد نامعتبر است'));
    fetchDayLock();
  });
}

function resetShift(){
  if(confirm('آیا از صفر کردن شمارنده شیفت اطمینان دارید؟')){
    fetch('/api/shift/reset', {method:'POST'}).then(r=>r.json()).then(d=>{
      alert(d.message); fetchStatus();
    });
  }
}

function startOta(prog){
  let name = (prog === 0) ? 'نان چانه‌گیر' : 'کانفیگ و تست';
  if(confirm('آیا از دانلود و ارتقای فریم‌ور ' + name + ' اطمینان دارید؟')){
    postCmd('/api/ota/start', {prog:prog});
    document.getElementById('ota-prog-card').style.display = 'flex';
    pollOta();
  }
}

function startCustomOta(){
  let u = document.getElementById('inp-custom-url').value;
  if(!u || !u.startsWith('http')){ alert('لطفاً آدرس معتبر وارد کنید'); return; }
  if(confirm('آیا از دانلود این لینک اطمینان دارید؟')){
    postCmd('/api/ota/start', {prog:2, url:u});
    document.getElementById('ota-prog-card').style.display = 'flex';
    pollOta();
  }
}

function pollOta(){
  let timer = setInterval(()=>{
    fetch('/api/ota/status').then(r=>r.json()).then(d=>{
      document.getElementById('ota-pct-lbl').innerText = d.percent + '%';
      document.getElementById('ota-pbar').style.width = d.percent + '%';
      if(d.success){
        clearInterval(timer);
        document.getElementById('ota-sub-lbl').innerText = 'دانلود با موفقیت کامل شد. دستگاه در حال ریبوت است...';
      } else if(d.error){
        clearInterval(timer);
        document.getElementById('ota-sub-lbl').innerText = 'خطا در دانلود یا نوشتن حافظه فلش.';
      }
    });
  }, 1000);
}

setInterval(fetchStatus, 1000);
fetchStatus();
fetchDayLock();
</script>
</body>
</html>
)rawliteral";
        server.send(200, "text/html; charset=UTF-8", DASHBOARD_HTML);
    }
};

extern WebPortal webPortal;

#endif // WEB_PORTAL_H
