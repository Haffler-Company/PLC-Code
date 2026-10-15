# Embedded WebApp Resilience & Industrial Communications
## Zero-Hang WebServer on ESP32-S3, Internal DRAM Protection, Wi-Fi, Bluetooth BLE, and RS485 Modbus

> **Document ID:** `05_EMBEDDED_WEBAPP_AND_CONNECTIVITY.md`  
> **Target Hardware:** Haffler Universal Integrated PLC + HMI Controller.  
> **Standards:** Industrial Internet of Things (IIoT), Embedded WebPortal Hardening, and Modbus RTU Fieldbus.

---

## 1. The 5 Root Causes of Embedded WebApp Crashes & Disconnections

Field testing across hundreds of industrial machines has pinpointed why microcontrollers freeze or disconnect when running web interfaces:

```
+-------------------------------------------------------------------------------+
|                       THE 5 ROOT CAUSES OF EMBEDDED WEBAPP FAILURES           |
+---+----------------------------+----------------------------------------------+
| # | Critical Failure Mode      | Impact & Engineering Solution                |
+---+----------------------------+----------------------------------------------+
| 1 | Internal DRAM Exhaustion   | Free SRAM drops below 40KB $\rightarrow$ TCP |
|   |                            | refuses sockets $\rightarrow$ 100% GZIP (<16KB) |
+---+----------------------------+----------------------------------------------+
| 2 | Mobile Captive Probe Drops | Android/iOS drop Wi-Fi without internet      |
|   |                            | $\rightarrow$ Return instant 204 No Content  |
+---+----------------------------+----------------------------------------------+
| 3 | TCP Socket Leak            | Sockets left open $\rightarrow$ Cap at 4     |
|   |                            | sockets with 5-second idle timeout           |
+---+----------------------------+----------------------------------------------+
| 4 | Core Cross-Contamination   | Web handlers calling blocking motor code     |
|   |                            | $\rightarrow$ Web runs strictly on Core 0    |
+---+----------------------------+----------------------------------------------+
| 5 | External CDN Dependencies  | Browser fails to load external CSS/JS fonts  |
|   |                            | $\rightarrow$ 100% self-contained bundle     |
+---+----------------------------+----------------------------------------------+
```

![Haffler Industrial Mobile WebApp Portal](./assets/images/mobile_webapp_portal.png)

---

## 2. Hardened Zero-Hang Web Server Implementation Rules

### 2.1. 100% GZIP Compression with $< 16\text{ KB}$ Hard Ceiling
HTML, CSS, and JavaScript files must be bundled into a single file and compressed using maximum GZIP compression (level 9) to under 16,384 bytes.  
The server must send the explicit header `Content-Encoding: gzip`:

```c
static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}
```

### 2.2. Guarding Free Internal DRAM ($> 40\text{ KB}$)
The LwIP network stack and Wi-Fi drivers require contiguous internal SRAM buffers to negotiate TCP handshakes:
* Large LVGL display buffers and asset caches must reside in external 8MB PSRAM.
* Prior to processing heavy HTTP requests, verify internal memory health:
  ```c
  if (esp_get_free_internal_heap_size() < 40000) {
      ESP_LOGW("WEB", "Low DRAM warning (<40KB)! Refusing heavy asset request");
      httpd_resp_send_500(req);
      return ESP_FAIL;
  }
  ```

### 2.3. Immediate Mobile Captive Portal 204 Responses
Android and iOS devices probe specific endpoints upon associating with an access point to check for Internet access:
- Android: `/generate_204`
- iOS / macOS: `/hotspot-detect.html` or `/canonical.html`
- Windows: `/ncsi.txt`

The controller must respond with HTTP `204 No Content` immediately:
```c
static esp_err_t captive_204_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}
```
This prevents smartphones from disconnecting from the machine's local access point.

---


### 2.4. FreeRTOS WiFi Task Stack Size & Heap Protection (Anti-Crash Mandate)
A common cause of boot failures and runtime crashes on the ESP32-S3 is overallocating stack to transient network tasks:
* `usStackDepth` in ESP-IDF FreeRTOS is measured in **4-byte WORDS**, NOT bytes!
* Allocating 8,192 words (32KB) or 12,288 words (48KB) to `WifiConn` or `WifiScanStart` causes `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` heap exhaustion and triggers the runtime yellow warning banner `"Task Allocation Failed - Memory Starvation"`.
* **Mandatory Rule:** Stack for `WifiConn` and `WifiScanStart` MUST be capped at **3072 words** (12KB) with a fallback to **2048 words** (8KB).
* The steady-state WebApp task must be capped at **8192 words** (32KB) to reserve at least 40KB of internal DRAM for the LwIP TCP/IP stack.
* Always release the SoftAP and free past scan results before task creation:
  ```cpp
  webApp.wStopAp();
  WiFi.scanDelete();
  ```
* When reconfiguring Wi-Fi credentials, invoke `WiFi.begin(ssid, pass, 0, NULL, false)` with `connect=false` before calling `esp_wifi_set_config` to prevent `ESP_ERR_WIFI_STATE` handshaking errors.
* **SPIFFS Partition Mandate:** The embedded web application bundle resides in flash at offset `0xE20000` (SPIFFS partition). Any production release or factory reflash must include `spiffs.bin` alongside `firmware.bin` to prevent HTTP 503 asset errors.

---

## 3. Real-Time Non-Blocking WebSocket Telemetry

* Real-time machine monitoring from mobile dashboards communicates over a **WebSocket** channel on port 80.
* **Update Cadence:** Broadcast a lightweight JSON telemetry packet every 250ms containing current machine state, dimensions, temperatures, and I/O masks:
  ```json
  {"status":"RUNNING","len":1240.5,"temp":245.2,"in":15,"out":3,"err":0}
  ```
* Telemetry operates purely asynchronously and never delays Core 1 motor or relay cycles.

---

## 4. Complete Wireless & Industrial Fieldbus Suite

### 4.1. Dual Wi-Fi Modes
1. **Local Access Point (SoftAP):**
   - Default SSID: `HAFFLER-[MODEL]-[MAC_LAST_4]` (e.g. `HAFFLER-CUTTER-4B12`).
   - Default IP: `192.168.4.1` or local mDNS `http://haffler.local`.
2. **Factory Station Mode (STA):**
   - Connects to factory Wi-Fi for centralized management, remote diagnostics, and recipe transfer.

### 4.2. Bluetooth Low Energy (BLE 5.0)
* Instant configuration of Wi-Fi credentials, machine passwords, and reading error codes without pairing or typing IP addresses.

### 4.3. Industrial RS485 Fieldbus (Modbus RTU)
* Hardware UART2 transceiver connected to `GPIO 43` (TX), `GPIO 44` (RX), and `GPIO 46` (DE/RE).
* **Industrial Use Cases:**
  1. **Motor Inverter (VFD) Control:** Direct Modbus control of spindle frequency, acceleration ramps, and torque feedback.
  2. **Industrial Thermal Label Printers:** Automatic printing of adhesive production labels containing Solar Hijri date, piece dimensions, operator name, and QC barcode.
  3. **SCADA / Factory MES Integration.**
