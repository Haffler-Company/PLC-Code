#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <Update.h>
#include "defines.h"
#include "hwtest.h"
#include "storage.h"

extern Storage storage;

// ارجاع به تسک‌ها و متغیرهای دانلود OTA فریمور
extern String ota_download_model;
extern bool ota_download;
extern String ota_custom_url;
extern bool ota_custom_download;
extern SemaphoreHandle_t xMutex_Wifi;

static const char PAGE_DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>HAFFLER HT-V3A | Industrial HMI &amp; PLC Controller</title>
<style>
:root {
  --bg: #090d16;
  --card-bg: rgba(21, 29, 45, 0.85);
  --card-border: #1e293b;
  --accent: #0284c7;
  --accent-light: #38bdf8;
  --success: #10b981;
  --success-glow: rgba(16, 185, 129, 0.35);
  --warning: #f59e0b;
  --danger: #ef4444;
  --text: #f1f5f9;
  --text-muted: #94a3b8;
  --font: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Vazirmatn", sans-serif;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: var(--font);
  background: var(--bg);
  color: var(--text);
  line-height: 1.6;
  padding: 14px;
  min-height: 100vh;
}
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: var(--card-bg);
  border: 1px solid var(--card-border);
  padding: 14px 18px;
  border-radius: 14px;
  margin-bottom: 14px;
  backdrop-filter: blur(10px);
}
.brand h1 { font-size: 18px; color: var(--accent-light); font-weight: 700; letter-spacing: 0.5px; }
.brand p { font-size: 11px; color: var(--text-muted); }
.header-meta { display: flex; gap: 8px; align-items: center; }
.badge {
  font-size: 11px;
  padding: 4px 10px;
  border-radius: 20px;
  background: rgba(56, 189, 248, 0.12);
  color: var(--accent-light);
  border: 1px solid rgba(56, 189, 248, 0.3);
  font-weight: 600;
  display: inline-flex;
  align-items: center;
  gap: 5px;
}
.live-dot {
  width: 8px; height: 8px; border-radius: 50%; background: var(--success);
  box-shadow: 0 0 8px var(--success); animation: pulse 2s infinite;
}
@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.4; } }

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
  gap: 10px;
  margin-bottom: 14px;
}
.stat-box {
  background: var(--card-bg);
  border: 1px solid var(--card-border);
  padding: 10px 14px;
  border-radius: 12px;
  text-align: center;
}
.stat-box .title { font-size: 11px; color: var(--text-muted); }
.stat-box .val { font-size: 16px; font-weight: 700; color: #fff; margin-top: 2px; direction: ltr; }

.tabs {
  display: flex;
  gap: 8px;
  margin-bottom: 14px;
  overflow-x: auto;
  padding-bottom: 4px;
}
.tab-btn {
  background: var(--card-bg);
  border: 1px solid var(--card-border);
  color: var(--text-muted);
  padding: 8px 16px;
  border-radius: 10px;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  transition: all 0.2s;
}
.tab-btn.active {
  background: var(--accent);
  color: #fff;
  border-color: var(--accent-light);
  box-shadow: 0 0 12px rgba(2, 132, 199, 0.4);
}

.tab-content { display: none; }
.tab-content.active { display: block; }

.card {
  background: var(--card-bg);
  border: 1px solid var(--card-border);
  border-radius: 14px;
  padding: 16px;
  margin-bottom: 14px;
  backdrop-filter: blur(8px);
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 14px;
  padding-bottom: 8px;
  border-bottom: 1px solid rgba(255,255,255,0.06);
}
.card-title { font-size: 15px; font-weight: 700; color: #fff; display: flex; align-items: center; gap: 8px; }

.relay-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(210px, 1fr));
  gap: 10px;
}
.relay-card {
  background: rgba(13, 17, 27, 0.7);
  border: 1px solid #1e293b;
  border-radius: 10px;
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  transition: border-color 0.2s;
}
.relay-card.active {
  border-color: var(--success);
  box-shadow: 0 0 10px var(--success-glow);
}
.relay-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.relay-name { font-size: 13px; font-weight: bold; }
.relay-pin { font-size: 11px; color: var(--accent-light); direction: ltr; }
.relay-led {
  width: 10px; height: 10px; border-radius: 50%;
  background: #334155; transition: background 0.3s, box-shadow 0.3s;
}
.relay-card.active .relay-led {
  background: var(--success);
  box-shadow: 0 0 8px var(--success);
}
.relay-actions {
  display: flex;
  gap: 6px;
  margin-top: 4px;
}
.btn {
  flex: 1;
  padding: 7px 10px;
  border-radius: 8px;
  border: none;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s;
  display: inline-flex;
  justify-content: center;
  align-items: center;
  gap: 4px;
}
.btn-primary { background: #0284c7; color: #fff; }
.btn-primary:hover { background: #0369a1; }
.btn-success { background: #059669; color: #fff; }
.btn-success:hover { background: #047857; }
.btn-warning { background: #d97706; color: #fff; }
.btn-warning:hover { background: #b45309; }
.btn-danger { background: #dc2626; color: #fff; }
.btn-danger:hover { background: #b91c1c; }
.btn-outline { background: transparent; border: 1px solid #334155; color: var(--text-muted); }
.btn-outline:hover { background: rgba(255,255,255,0.05); color: #fff; }

.temp-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
  gap: 12px;
}
.temp-card {
  background: rgba(13, 17, 27, 0.7);
  border: 1px solid #1e293b;
  border-radius: 12px;
  padding: 14px;
  text-align: center;
}
.temp-val {
  font-size: 24px;
  font-weight: 800;
  margin: 4px 0;
  direction: ltr;
}
.temp-c1 { color: #38bdf8; }
.temp-c2 { color: #a78bfa; }
.temp-rtc { color: #34d399; }
.temp-esp { color: #fbbf24; }

.inputs-row {
  display: grid;
  grid-template-columns: repeat(8, 1fr);
  gap: 6px;
  margin-top: 8px;
}
.input-indicator {
  background: rgba(13, 17, 27, 0.8);
  border: 1px solid #1e293b;
  border-radius: 8px;
  padding: 8px 4px;
  text-align: center;
}
.input-indicator.on {
  border-color: var(--success);
  background: rgba(16, 185, 129, 0.15);
}
.input-led {
  width: 12px; height: 12px; border-radius: 50%;
  background: #334155; margin: 0 auto 4px auto;
}
.input-indicator.on .input-led {
  background: var(--success);
  box-shadow: 0 0 8px var(--success);
}
.input-lbl { font-size: 11px; font-weight: bold; }
.input-v { font-size: 9px; color: var(--text-muted); margin-top: 2px; direction: ltr; }

.ota-box {
  background: rgba(13, 17, 27, 0.7);
  border: 1px solid #1e293b;
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 12px;
}
.ota-box h3 { font-size: 14px; color: var(--accent-light); margin-bottom: 8px; }
.form-group { margin-bottom: 12px; }
.form-group label { display: block; font-size: 12px; color: var(--text-muted); margin-bottom: 4px; }
.form-control {
  width: 100%;
  padding: 8px 12px;
  background: #0b0f19;
  border: 1px solid #334155;
  border-radius: 8px;
  color: #fff;
  font-size: 13px;
  outline: none;
}
.form-control:focus { border-color: var(--accent); }
.progress-bar-cont {
  height: 12px;
  background: #1e293b;
  border-radius: 6px;
  overflow: hidden;
  margin-top: 10px;
  display: none;
}
.progress-bar-val {
  height: 100%;
  background: linear-gradient(90deg, #0284c7, #38bdf8);
  width: 0%;
  transition: width 0.2s;
}

.footer {
  text-align: center;
  font-size: 11px;
  color: var(--text-muted);
  margin-top: 14px;
}
</style>
</head>
<body>

<div class="header">
  <div class="brand">
    <h1>HAFFLER HT-V3A PLC CONTROLLER</h1>
    <p>سامانه جامع عیب‌یابی صنعتی، مانیتورینگ بلادرنگ و بروزرسانی OTA</p>
  </div>
  <div class="header-meta">
    <span class="badge"><span class="live-dot"></span> زنده</span>
    <span class="badge" id="b-ver">VER )" VERSION R"(</span>
  </div>
</div>

<div class="stats-grid">
  <div class="stat-box"><div class="title">پردازنده</div><div class="val" id="st-cpu">240 MHz</div></div>
  <div class="stat-box"><div class="title">دمای چیپ</div><div class="val" id="st-temp">-- °C</div></div>
  <div class="stat-box"><div class="title">حافظه آزاد (SRAM)</div><div class="val" id="st-heap">-- KB</div></div>
  <div class="stat-box"><div class="title">حافظه PSRAM</div><div class="val" id="st-psram">-- MB</div></div>
  <div class="stat-box"><div class="title">وای‌فای کارگاه</div><div class="val" id="st-wifi">--</div></div>
  <div class="stat-box"><div class="title">مدت روشن بودن</div><div class="val" id="st-uptime">00:00:00</div></div>
</div>

<div class="tabs">
  <button class="tab-btn active" onclick="switchTab('tab-relays')">⚡ خروجی‌ها و رله‌ها (Q0..Q9)</button>
  <button class="tab-btn" onclick="switchTab('tab-sensors')">🔌 سنسورها و دما (MAX6675 &amp; ADC)</button>
  <button class="tab-btn" onclick="switchTab('tab-periph')">🖨️ پرینتر حرارتی و جانبی</button>
  <button class="tab-btn" onclick="switchTab('tab-ota')">🚀 بروزرسانی و سرور هافلر</button>
</div>

<!-- تب رله‌ها -->
<div id="tab-relays" class="tab-content active">
  <div class="card">
    <div class="card-header">
      <div class="card-title">⚡ وضعیت و فرمان ۱۰ خروجی رله و SSR</div>
      <button class="btn btn-danger" style="flex:0 0 auto; padding:6px 14px;" onclick="killAll()">🛑 خاموشی اضطراری همه</button>
    </div>
    <div class="relay-grid" id="relay-container">
      <!-- تولید خودکار -->
    </div>
  </div>

  <div class="card" id="mcp-card" style="display:none;">
    <div class="card-header">
      <div class="card-title">🎛️ خروجی‌های کمکی افزاینده MCP23017 (I2C)</div>
      <span class="badge" style="background:#065f46; color:#34d399;">چیپ متصل است</span>
    </div>
    <div style="display:grid; grid-template-columns: repeat(auto-fill, minmax(130px, 1fr)); gap:8px;" id="mcp-container">
      <!-- خروجی‌های GPA0..GPA5 -->
    </div>
  </div>
</div>

<!-- تب سنسورها -->
<div id="tab-sensors" class="tab-content">
  <div class="card">
    <div class="card-header">
      <div class="card-title">🌡️ حسگرهای دما (ترموکوپل‌های MAX6675 و ساعت)</div>
      <span class="badge" id="rtc-time-badge">RTC: --</span>
    </div>
    <div class="temp-grid">
      <div class="temp-card">
        <div style="font-size:12px; color:var(--text-muted);">کوره ۱ (TC1)</div>
        <div class="temp-val temp-c1" id="val-tc1">-- °C</div>
        <div style="font-size:11px;" id="stat-tc1">درحال خواندن...</div>
      </div>
      <div class="temp-card">
        <div style="font-size:12px; color:var(--text-muted);">کوره ۲ (TC2)</div>
        <div class="temp-val temp-c2" id="val-tc2">-- °C</div>
        <div style="font-size:11px;" id="stat-tc2">درحال خواندن...</div>
      </div>
      <div class="temp-card">
        <div style="font-size:12px; color:var(--text-muted);">ساعت DS3231</div>
        <div class="temp-val temp-rtc" id="val-rtctemp">-- °C</div>
        <button class="btn btn-outline" style="margin-top:6px; width:100%; font-size:10px;" onclick="syncRtc()">تنظیم با NTP</button>
      </div>
      <div class="temp-card">
        <div style="font-size:12px; color:var(--text-muted);">پردازنده ESP32</div>
        <div class="temp-val temp-esp" id="val-esptemp">-- °C</div>
        <div style="font-size:11px; color:#94a3b8;">دمای هسته مرکزی</div>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="card-header">
      <div class="card-title">👁️ ورودی‌های ایزوله اپتوکوپلری (I0 تا I7)</div>
      <span class="badge">نردبان آنالوگ GPIO 3..7</span>
    </div>
    <div class="inputs-row" id="opto-container">
      <!-- I0 تا I7 -->
    </div>
  </div>

  <div class="card">
    <div class="card-header">
      <div class="card-title">🔘 کلیدهای ۱۰ گانه پنل جلو (F1..F10)</div>
      <span class="badge">GPIO 1 &amp; GPIO 2</span>
    </div>
    <div style="display:grid; grid-template-columns: repeat(10, 1fr); gap:6px;" id="keys-container">
      <!-- F1 تا F10 -->
    </div>
  </div>
</div>

<!-- تب تجهیزات جانبی -->
<div id="tab-periph" class="tab-content">
  <div class="card">
    <div class="card-header"><div class="card-title">🖨️ پرینتر حرارتی RS485 و هشدار صوتی</div></div>
    <div style="display:grid; grid-template-columns: 1fr 1fr; gap:12px;">
      <div class="ota-box">
        <h3>پرینتر حرارتی کارخانه (UART2 / RS485)</h3>
        <p style="font-size:12px; color:var(--text-muted); margin-bottom:10px;">چاپ لوگوی رسمی هافلر، بارکد، جدول سلامت قطعات و کارنامه QC.</p>
        <button class="btn btn-primary" style="width:100%; padding:10px;" onclick="printTest()">🖨️ چاپ رسید تست کیفی (QC Ticket)</button>
      </div>
      <div class="ota-box">
        <h3>سیستم بازر و تست پیزوالکتریک</h3>
        <p style="font-size:12px; color:var(--text-muted); margin-bottom:10px;">بررسی سلامت مدار تقویت‌کننده ترانزیستوری بازر (GPIO 45).</p>
        <button class="btn btn-warning" style="width:100%; padding:10px;" onclick="testBuzzer()">🔊 پخش بوق آزمایشی (Beep)</button>
      </div>
    </div>
  </div>
</div>

<!-- تب بروزرسانی OTA -->
<div id="tab-ota" class="tab-content">
  <div class="card">
    <div class="card-header"><div class="card-title">🚀 بروزرسانی نرم‌افزار و فلش از سرور یا فایل</div></div>

    <!-- متد ۱: اتصال به سرور ابری هافلر -->
    <div class="ota-box">
      <h3>🌐 متد ۱: دریافت و نصب مستقیم فریمور از سرور هافلر (Cloud OTA)</h3>
      <p style="font-size:12px; color:var(--text-muted); margin-bottom:10px;">اتصال خودکار به مخزن رسمی ArvanCloud هافلر، استعلام آخرین نسخه و فلش روی پارتیشن app0.</p>
      <div class="form-group">
        <label>انتخاب دستگاه و نرم‌افزار هدف:</label>
        <select class="form-control" id="target-model-select">
          <option value="cutter_single_head">اره تک‌سر صنعتی (Cutter Single Head)</option>
          <option value="cutter_double_head">اره دوسر صنعتی (Cutter Double Head)</option>
          <option value="welder_single_head">جوش تک‌سر UPVC (Welder Single Head)</option>
          <option value="welder_double_head">جوش دوسر UPVC (Welder Double Head)</option>
          <option value="gas_filling">دستگاه تزریق گاز آرگون (Gas Filling)</option>
          <option value="dough_divider">دستگاه چانه‌گیر نان (Dough Divider)</option>
          <option value="config-test">برنامه تست کانفیگ و لانچر (Config Launcher)</option>
        </select>
      </div>
      <button class="btn btn-success" style="padding:10px 16px;" onclick="flashTargetServer()">⚡ دریافت و بروزرسانی از سرور هافلر</button>
      <div id="server-ota-status" style="margin-top:8px; font-size:12px;"></div>
    </div>

    <!-- متد ۲: آپلود مستقیم فایل فریمور از کامپیوتر یا موبایل -->
    <div class="ota-box">
      <h3>💻 متد ۲: آپلود مستقیم فایل باینری فریمور (.bin) از کامپیوتر / گوشی</h3>
      <p style="font-size:12px; color:var(--text-muted); margin-bottom:10px;">بدون نیاز به اینترنت! فایل firmware.bin کامپایل‌شده را انتخاب کرده و مستقیماً روی برد آپلود کنید.</p>
      <div class="form-group">
        <input type="file" id="bin-file-input" class="form-control" accept=".bin">
      </div>
      <button class="btn btn-primary" style="padding:10px 16px;" onclick="uploadDirectBin()">📤 شروع فلش و ارسال فایل</button>
      <div class="progress-bar-cont" id="prg-cont">
        <div class="progress-bar-val" id="prg-bar"></div>
      </div>
      <div id="upload-status" style="margin-top:8px; font-size:12px;"></div>
    </div>

    <!-- دکمه ریبوت -->
    <div style="text-align:left; margin-top:14px;">
      <button class="btn btn-danger" style="padding:8px 18px;" onclick="rebootEsp()">🔄 راه‌اندازی مجدد برد (Reboot)</button>
    </div>
  </div>
</div>

<div class="footer">
  HAFFLER INDUSTRIAL AUTOMATION SYSTEMS &bull; HT-V3A CONTROLLER &bull; ALL RIGHTS RESERVED
</div>

<script>
const RELAY_LABELS = [
  { name: "Q0 (Relay 1)", pin: "GPIO 16", desc: "جک نگه‌دارنده ۱ / شیر گاز" },
  { name: "Q1 (Relay 2)", pin: "GPIO 42", desc: "جک نگه‌دارنده ۲ / شیر گاز ۲" },
  { name: "Q2 (Relay 3)", pin: "GPIO 39", desc: "جک تغذیه قطعه / موتور پارو" },
  { name: "Q3 (Relay 4)", pin: "GPIO 48", desc: "موتور تیغه اره / پمپ اصلی" },
  { name: "Q4 (Relay 5)", pin: "GPIO 40", desc: "جک فک متحرک / شیر فرعی" },
  { name: "Q5 (SSR 1)",   pin: "GPIO 15", desc: "المنت حرارتی هیتر ۱ (PWM)" },
  { name: "Q6 (Relay 7)", pin: "GPIO 35", desc: "شیر خنک‌کننده / باد پاش" },
  { name: "Q7 (Relay 8)", pin: "GPIO 36", desc: "حفاظ ایمنی / رله کمکی" },
  { name: "Q8 (Relay 9)", pin: "GPIO 38", desc: "جک بالابر اره / قفل" },
  { name: "Q9 (SSR 2)",   pin: "GPIO 41", desc: "المنت حرارتی هیتر ۲ (PWM)" }
];

function initUI() {
  const rCont = document.getElementById('relay-container');
  rCont.innerHTML = '';
  RELAY_LABELS.forEach((r, i) => {
    rCont.innerHTML += `
      <div class="relay-card" id="rc-${i}">
        <div class="relay-info">
          <div>
            <div class="relay-name">${r.name}</div>
            <div style="font-size:10px; color:#94a3b8;">${r.desc}</div>
          </div>
          <div style="display:flex; align-items:center; gap:6px;">
            <span class="relay-pin">${r.pin}</span>
            <div class="relay-led"></div>
          </div>
        </div>
        <div class="relay-actions">
          <button class="btn btn-primary" onclick="pulseRelay(${i})" id="btn-p-${i}">⚡ پالس ۳ ثانیه</button>
          <button class="btn btn-outline" onclick="toggleRelay(${i})" id="btn-t-${i}">تغییر</button>
        </div>
      </div>
    `;
  });

  const oCont = document.getElementById('opto-container');
  oCont.innerHTML = '';
  for (let i = 0; i < 8; i++) {
    oCont.innerHTML += `
      <div class="input-indicator" id="opto-${i}">
        <div class="input-led"></div>
        <div class="input-lbl">I${i}</div>
        <div class="input-v" id="ov-${i}">-- mV</div>
      </div>
    `;
  }

  const kCont = document.getElementById('keys-container');
  kCont.innerHTML = '';
  for (let i = 1; i <= 10; i++) {
    kCont.innerHTML += `
      <div class="input-indicator" id="key-${i}">
        <div class="input-led"></div>
        <div class="input-lbl">F${i}</div>
      </div>
    `;
  }

  const mCont = document.getElementById('mcp-container');
  mCont.innerHTML = '';
  for (let i = 0; i < 6; i++) {
    mCont.innerHTML += `
      <div class="relay-card" id="mcpc-${i}">
        <div style="display:flex; justify-content:space-between; align-items:center;">
          <b style="font-size:12px;">GPA${i}</b>
          <div class="relay-led" id="mcpled-${i}"></div>
        </div>
        <button class="btn btn-outline" style="margin-top:4px;" onclick="toggleMcp(${i})">سوییچ</button>
      </div>
    `;
  }
}

function switchTab(id) {
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
  event.target.classList.add('active');
  document.getElementById(id).classList.add('active');
}

let lastStatus = null;

async function fetchStatus() {
  try {
    const res = await fetch('/api/hw_status');
    if (!res.ok) return;
    const data = await res.json();
    lastStatus = data;

    document.getElementById('b-ver').textContent = 'VER ' + data.version;
    document.getElementById('st-cpu').textContent = data.cpu_freq_mhz + ' MHz';
    document.getElementById('st-temp').textContent = data.cpu_temp.toFixed(1) + ' °C';
    document.getElementById('st-heap').textContent = Math.round(data.heap_free / 1024) + ' KB';
    document.getElementById('st-psram').textContent = (data.psram_free / (1024*1024)).toFixed(1) + ' MB';
    document.getElementById('st-wifi').textContent = data.wifi_connected ? (data.wifi_ssid + ' (' + data.wifi_rssi + 'dBm)') : 'SoftAP: ' + data.ap_ip;

    const s = data.uptime_sec;
    const hrs = Math.floor(s / 3600);
    const mins = Math.floor((s % 3600) / 60);
    const secs = s % 60;
    document.getElementById('st-uptime').textContent = 
      String(hrs).padStart(2,'0') + ':' + String(mins).padStart(2,'0') + ':' + String(secs).padStart(2,'0');

    // رله‌ها
    data.relays.forEach((st, i) => {
      const card = document.getElementById(`rc-${i}`);
      if (card) {
        if (st) card.classList.add('active');
        else card.classList.remove('active');
      }
    });

    // دماها
    const t1El = document.getElementById('val-tc1');
    const s1El = document.getElementById('stat-tc1');
    if (data.tc1_valid) {
      t1El.textContent = data.tc1_temp.toFixed(1) + ' °C';
      s1El.textContent = 'نرمال و متصل'; s1El.style.color = '#10b981';
    } else {
      t1El.textContent = 'OPEN / N/A';
      s1El.textContent = 'پراب قطع است'; s1El.style.color = '#ef4444';
    }

    const t2El = document.getElementById('val-tc2');
    const s2El = document.getElementById('stat-tc2');
    if (data.tc2_valid) {
      t2El.textContent = data.tc2_temp.toFixed(1) + ' °C';
      s2El.textContent = 'نرمال و متصل'; s2El.style.color = '#10b981';
    } else {
      t2El.textContent = 'OPEN / N/A';
      s2El.textContent = 'پراب قطع است'; s2El.style.color = '#ef4444';
    }

    document.getElementById('val-rtctemp').textContent = data.rtc_temp + ' °C';
    document.getElementById('val-esptemp').textContent = data.cpu_temp.toFixed(1) + ' °C';
    document.getElementById('rtc-time-badge').textContent = 'RTC: ' + data.rtc_time;

    // سنسورهای ورودی I0..I7
    for (let i = 0; i < 8; i++) {
      const el = document.getElementById(`opto-${i}`);
      const on = (data.opto_mask & (1 << i)) !== 0;
      if (on) el.classList.add('on'); else el.classList.remove('on');
    }
    if (data.opto_v && data.opto_v.length >= 5) {
      document.getElementById('ov-0').textContent = data.opto_v[0] + ' ADC';
      document.getElementById('ov-1').textContent = data.opto_v[0] + ' ADC';
      document.getElementById('ov-2').textContent = data.opto_v[1] + ' ADC';
      document.getElementById('ov-3').textContent = data.opto_v[1] + ' ADC';
      document.getElementById('ov-4').textContent = data.opto_v[2] + ' ADC';
      document.getElementById('ov-5').textContent = data.opto_v[2] + ' ADC';
      document.getElementById('ov-6').textContent = data.opto_v[3] + ' ADC';
      document.getElementById('ov-7').textContent = data.opto_v[4] + ' ADC';
    }

    // کلیدهای پنل
    for (let i = 1; i <= 10; i++) {
      const kel = document.getElementById(`key-${i}`);
      const kon = (data.keys_mask & (1 << (i - 1))) !== 0;
      if (kon) kel.classList.add('on'); else kel.classList.remove('on');
    }

    // MCP23017
    const mCard = document.getElementById('mcp-card');
    if (data.mcp_present) {
      mCard.style.display = 'block';
      for (let i = 0; i < 6; i++) {
        const led = document.getElementById(`mcpled-${i}`);
        const mOn = (data.mcp_out & (1 << i)) !== 0;
        if (led) {
          if (mOn) { led.style.background = '#10b981'; led.style.boxShadow = '0 0 8px #10b981'; }
          else { led.style.background = '#334155'; led.style.boxShadow = 'none'; }
        }
      }
    } else {
      mCard.style.display = 'none';
    }

  } catch(e) {
    // خطای لحظه‌ای شبکه
  }
}

async function pulseRelay(ch) {
  try {
    await fetch(`/api/test_relay?ch=${ch}&dur=3000`, { method: 'POST' });
    const btn = document.getElementById(`btn-p-${ch}`);
    if (btn) {
      let left = 3;
      btn.textContent = `⏳ ${left} ثانیه`;
      const iv = setInterval(() => {
        left--;
        if (left <= 0) {
          clearInterval(iv);
          btn.textContent = '⚡ پالس ۳ ثانیه';
        } else {
          btn.textContent = `⏳ ${left} ثانیه`;
        }
      }, 1000);
    }
  } catch(e) { alert('خطا در ارتباط با برد'); }
}

async function toggleRelay(ch) {
  if (!lastStatus) return;
  const cur = lastStatus.relays[ch];
  try {
    await fetch(`/api/toggle_relay?ch=${ch}&state=${cur ? 0 : 1}`, { method: 'POST' });
    fetchStatus();
  } catch(e) { alert('خطا در تغییر وضعیت'); }
}

async function killAll() {
  if (!confirm('آیا از خاموشی اضطراری تمام رله‌ها و خروجی‌ها اطمینان دارید؟')) return;
  try {
    await fetch('/api/kill_all', { method: 'POST' });
    fetchStatus();
  } catch(e) { alert('خطا در خاموشی اضطراری'); }
}

async function toggleMcp(ch) {
  if (!lastStatus) return;
  const cur = (lastStatus.mcp_out & (1 << ch)) !== 0;
  try {
    await fetch(`/api/test_mcp?ch=${ch}&state=${cur ? 0 : 1}`, { method: 'POST' });
    fetchStatus();
  } catch(e) {}
}

async function printTest() {
  try {
    const res = await fetch('/api/print_test', { method: 'POST' });
    const d = await res.json();
    alert('دستور چاپ ارسال شد: ' + d.message);
  } catch(e) { alert('خطا در ارسال دستور پرینتر'); }
}

async function testBuzzer() {
  try {
    await fetch('/api/test_buzzer', { method: 'POST' });
  } catch(e) {}
}

async function syncRtc() {
  try {
    const res = await fetch('/api/sync_rtc', { method: 'POST' });
    const d = await res.json();
    alert(d.message);
  } catch(e) { alert('خطا در همگام‌سازی ساعت'); }
}

async function flashTargetServer() {
  const sel = document.getElementById('target-model-select');
  const model = sel.value;
  const st = document.getElementById('server-ota-status');
  if (!confirm(`آیا از شروع دانلود و نصب فریمور «${sel.options[sel.selectedIndex].text}» از سرور هافلر اطمینان دارید؟`)) return;

  st.style.color = '#38bdf8';
  st.textContent = 'در حال ارسال دستور به پردازنده... لطفا به نمایشگر دستگاه نگاه کنید.';
  try {
    const res = await fetch(`/api/flash_target?model=${encodeURIComponent(model)}`, { method: 'POST' });
    const d = await res.json();
    st.style.color = '#10b981';
    st.textContent = 'دستور تایید شد. فرآیند دانلود در هسته ۰ پردازنده آغاز شد.';
  } catch(e) {
    st.style.color = '#ef4444';
    st.textContent = 'خطا در صدور فرمان دانلود.';
  }
}

function uploadDirectBin() {
  const fi = document.getElementById('bin-file-input');
  const file = fi.files[0];
  const st = document.getElementById('upload-status');
  const pCont = document.getElementById('prg-cont');
  const pBar = document.getElementById('prg-bar');

  if (!file) { alert('لطفا ابتدا یک فایل فریمور با پسوند .bin انتخاب کنید.'); return; }
  if (!confirm(`آیا از فلش فایل ${file.name} (${(file.size / 1024).toFixed(0)} KB) روی دستگاه اطمینان دارید؟`)) return;

  pCont.style.display = 'block';
  pBar.style.width = '0%';
  st.style.color = '#38bdf8';
  st.textContent = 'درحال ارسال فایل به حافظه فلش برد...';

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);
  xhr.setRequestHeader('Content-Type', 'application/octet-stream');

  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      pBar.style.width = pct + '%';
      st.textContent = `درحال آپلود: ${pct}% (${(e.loaded/1024).toFixed(0)} / ${(e.total/1024).toFixed(0)} KB)`;
    }
  };

  xhr.onload = function() {
    if (xhr.status === 200) {
      pBar.style.width = '100%';
      st.style.color = '#10b981';
      st.textContent = '✅ آپلود و نصب با موفقیت پایان یافت! برد در حال راه‌اندازی مجدد است... (۱۰ ثانیه صبر کنید)';
      setTimeout(() => { location.reload(); }, 12000);
    } else {
      st.style.color = '#ef4444';
      st.textContent = '❌ خطا در آپلود و فلش فریمور: ' + xhr.responseText;
    }
  };

  xhr.onerror = function() {
    st.style.color = '#ef4444';
    st.textContent = '❌ قطع ارتباط با برد در حین آپلود.';
  };

  xhr.send(file);
}

async function rebootEsp() {
  if (!confirm('آیا مایل به راه‌اندازی مجدد (Reboot) برد هستید؟')) return;
  try {
    await fetch('/reboot', { method: 'POST' });
    alert('دستور راه‌اندازی ارسال شد. صفحه پس از ۵ ثانیه رفرش می‌شود.');
    setTimeout(() => { location.reload(); }, 5000);
  } catch(e) {}
}

initUI();
fetchStatus();
setInterval(fetchStatus, 600);
</script>
</body>
</html>
)rawliteral";

class HafflerWebPortal {
private:
    WiFiServer *server;
    bool active;

    static int parseQueryInt(const String &url, const String &key, int defaultVal) {
        String search = key + "=";
        int idx = url.indexOf(search);
        if (idx < 0) return defaultVal;
        int end = url.indexOf('&', idx);
        String val = (end > 0) ? url.substring(idx + search.length(), end) : url.substring(idx + search.length());
        return val.toInt();
    }

    static String parseQueryString(const String &url, const String &key, const String &defaultVal) {
        String search = key + "=";
        int idx = url.indexOf(search);
        if (idx < 0) return defaultVal;
        int end = url.indexOf('&', idx);
        String val = (end > 0) ? url.substring(idx + search.length(), end) : url.substring(idx + search.length());
        val.replace("%20", " ");
        val.replace("%2F", "/");
        val.replace("%2f", "/");
        return val;
    }

    static void sendJsonResponse(WiFiClient &client, const String &json, int code = 200) {
        client.printf("HTTP/1.1 %d %s\r\n", code, (code == 200) ? "OK" : "Error");
        client.println("Content-Type: application/json; charset=utf-8");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.printf("Content-Length: %d\r\n\r\n", (int)json.length());
        client.print(json);
    }

    void handleUpdateUpload(WiFiClient &client, size_t contentLength) {
        if (contentLength == 0) {
            sendJsonResponse(client, "{\"status\":\"error\",\"message\":\"Zero content length\"}", 400);
            return;
        }

        Serial.printf("[WEB-OTA] Streaming direct firmware upload (%u bytes)...\n", (unsigned)contentLength);
        if (!Update.begin(contentLength, U_FLASH)) {
            Serial.printf("[WEB-OTA] Update.begin failed: %d\n", (int)Update.getError());
            sendJsonResponse(client, "{\"status\":\"error\",\"code\":" + String(Update.getError()) + "}", 500);
            return;
        }

        const size_t CHUNK_SZ = 2048;
        uint8_t *buf = (uint8_t *)malloc(CHUNK_SZ);
        if (!buf) {
            Update.abort();
            sendJsonResponse(client, "{\"status\":\"error\",\"message\":\"Memory alloc failed\"}", 500);
            return;
        }

        size_t remaining = contentLength;
        uint32_t t_last_data = millis();

        while (remaining > 0 && client.connected()) {
            size_t toRead = (remaining < CHUNK_SZ) ? remaining : CHUNK_SZ;
            int avail = client.available();
            if (avail > 0) {
                size_t actualRead = (avail < (int)toRead) ? avail : toRead;
                int r = client.read(buf, actualRead);
                if (r > 0) {
                    if (Update.write(buf, r) != (size_t)r) {
                        Serial.printf("[WEB-OTA] Update.write failed: %d\n", (int)Update.getError());
                        break;
                    }
                    remaining -= r;
                    t_last_data = millis();
                }
            } else {
                if (millis() - t_last_data > 10000) {
                    Serial.println("[WEB-OTA] Timeout reading upload data");
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        free(buf);

        if (remaining == 0 && Update.end(true)) {
            Serial.println("[WEB-OTA] Firmware upload SUCCESS! Rebooting in 500ms...");
            sendJsonResponse(client, "{\"status\":\"ok\",\"rebooting\":true}");
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP.restart();
        } else {
            Serial.printf("[WEB-OTA] Firmware upload FAILED (remaining=%u, err=%d)\n", (unsigned)remaining, (int)Update.getError());
            Update.abort();
            sendJsonResponse(client, "{\"status\":\"error\",\"code\":" + String(Update.getError()) + "}", 500);
        }
    }

public:
    HafflerWebPortal() : server(nullptr), active(false) {}

    bool isActive() const { return active; }

    void start() {
        if (active) return;
        
        // فعال‌سازی همزمان SoftAP و کلاینت Station
        WiFi.mode(WIFI_AP_STA);
        String mac = WiFi.macAddress();
        String apSuffix = mac.substring(mac.length() - 5);
        apSuffix.replace(":", "");
        String apName = "HAFFLER-PLC-" + apSuffix;
        WiFi.softAP(apName.c_str(), "12345678");

        server = new WiFiServer(80);
        server->begin();
        active = true;
        Serial.println("[WEB-PORTAL] Industrial Web Portal started on " + apName + " (192.168.4.1:80)");
    }

    void stop() {
        if (!active) return;
        if (server) {
            server->stop();
            delete server;
            server = nullptr;
        }
        active = false;
    }

    void tick() {
        if (!active || !server) return;
        WiFiClient client = server->accept();
        if (!client) return;

        // خواندن خط نخست درخواست HTTP
        String reqLine = "";
        uint32_t t_start = millis();
        while (client.connected() && millis() - t_start < 600) {
            if (client.available()) {
                char c = (char)client.read();
                if (c == '\r') continue;
                if (c == '\n') break;
                reqLine += c;
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        if (reqLine.length() == 0) {
            client.stop();
            return;
        }

        int sp1 = reqLine.indexOf(' ');
        int sp2 = (sp1 >= 0) ? reqLine.indexOf(' ', sp1 + 1) : -1;
        if (sp1 < 0) { client.stop(); return; }
        String method = reqLine.substring(0, sp1);
        String path = (sp2 > 0) ? reqLine.substring(sp1 + 1, sp2) : reqLine.substring(sp1 + 1);

        // بررسی هدرها (به ویژه Content-Length برای متد POST)
        size_t contentLength = 0;
        String curHeader = "";
        uint32_t h_start = millis();
        while (client.connected() && millis() - h_start < 800) {
            if (client.available()) {
                char c = (char)client.read();
                if (c == '\r') continue;
                if (c == '\n') {
                    if (curHeader.length() == 0) break; // پایان هدرها (\r\n\r\n)
                    if (curHeader.startsWith("Content-Length:") || curHeader.startsWith("content-length:")) {
                        contentLength = (size_t)curHeader.substring(15).toInt();
                    }
                    curHeader = "";
                } else {
                    curHeader += c;
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        // ۱. صفحه اصلی داشبورد
        if (method == "GET" && (path == "/" || path == "/index.html")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html; charset=utf-8");
            client.println("Connection: close");
            client.printf("Content-Length: %u\r\n\r\n", sizeof(PAGE_DASHBOARD_HTML) - 1);
            client.write_P(PAGE_DASHBOARD_HTML, sizeof(PAGE_DASHBOARD_HTML) - 1);
        }
        // ۲. تله‌متری و وضعیت زنده
        else if (method == "GET" && path.startsWith("/api/hw_status")) {
            char rtc_dt[32] = {0};
            float rtc_temp = 0.0f;
            hwtest_get_rtc_info(rtc_dt, sizeof(rtc_dt), &rtc_temp);

            int opto_v[5] = {0};
            hwtest_get_opto_voltages(opto_v);

            String json = "{";
            json += "\"version\":\"" VERSION "\",";
            json += "\"uptime_sec\":" + String(millis() / 1000) + ",";
            json += "\"cpu_freq_mhz\":" + String(getCpuFrequencyMhz()) + ",";
            json += "\"cpu_temp\":" + String(hwtest_get_esp_temp(), 1) + ",";
            json += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
            json += "\"psram_free\":" + String(ESP.getFreePsram()) + ",";
            json += "\"wifi_connected\":" + String((WiFi.status() == WL_CONNECTED) ? "true" : "false") + ",";
            json += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
            json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
            json += "\"wifi_ip\":\"" + WiFi.localIP().toString() + "\",";
            json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
            json += "\"mac\":\"" + WiFi.macAddress() + "\",";

            // رله‌های ده‌گانه Q0..Q9
            json += "\"relays\":[";
            for (int i = 0; i < 10; i++) {
                json += (hwtest_get_relay_state((uint8_t)i) ? "true" : "false");
                if (i < 9) json += ",";
            }
            json += "],";

            // دماسنج‌های MAX6675
            json += "\"tc1_temp\":" + String(hwtest_get_tc1_temp(), 2) + ",";
            json += "\"tc1_valid\":" + String(hwtest_get_tc1_valid() ? "true" : "false") + ",";
            json += "\"tc2_temp\":" + String(hwtest_get_tc2_temp(), 2) + ",";
            json += "\"tc2_valid\":" + String(hwtest_get_tc2_valid() ? "true" : "false") + ",";

            // ساعت RTC
            json += "\"rtc_time\":\"" + String(rtc_dt) + "\",";
            json += "\"rtc_temp\":" + String(rtc_temp, 1) + ",";

            // MCP23017
            json += "\"mcp_present\":" + String(g_mcp23017_present ? "true" : "false") + ",";
            uint8_t mcp_out = 0;
            if (g_mcp23017_present) {
                for (uint8_t i = 0; i < 6; i++) {
                    if (hwtest_mcp23017_get_output(i)) mcp_out |= (1 << i);
                }
            }
            json += "\"mcp_out\":" + String(mcp_out) + ",";
            json += "\"mcp_in\":" + String(g_mcp23017_present ? hwtest_mcp23017_poll_inputs() : 0) + ",";

            // ورودی‌های اپتوکوپلری I0..I7
            json += "\"opto_mask\":" + String(hwtest_get_opto_inputs()) + ",";
            json += "\"opto_v\":[";
            for (int i = 0; i < 5; i++) {
                json += String(opto_v[i]);
                if (i < 4) json += ",";
            }
            json += "],";

            // کلیدهای پنل جلو
            json += "\"keys_mask\":" + String(hwtest_get_panel_keys()) + ",";
            json += "\"is_hwtest\":" + String(g_hwtest_active ? "true" : "false");
            json += "}";

            sendJsonResponse(client, json);
        }
        // ۳. پالس ۳ ثانیه‌ای رله
        else if (path.startsWith("/api/test_relay")) {
            int ch = parseQueryInt(path, "ch", 0);
            int dur = parseQueryInt(path, "dur", 3000);
            if (dur == 0 || dur > 3000) dur = 3000;
            hwtest_pulse_relay((uint8_t)ch, (uint32_t)dur);
            sendJsonResponse(client, "{\"status\":\"ok\",\"channel\":" + String(ch) + ",\"duration\":" + String(dur) + "}");
        }
        // ۴. تغییر وضعیت رله
        else if (path.startsWith("/api/toggle_relay")) {
            int ch = parseQueryInt(path, "ch", 0);
            int st = parseQueryInt(path, "state", 1);
            hwtest_set_relay_state((uint8_t)ch, st != 0);
            sendJsonResponse(client, "{\"status\":\"ok\",\"channel\":" + String(ch) + ",\"state\":" + String(st) + "}");
        }
        // ۵. خاموشی اضطراری همه
        else if (path.startsWith("/api/kill_all")) {
            hwtest_kill_all_outputs();
            sendJsonResponse(client, "{\"status\":\"ok\",\"message\":\"All outputs killed\"}");
        }
        // ۶. سوییچ MCP23017
        else if (path.startsWith("/api/test_mcp")) {
            int ch = parseQueryInt(path, "ch", 0);
            int st = parseQueryInt(path, "state", 1);
            if (g_mcp23017_present) hwtest_mcp23017_set_output((uint8_t)ch, st != 0);
            sendJsonResponse(client, "{\"status\":\"ok\",\"channel\":" + String(ch) + ",\"state\":" + String(st) + "}");
        }
        // ۷. تست بازر
        else if (path.startsWith("/api/test_buzzer")) {
            hwtest_beep_click(150);
            sendJsonResponse(client, "{\"status\":\"ok\",\"message\":\"Buzzer tested\"}");
        }
        // ۸. چاپ تست پرینتر
        else if (path.startsWith("/api/print_test")) {
            hwtest_print_haffler();
            sendJsonResponse(client, "{\"status\":\"ok\",\"message\":\"QC Print ticket sent\"}");
        }
        // ۹. همگام‌سازی ساعت با اینترنت
        else if (path.startsWith("/api/sync_rtc")) {
            if (WiFi.status() == WL_CONNECTED) {
                app_trigger_ntp_sync();
                sendJsonResponse(client, "{\"status\":\"ok\",\"message\":\"NTP sync triggered\"}");
            } else {
                sendJsonResponse(client, "{\"status\":\"error\",\"message\":\"WiFi not connected to Internet\"}", 400);
            }
        }
        // ۱۰. دانلود و فلش از سرور ابری هافلر
        else if (path.startsWith("/api/flash_target")) {
            String model = parseQueryString(path, "model", "config-test");
            if (xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(500))) {
                ota_download_model = model;
                ota_download = true;
                xSemaphoreGive(xMutex_Wifi);
                sendJsonResponse(client, "{\"status\":\"ok\",\"message\":\"OTA download initiated for " + model + "\"}");
            } else {
                sendJsonResponse(client, "{\"status\":\"error\",\"message\":\"WiFi Mutex busy\"}", 500);
            }
        }
        // ۱۱. دانلود از URL دلخواه
        else if (path.startsWith("/api/flash_url")) {
            String url = parseQueryString(path, "url", "");
            if (url.length() > 0 && xSemaphoreTake(xMutex_Wifi, pdMS_TO_TICKS(500))) {
                ota_custom_url = url;
                ota_custom_download = true;
                xSemaphoreGive(xMutex_Wifi);
                sendJsonResponse(client, "{\"status\":\"ok\",\"message\":\"Custom URL OTA initiated\"}");
            } else {
                sendJsonResponse(client, "{\"status\":\"error\",\"message\":\"Empty URL or Mutex busy\"}", 400);
            }
        }
        // ۱۲. آپلود مستقیم فایل فریمور از مرورگر
        else if (method == "POST" && path.startsWith("/update")) {
            handleUpdateUpload(client, contentLength);
        }
        // ۱۳. راه‌اندازی مجدد
        else if (path.startsWith("/reboot")) {
            sendJsonResponse(client, "{\"status\":\"ok\",\"rebooting\":true}");
            vTaskDelay(pdMS_TO_TICKS(400));
            ESP.restart();
        }
        else {
            sendJsonResponse(client, "{\"status\":\"error\",\"message\":\"Not found\"}", 404);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
        client.stop();
    }
};

extern HafflerWebPortal g_web_portal;

#endif // WEB_PORTAL_H
