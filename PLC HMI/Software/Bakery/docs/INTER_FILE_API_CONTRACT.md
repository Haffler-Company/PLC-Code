# Inter-File API & Data Flow Contract — Bakery HMI

This document specifies the interface contracts, global variable ownership, and asynchronous communication patterns across modules in the Bakery HMI firmware.

---

## 1. Subsystem Interconnection Matrix

```text
┌─────────────────┐
│     ui.c/h      │◄─────────────────────────────┐
└────────┬────────┘                              │
         │ UI Events                             │ Widget Mutation
         ▼                                       │
┌─────────────────┐       ┌─────────────────┐    │
│ lvgl_function.h ├──────►│    storage.h    │    │
└────────┬────────┘       └────────┬────────┘    │
         │ Flags                   │ Parameters  │
         ▼                         ▼             │
┌─────────────────┐       ┌─────────────────┐    │
│     tasks.h     │──────►│    general.h    ├───┘
└────────┬────────┘       └────────┬────────┘
         │                         │
         ▼                         ▼
  [ FreeRTOS Tasks ]        [ Hardware I/O ]
```

---

## 2. Global Shared State & Ownership Contract

| Global Variable | Declared In | Consumed In | Role & Synchronization Requirement |
|---|---|---|---|
| `ScreenMap currentScreen` | `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | Active UI screen enum (`WARNING`, `MAIN`, `AUTOMATIC`, `MANUAL`, `SETTING`) |
| `bool processActive` | `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | Master flag indicating machine running state |
| `bool cycleStarted` | `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | Automatic motion loop execution trigger |
| `bool isAutoScreenActive` | `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | Controls whether `app_task` updates automatic progress bar |
| `bool isFunneleLeft` / `Right` | `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | Flags for manual funnel jogging |
| `bool isPistonUp` / `Down` | `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | Flags for manual piston jogging |
| `Storage storage` | `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | Global instance of NVS storage wrapper |
| `TaskHandle_t buzzerTaskHandle`| `ChoneProject.ino` | `tasks.h`, `lvgl_function.h` | FreeRTOS notification handle for non-blocking acoustic cues |

---

## 3. Function Call Contracts

1. **Parameter Updates via UI:**
   - Any modification to machine parameters (`auto_weight`, `auto_speed`, `max_piston`, `max_funnle`) must route through setters in `Storage` class (e.g. `storage.setAutoWeight(...)`).
   - Setters automatically sync to NVS non-volatile flash.
2. **Acoustic Event Notification:**
   - Tasks must never call blocking buzzer delays directly. Instead, send an asynchronous FreeRTOS notification:
     ```cpp
     xTaskNotifyGive(buzzerTaskHandle);
     ```
3. **Emergency Motion Stop:**
   - Functions `pistonStop()` and `funneleStop()` in `general.h` immediately de-energize all directional and motor relays on hardware GPIOs.
