# Gas Filling Machine Scenario

This document explains what the current firmware does from an operator and code-mapping point of view.

## Product Purpose

The machine is an argon gas filling HMI. The Main screen controls two independent filling channels:

- Left channel: dimensions and filling time for Valve 1 on GPIO16.
- Right channel: dimensions and filling time for Valve 2 on GPIO42.

Each channel accepts Length, Height, and Width. The firmware calculates gas volume and filling time from dimensions, pressure ratio, and flow rate. When the operator presses Start, the selected valve is active for the calculated time and the progress bar moves toward 100%.

## Boot Flow

Current boot flow:

1. Serial starts.
2. Valve and buzzer pins are configured.
3. EEPROM is initialized.
4. TFT_eSPI starts the ST7796 display.
5. LVGL display and touch input are registered.
6. The generated UI initializes.
7. Main screen appears.
8. The agent protocol becomes available on serial.

## Main Screen

The Main screen title is `Argon Gas Filler`. A settings icon appears at the top right. The screen is split into two independent process channels.

### Left Channel

Inputs:

- Length
- Height
- Width

Actions:

- Start left: sets `TimerValve1 = TimerValve1_Saved`.
- Stop left: sets `TimerValve1 = 0` and resets the left progress bar.

Output:

- Valve 1 on GPIO16.

### Right Channel

Inputs:

- Length
- Height
- Width

Actions:

- Start right: sets `TimerValve2 = TimerValve2_Saved`.
- Stop right: sets `TimerValve2 = 0` and resets the right progress bar.

Output:

- Valve 2 on GPIO42.

## Filling Formula

Current formula pattern:

```text
V = Length * Height * Width / 1000
Gas = (P1 / P2) * V
time_sec = (Gas / Qs) * 60
TimerValveSaved = (int)time_sec * 10
```

Timer units are 100 ms ticks. The expression `(int)time_sec * 10` truncates seconds before multiplying; this is a known precision issue to fix later.

## Settings Icon

The top-right icon is:

```text
objects.ib_setting1
img_setting1
action_gotosetting()
```

It opens `SCREEN_ID_SETTING`. The corrupted top-right icon issue was caused by LVGL 9 `RGB565A8` image layout mismatch and has been fixed by converting all `ui_image_*.c` files.

## Setting Screen

The Setting screen is created by `create_screen_setting()`. The top-right back button returns to Main through `action_gotohome()`.

### General Tab

Controls (Phase-1 card layout):

- Language dropdown (`English / Persian / Turkish / Russian`)
- Sound (buzzer enable) — EEPROM `EEprom3_Buzzer`
- Touch feedback (UI tap beeps only; never disables capacitive touch) — EEPROM `EEprom4_TouchAvailable`
- Firmware version whisper under the card

Notes:

- Language support exists in code but Persian strings in `actions.c` have encoding/mojibake risk.
- UI key beeps require both Sound and Touch feedback ON (`buzzer_key` gates on both).
- Breath orb UI remains deferred (`ui_breath.c`); Phase-1 uses `ui_polish_general_tab()` only.

### Process Tab

Controls:

- Flow Rate `Qs`
- Hidden pressure values `P1` and `P2`
- Output type N.O/N.C
- Save

N.O/N.C behavior:

- When the switch is off: label is `N.O`, `vr_OutPutType = 0`.
- When the switch is on: label is `N.C`, `vr_OutPutType = 1`.
- During active valve time: `digitalWrite(pin, vr_OutPutType)`.
- During inactive time: `digitalWrite(pin, !vr_OutPutType)`.

The Save button currently hides the keyboard; do not assume it persists all process parameters until the code is expanded and verified.

### Test Tab

Displayed inputs:

- Start
- Stop

Displayed outputs:

- Valve 1
- Valve 2
- BuzzerOut

Current limitation: output switches currently beep but do not safely actuate real outputs as a full diagnostic mode. Use the serial `PULSE1`, `PULSE2`, and `BUZ` commands only with explicit user permission.

### Guide Tab

The guide describes I/O naming:

- `Q1` Start
- `Q2` Stop
- `Q3` Valve 1
- `Q4` Valve 2
- `Q5` BuzzerOut

Hardware input reading is not currently implemented as a complete input subsystem.

## Runtime Timer Behavior

A timer interrupt runs every 100 ms and decrements valve timers. When timers are active, the related output pin is written according to the configured output polarity.

Known risk: the timer ISR currently touches LVGL/UI state through functions such as `ChangeBar`, `ChangeBar1`, and `HideLogo`. LVGL is not ISR-safe. A future fix should move UI updates into the main loop using flags or counters.
