# Operations and safe development

## Baseline

- ESP32-S3, 16 MB flash, custom `partitions_16MB_2x7M_ota.csv`.
- ESP-IDF Component Manager dependencies are pinned in `dependencies.lock`.
- LVGL 9.4 HMI on ILI9488 LCD with GT911 touch.

## Build

Build only when requested:

```powershell
idf.py set-target esp32s3
idf.py build
```

Do not erase flash, clear NVS, alter partitions, or upload firmware without direct owner approval. A successful build is not a hardware validation.

## Architecture

`main/main.c` initializes storage, output and input GPIO, Wi-Fi, timers, LCD/touch, RTC, and LVGL. Keep control paths bounded; touch callbacks must not wait for an actuator. Review an affected I/O, timer, and fault path together for any machine-control change.

## Repository hygiene

`build/` and `managed_components/` are locally regenerated and are ignored. Preserve `dependencies.lock`; keep logs, debugger state, caches, and temporary exports out of commits.

## Continuous Learning & Root-Cause Postmortem Ledger

For deep architectural failure modes, postmortems, network socket race conditions, captive portal traps, and empirical root-cause solutions, see:
- [CONTINUOUS_LEARNING_AND_PERPETUAL_POSTMORTEM_LEDGER.md](file:///c:/Users/Haffler/Documents/Haffler/PLC/PLC%20HMI/Software/ICE%20CREEAM/docs/CONTINUOUS_LEARNING_AND_PERPETUAL_POSTMORTEM_LEDGER.md)

## WebPortal, Captive DNS & Network Tuning Architecture

- **Probe Handling:** `/redirect` and `/success.txt` return `302 Found` to `http://192.168.4.1/`. `/wpad.dat` returns `204 No Content`. Foreign host probe requests do not receive the 84KB GZIP application payload.
- **DNS Server (RFC-1035):** Captive DNS strips variable-length EDNS0 OPT pseudo-RRs and returns immediate `NODATA` (`ANCOUNT=0, NOERROR`) for IPv6 `AAAA` and type 65 `HTTPS` queries.
- **Wi-Fi Buffers:** Wi-Fi TX buffers must remain at default 16 static + 32 cache buffers to support `CONFIG_LWIP_TCP_SND_BUF_DEFAULT=65534` without tail-drop packet loss.
- **Serial Communication:** Python tools communicating with the ESP32-S3 native USB-Serial/JTAG port must set `s.dtr = None; s.rts = None` before `s.open()` to avoid involuntary device resets.
- **Visual Verification:** Live screen verification must be conducted using TURBOSHOT RLE capture (`tools/capture_screen_live.py`) with zero impact on real-time PLC tasks.

