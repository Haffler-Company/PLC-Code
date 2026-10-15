# 09. Cross-Project Reference Library & Shared Architecture

## Executive Summary
The Haffler industrial automation portfolio encompasses 11 specialized machines. While their mechanical actuators, process thermodynamics, and sensor topologies differ, they all share an identical hardware core (ESP32-S3 N16R8, 4" 480x320 IPS display, GT911 touch, 10-key tactile ladder, 8 optocoupled inputs, 10 relay outputs, and DS3231 RTC). 

This reference library details lessons learned, state machine topologies, and domain-specific engineering solutions developed across the fleet to prevent duplicate errors and enable rapid cross-project engineering.

---

## 1. Machine-by-Machine Technical Encyclopedia

### 1. UPVC Single-Head Welding Machine (`Welding_single_head`)
- **Process Sequence:** Profile Clamp Down $\rightarrow$ Heater Plate In $\rightarrow$ Melt Compression $\rightarrow$ Heater Plate Out $\rightarrow$ Fusion Joining $\rightarrow$ Cool Down $\rightarrow$ Clamp Release.
- **Key Engineering Solution:** Dual thermocouple PID temperature regulation with zero-crossing solid-state relays (SSRs) to prevent mains EMI noise from coupling into the ADC lines.
- **Safety Interlock:** Heater plate advance is strictly inhibited unless profile clamps are confirmed fully retracted.

### 2. UPVC Double-Head Welding Machine (`Welding_double_head`)
- **Process Sequence:** Synchronized dual-head clamping and fusion of both ends of a UPVC window frame profile simultaneously.
- **Key Engineering Solution:** Master-Slave position synchronization over RS485 Modbus RTU. Head 1 coordinates heating timing with Head 2, compensating for thermal mass differences.
- **Safety Interlock:** Light curtain / safety barrier input on $I_0$ immediately halts carriage traverse.

### 3. UPVC Single-Head Digital Cutter (`Cutter_single_head(Digital_metr)`)
- **Process Sequence:** Target Length Entry $\rightarrow$ Carriage Automatic Position $\rightarrow$ Profile Clamp $\rightarrow$ Safety Hood Close $\rightarrow$ Saw Motor Start $\rightarrow$ Saw Cylinder Up $\rightarrow$ Saw Down $\rightarrow$ Clamp Release.
- **Key Engineering Solution:** ESP32-S3 Pulse Counter (PCNT) peripheral reads optical linear scale at $>100\text{ kHz}$. Implements unilateral **LEFT-Only Backlash Compensation** to eliminate mechanical gear play.
- **Safety Interlock:** Saw blade cannot ascend unless the pneumatic safety hood down-switch is actively closed.

### 4. UPVC Double-Head Precision Cutter (`Cutter double_head`)
- **Process Sequence:** Simultaneous 45° / 90° miter cutting of both ends of window sash and frame profiles.
- **Key Engineering Solution:** Servo-controlled positioning with pneumatic brake shoe clamping on hardened linear rails. Angular tilt cylinders with dual reed switches.
- **Safety Interlock:** Simultaneous dual-hand start button logic (EN 574 Two-Hand Control) prevents operator hands from entering the cutting zone.

### 5. Industrial Soft-Serve Ice Cream Dispenser (`ICE CREEAM`)
- **Process Sequence:** Hopper Refrigeration $\rightarrow$ Cylinder Freezing $\rightarrow$ Auger Dispense $\rightarrow$ Night Pasteurization.
- **Key Engineering Solution:** Auger motor current sensing via ADC transformer. Viscosity of freezing ice cream correlates with motor current; when target current is reached, refrigeration compressor cycles off.
- **Security Feature:** 5-stage cascading day-lock for commercial franchise leasing with Jalali calendar integration.

### 6. Insulating Glass Argon Gas-Filling Machine (`Gas-filling`)
- **Process Sequence:** Glass Unit Vacuum Purge $\rightarrow$ Argon Gas Injection $\rightarrow$ Sensor O₂ Concentration Analysis $\rightarrow$ Hole Sealing.
- **Key Engineering Solution:** High-accuracy differential pressure sensor reads chamber vacuum. Electrochemical oxygen sensor measures residual air down to $<10\text{ ppm}$ to guarantee $>90\%$ argon fill ratio.
- **Calibration Routine:** Automatic 21.0% ambient air zero-calibration routine in F5 settings.

### 7. Industrial Commercial Bakery Oven (`Bakery`)
- **Process Sequence:** Pre-Heat $\rightarrow$ Loading $\rightarrow$ Steam Injection $\rightarrow$ Rotary Rack Traverse $\rightarrow$ Multi-Stage Baking $\rightarrow$ Exhaust Vent.
- **Key Engineering Solution:** Multi-stage recipe curve (Time vs. Temperature) stored in SPIFFS/LittleFS. High-pressure steam solenoid pulsed for precise millisecond bursts.
- **Safety Interlock:** Door limit switch instantly cuts convection blowers to prevent hot air discharge onto the baker.

### 8. UPVC Corner Cleaning Machine (`Corner_cleaning`)
- **Process Sequence:** Welded Frame Placement $\rightarrow$ Vacuum Clamp $\rightarrow$ Top/Bottom Milling Knife $\rightarrow$ Outer Contour Saw $\rightarrow$ Inner Corner Drill.
- **Key Engineering Solution:** Multi-axis pneumatic sequencing using hardware timer interrupts. Automatic profile shape detection via 3 inductive sensors.
- **Maintenance Counter:** Tool cycle counter triggers sharpening warning every 10,000 cycles.

### 9. UPVC Copy Router Machine (`Copy_router`)
- **Process Sequence:** Triple-Hole Drilling (Water Slot & Handle) $\rightarrow$ Profile Milling along mechanical template.
- **Key Engineering Solution:** High-speed electro-spindle control (12,000 - 18,000 RPM) driven via 0-10V analog signal or RS485 Modbus VFD inverter.
- **Safety Interlock:** Pneumatic clamp pressure sensor halts spindle run if line pressure drops below $5\text{ bar}$.

### 10. Aluminum Profile Punching Press (`Aluminum-punching`)
- **Process Sequence:** Die Selection $\rightarrow$ Profile Insertion $\rightarrow$ Hydraulic/Pneumatic Ram Down $\rightarrow$ Slug Ejection $\rightarrow$ Ram Return.
- **Key Engineering Solution:** Microsecond response valve firing. High-tonnage hydraulic stroke limit sensors.
- **Safety Standard:** ISO 13849 Category 4 redundant safety circuit monitoring.

### 11. Digital Metr / Linear Measurement Gauge (`Digital_metr`)
- **Process Sequence:** Manual/Powered Profile Positioning $\rightarrow$ Real-time Sub-Millimeter Dimension Display $\rightarrow$ Remote Cut-List Sync.
- **Key Engineering Solution:** Ultra-low latency dirty-driven display refresh ($>60\text{ FPS}$). Zero lag during high-speed manual pull of measuring carriage.

---

## 2. Universal Software Architecture Patterns

Across all 11 machines, the following software modules remain 100% identical and reusable:

```
+-------------------------------------------------------------+
|                      USER INTERFACE                         |
|   LVGL 9.4 | Apple HIG Dark Theme | Touchless Keypad FSM    |
+-------------------------------------------------------------+
|                    APPLICATION SERVICES                     |
|  Day-Lock (Jalali) | OTA Manager | Recipe DB | Alarms (ISA) |
+-------------------------------------------------------------+
|               DETERMINISTIC FSM & CONTROL                   |
|   Core 1 Task: PackML Industrial State Engine (50ms Cycle)  |
+-------------------------------------------------------------+
|                      HARDWARE DRIVERS                       |
| Opto Inputs | Clamped Relays | PCNT | ADC Keys | DS3231 RTC |
+-------------------------------------------------------------+
```

### The PackML State Model Standard
Every machine state machine inherits from the PackML standard state machine:
- `STATE_IDLE`
- `STATE_STARTING`
- `STATE_EXECUTE`
- `STATE_COMPLETING`
- `STATE_STOPPING`
- `STATE_STOPPED`
- `STATE_ABORTING`
- `STATE_ABORTED`
- `STATE_HOLDING`
- `STATE_HELD`

By adhering to this state architecture, any technician or developer moving between machines encounters familiar diagnostic states, alarm structures, and control logic.
