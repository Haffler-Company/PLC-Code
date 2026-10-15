# PLC HMI Master Reference

This file is the shared hardware reference for all future projects that use the HAFFLER PLC HMI platform.

Rule:
- This document stores physical hardware mapping and confirmed platform facts.
- Project-specific roles can change per project and must be documented separately.
- If a new board revision differs from this file, record the mismatch before editing project logic.

## 1) Board Family

- Board family name: `HAFFLER PLC HMI`
- Usage: industrial HMI / PLC controller platform
- UI stack used in the current family: `LVGL` + `TFT_eSPI` + `bb_captouch`

## 2) Confirmed Touch Baseline

- Touch uses only:
  - `GPIO8` = `SDA`
  - `GPIO9` = `SCL`
- No separate touch `RST` or `INT` is confirmed on the current baseline.
- Do not assume extra touch wires unless a PCB revision proves them.

## 3) Confirmed GPIO Baseline

### Outputs
- `GPIO16` -> `Q0`
- `GPIO42` -> `Q1`
- `GPIO39` -> `Q2`
- `GPIO48` -> `Q3`
- `GPIO40` -> `Q4`
- `GPIO15` -> `Q5`
- `GPIO35` -> `Q6`
- `GPIO36` -> `Q7`
- `GPIO38` -> `Q8`
- `GPIO41` -> `Q9`

### Inputs
- `GPIO3` -> `I6`
- `GPIO7` -> `I7`

### Enable / Control
- `GPIO46` -> enable line `ENB`

### Display / Touch SPI
- `GPIO12` -> TFT `SCLK`
- `GPIO11` -> TFT `MISO`
- `GPIO10` -> TFT `MOSI`
- `GPIO13` -> TFT `CS`
- `GPIO14` -> TFT `DC`
- `GPIO8` -> touch `SDA`
- `GPIO9` -> touch `SCL`

### UART / Service
- `GPIO17` -> UART2 `TX`
- `GPIO18` -> UART2 `RX`

### Other Baseline Pins
- `GPIO45` -> buzzer

## 4) MAX6675 Baseline

- `MAX6675_1`
  - `MISO` -> `GPIO37`
  - `SCK` -> `GPIO47`
  - `CS` -> `GPIO21`
- `MAX6675_2`
  - `MISO` -> `GPIO37`
  - `SCK` -> `GPIO47`
  - `CS` -> `GPIO0`

## 5) Project Notes From Current Cutter Branch

These are confirmed for the current cutter firmware branch, not necessarily for every future project.

- `GPIO6` -> `U_SWITCH_PIN`
- `GPIO4` -> joystick analog ladder
- `GPIO5` -> start/stop analog ladder
- `GPIO1` -> F-key analog ladder
- `GPIO2` -> menu-key analog ladder
- Wi-Fi AP SSID: `ESP32-UPLOAD`
- Wi-Fi AP password: `12345678`
- HTTP upload route: `/upload`
- Motion parameters used in the cutter branch:
  - `drv_ppr = 6400`
  - `Axis_Pitch = 20`

## 6) Confirmed Operating Rules

- Treat the PLC HMI pin map as hardware baseline only.
- Do not lock a GPIO to a software role globally unless the specific project requires it.
- Always verify:
  - touch wiring
  - RS485 pair
  - MAX6675 wiring
  - emergency chain outputs
  - upload/programming path
  before finalizing a project.

## 7) Known Unknowns

These are not yet universally confirmed across all board revisions:
- exact MCU/package variant
- exact RS485 transceiver and pin pair
- full power tree
- exact LCD panel model for all revisions
- any extra board revision-specific reset or interrupt lines

## 8) Usage Rule For Future Projects

When starting a new project on this hardware:
1. Read this file first.
2. Compare the project firmware pin map against this baseline.
3. Record only the mismatches and project-specific roles in the project docs.
4. Keep the master reference hardware-only unless a reliable new board fact is confirmed.

