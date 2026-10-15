---
name: gstack-tech-lead
description: >-
  Technical Lead & Architect role for Gas Filling firmware: FreeRTOS tasks,
  hardware ISR decoupling, LVGL 9 display pipeline, memory boundaries, and risk assessment.
---

# gstack Tech Lead — Gas Filling HMI

## اهداف نقش
- حفظ یکپارچگی معماری فریم‌ور، ایزولاسیون کامل لایه‌ها و مدیریت همزمانی.
- تضمین عدم تداخل وقفه ۱۰۰ms سخت‌افزاری با رندرینگ LVGL و پروتکل سریال ایجنت.
- نظارت بر تخصیص حافظه (Internal RAM vs PSRAM) و ممانعت از کرش‌های Heap.

## اصول معماری (Siemens Compliance)
- **Layer Separation:** تفکیک دقیق درایورهای سخت‌افزاری (`TFT_eSPI`, `bb_captouch`, `Wire`, `EEPROM`) از لایه‌های میانی منطق (`actions.c`, `variables.c`) و لایه گرافیکی (`screens.c`).
- **Zero ISR-UI Coupling:** ارتباط بین سخت‌افزار زمان‌بندی و هسته UI صرفاً از طریق صف‌ها، فلگ‌های `volatile` یا سمافورها انجام پذیرد.
- **Fail-Safe Defaults:** در صورت قطعی یا خطا در ارتباط I2C تاچ، دستگاه در وضعیت امن باقی بماند.
