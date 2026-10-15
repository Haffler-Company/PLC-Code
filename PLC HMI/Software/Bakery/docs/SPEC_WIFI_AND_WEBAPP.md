# Implementation Blueprint: Embedded Mobile WiFi WebApp & Dashboard

**Document ID:** SPEC-002  
**Target Subsystem:** Connectivity, Mobile Monitoring & Embedded Web Server  
**Dependencies:** `WiFi.h`, `ESPAsyncWebServer.h` (or lightweight `WebServer.h`), `DNSServer.h`, `ArduinoJson.h`  
**Benchmarked Standard:** `Welding_single_head/src/web_app`

---

## 1. Executive Summary & Objective

In industrial bakery environments, operators and business owners frequently need to inspect production throughput (portion counts, shift progress, dough weight, cycle pace) without physically standing next to flour dust or interfering with machine operators.

This specification provides the blueprint to:
1. Launch an embedded Wi-Fi Access Point (`Haffler-Bakery-XXXX`) with captive portal.
2. Serve an ultra-fast, single-page, responsive WebApp optimized for mobile smartphones.
3. Provide live bi-directional REST / WebSocket endpoints for real-time monitoring and parameter tuning.

---

## 2. Network Topology & Access Point Configuration

### 2.1. Soft-AP Credentials:
- **SSID:** `Haffler_Bakery_<ChipMAC>` (e.g. `Haffler_Bakery_A4F2`)
- **Password (WPA2):** `haffler1234` (configurable via Settings or NVS)
- **IP Address:** `192.168.4.1`
- **Subnet:** `255.255.255.0`
- **DNS Server:** Port 53 running captive portal redirecting any browser request to `http://192.168.4.1`.

### 2.2. Initialization Code Pattern:
```cpp
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);

void initWiFiWebApp() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    
    String apName = "Haffler_Bakery_" + String((uint32_t)ESP.getEfuseMac(), HEX).substring(0, 4);
    WiFi.softAP(apName.c_str(), "haffler1234");
    
    dnsServer.start(DNS_PORT, "*", apIP);
    setupWebRoutes();
    server.begin();
}
```

---

## 3. REST API Endpoints Specification

| Endpoint | Method | Response Payload | Description |
|---|---|---|---|
| `/` | GET | `text/html` (Gzipped) | Serves the compressed single-page dashboard HTML/CSS/JS |
| `/api/status` | GET | `application/json` | Returns live machine status, portion tally, active screen, weight, pace |
| `/api/set` | POST | `application/json` | Updates parameters (`weight`, `speed`) from authenticated mobile client |
| `/api/reset_counter`| POST | `application/json` | Clears portion counter (requires PIN) |

### 3.1. `/api/status` JSON Schema:
```json
{
  "running": true,
  "portions_count": 1420,
  "weight_ms": 350,
  "speed_ms": 600,
  "hopper_state": "HOME",
  "piston_state": "DISCHARGING",
  "temp_c": 28.5,
  "uptime_sec": 14320,
  "firmware_version": "1.1.0"
}
```

---

## 4. Single-Page Application (SPA) Design

The HTML/CSS/JS is stored as a GZipped raw byte array in PROGMEM or inside the SPIFFS filesystem:

- **Apple HIG Styling:** Clean dark/titanium theme with large legible typography, high contrast, smooth gauges, and responsive flexbox layout.
- **Polling Interval:** The client polls `/api/status` every 1000ms using Fetch API.
- **Controls Available on Mobile:**
  - Start / Stop button (with confirmation modal)
  - Increment / decrement buttons for portion weight
  - Real-time animated portion counter
  - Shift summary card (Morning shift tally, afternoon shift tally).
