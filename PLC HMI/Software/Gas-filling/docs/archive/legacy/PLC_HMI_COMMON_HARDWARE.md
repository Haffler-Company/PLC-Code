# PLC/HMI Common Hardware Notes

The Gas Filling project belongs to the same Haffler PLC/HMI family as the cutter and welding projects. Many lessons transfer, but pin mappings and machine logic must be verified per project.

## Common Patterns

| Area | Shared lesson |
| --- | --- |
| MCU | ESP32-S3 family hardware. |
| LCD | 480x320 TFT panels are sensitive to driver, color order, byte swap, and rotation. |
| GUI | LVGL-based HMIs with generated UI files. |
| Touch | Capacitive touch over I2C. |
| Outputs | GPIO outputs often need N.O/N.C polarity handling. |
| Debug | Serial commands and real LCD screenshots are extremely valuable. |
| Text | Multilingual text is sensitive to UTF-8 encoding and font coverage. |

## Gas-Specific Pins

| Signal | Pin |
| --- | ---: |
| Touch SDA | GPIO8 |
| Touch SCL | GPIO9 |
| Touch INT | GPIO40 |
| Touch RST | GPIO39 |
| Valve 1 | GPIO16 |
| Valve 2 | GPIO42 |
| Buzzer | GPIO45 |

## Rules

- Do not move pins based only on another project.
- Do not copy cutter or welding process logic into the gas machine.
- Use reference projects to understand display setup, screenshot capture, and embedded HMI polish.
- Verify every display or output change on the actual target hardware.
