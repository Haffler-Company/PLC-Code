# Implementation Blueprint: Dual-Slot Web OTA Firmware Flashing

**Document ID:** SPEC-003  
**Target Subsystem:** Remote Firmware Maintenance & OTA Flash Management  
**Partition Table:** `partitions.csv` (16MB layout with `app0` at `0x10000` and `app1` at `0x710000`)  
**Standard:** Espressif ESP-IDF OTA Subsystem & Fail-Safe Bootloader Rollback

---

## 1. Executive Summary & Objective

In production bakery environments, airborne flour dust and high humidity make opening the electrical enclosure to connect micro-USB/Type-C cables dangerous and prone to port damage.

This specification provides the blueprint to:
1. Enable wireless firmware updating directly from any web browser on a smartphone or laptop.
2. Utilize the dual-slot 7MB OTA partitions (`app0` and `app1`).
3. Enforce cryptographic binary integrity checking, hardware model validation, and automated rollback if the new firmware fails to boot.

---

## 2. Flash Memory Partition Architecture

From `partitions.csv`:
```csv
# Name,     Type, SubType, Offset,   Size
nvs,        data, nvs,     0x9000,   0x5000,
otadata,    data, ota,     0xE000,   0x2000,
app0,       app,  ota_0,   0x10000,  0x700000,
app1,       app,  ota_1,   0x710000, 0x700000,
spiffs,     data, spiffs,  0xE20000, 0x1E0000
```

- When running from `app0`, incoming updates are written into `app1`.
- When running from `app1`, incoming updates are written into `app0`.
- The active partition is managed by the ESP32 secondary bootloader via `otadata`.

---

## 3. Web Flashing Endpoints & Protocol

### 3.1. Web Interface Route: `/update`
- Protected by Technician PIN (`123456`).
- Presents a clean HTML form with drag-and-drop file upload for `firmware.bin`.
- Shows real-time upload progress bar (0% to 100%).

### 3.2. Server-Side Handler Implementation Blueprint:
```cpp
#include <Update.h>
#include "esp_ota_ops.h"

void setupOTARoutes(WebServer &server) {
    server.on("/update", HTTP_GET, [&server]() {
        server.send(200, "text/html", R"rawliteral(
            <!DOCTYPE html>
            <html>
            <head><title>Haffler Bakery OTA</title></head>
            <body style="font-family:sans-serif; text-align:center; padding:2rem;">
                <h2>Bakery HMI Firmware Update</h2>
                <form method="POST" action="/update" enctype="multipart/form-data">
                    <input type="file" name="update" accept=".bin"><br><br>
                    <input type="submit" value="Flash Firmware" style="padding:10px 20px;">
                </form>
            </body>
            </html>
        )rawliteral");
    });

    server.on("/update", HTTP_POST, [&server]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "UPDATE FAILED" : "UPDATE SUCCESS. REBOOTING...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
    }, [&server]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update Start: %s\n", upload.filename.c_str());
            
            // Stop all motors before flashing
            pistonStop();
            funneleStop();
            
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %u bytes\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
}
```

---

## 4. Fail-Safe Boot Verification & Rollback Protection

To prevent bricking if buggy firmware crashes during boot:
1. In `setup()`, call `esp_ota_mark_app_valid_cancel_rollback()` only after display, storage, and motors are successfully initialized.
2. If the new firmware crashes before reaching this line, the ESP-IDF bootloader automatically reverts to the previous working partition upon next restart.
