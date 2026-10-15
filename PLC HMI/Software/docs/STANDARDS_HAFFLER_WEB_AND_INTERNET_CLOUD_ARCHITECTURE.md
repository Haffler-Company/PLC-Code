# نظام‌نامه جامع معماری اتصال وب و اینترنت ابری ماشین‌آلات هافلر
## Unified Cloud WebPortal, MQTT/WebSockets Architecture & Cross-Machine SCADA Spec
**نسخه:** ۱.۰.۰ | **تاریخ تصویب:** ۲۰۲۶.۰۹.۱۷ | **مرجع:** مهندسی اتوماسیون هافلر (Haffler Industrial Automation)

> **مخاطب:** کلیه ایجنت‌های هوش مصنوعی و توسعه‌دهندگان فریمورک، پرتال وب و اسکادای ابری برای تمامی ۱۱ ماشین هافلر.

---

## ۱. چشم‌انداز معماری هیبریدی (Dual-Mode Hybrid Architecture)

پرتال وب ماشین‌آلات هافلر به صورت **دوحالته (Hybrid Dual-Mode)** طراحی شده است تا دسترسی اپراتور و کارفرما در هر شرایطی تضمین شود:

```
+-----------------------------------------------------------------------------------+
|                           Haffler Cloud SCADA Web Portal                          |
|                     (Any Browser / Mobile 4G-5G / Tablet / PC)                    |
+-----------------------------------------+-----------------------------------------+
                                          |
              +---------------------------+---------------------------+
              |                                                       |
     [حالت شبکه محلی / LAN]                                 [حالت اینترنت ابری / Cloud]
  Direct HTTP REST API (Port 80)                         MQTT over WebSockets (WSS 8884/8084)
              |                                                       |
  +-----------v------------+                               +----------v-----------+
  |  ESP32-S3 Web Server   |                               |  Haffler Cloud Broker |
  | (192.168.4.1 / Local)  |                               | (haffler-ai.ir / S3) |
  +-----------+------------+                               +----------+-----------+
              |                                                       | Standard MQTT (1883/8883)
              +---------------------------+---------------------------+
                                          |
                              +-----------v------------+
                              |   ESP32-S3 Core 0      |
                              |  (cloud_client Task)   |
                              +-----------+------------+
                                          | Thread-Safe Interlock
                              +-----------v------------+
                              |   ESP32-S3 Core 1      |
                              |  (Deterministic PLC)   |
                              +------------------------+
```

### ۱. حالت شبکه محلی (Local LAN / AP Mode):
- مناسب زمانی که کاربر در کارگاه حضور دارد و مستقیماً به وای‌فای داخلی دستگاه (SSID: `HAFFLER-ICE-...` و IP: `192.168.4.1`) یا مودم شبکه داخلی متصل است.
- ارسال درخواست‌های سبُک REST به اندپوینت‌های محلی (`/api/status`, `/api/cmd`, `/api/settings`, `/api/ota`).

### ۲. حالت اینترنت ابری سراسری (Global Cloud / 4G-5G Mode):
- مناسب زمانی که کاربر در هر نقطه‌ای از جهان با گوشی موبایل، تبلت یا لپ‌تاپ خود می‌خواهد وضعیت کارگاه و ماشین‌ها را ببیند.
- **بدون نیاز به IP استاتیک، بدون پورت‌فورواردینگ و بدون VPN:** مرورگر وب کاربر از طریق پروتکل امن **MQTT over WebSockets (`wss://`)** به سرور ابری هافلر متصل می‌شود.
- دستگاه ESP32-S3 نیز در کارگاه به وای‌فای متصل بوده و کلاینت سبک `cloud_client` آن به بروکر ابری وصل است.
- برقراری ارتباط دوطرفه بلادرنگ با حداقل مصرف اینترنت (کمتر از چند کیلوبایت در ساعت).

---

## ۲. ساختار یکپارچه تاپیک‌ها برای تمامی ۱۱ ماشین هافلر

الگوی رسمی تاپیک‌های MQTT بر اساس استاندارد مادر:
`haffler/{machine_slug}/{device_id}/{channel}`

| ردیف | عنوان ماشین | Machine Slug | شناسه پیش‌فرض | کانال تله‌متری | کانال فرامین | کانال تاییدیه |
|:---:|:---|:---:|:---:|:---|:---|:---|
| ۱ | بستنی‌ساز صنعتی ASTIAJ CM-700 | `ice_cream` | `HAFFLER-ICE-XXXXXX` | `haffler/ice_cream/{id}/telemetry` | `haffler/ice_cream/{id}/cmd` | `haffler/ice_cream/{id}/ack` |
| ۲ | جوشکاری تک‌سر UPVC | `weld_single` | `HAFFLER-WELD1-XXXX` | `haffler/weld_single/{id}/telemetry` | `haffler/weld_single/{id}/cmd` | `haffler/weld_single/{id}/ack` |
| ۳ | جوشکاری دوسر UPVC | `weld_double` | `HAFFLER-WELD2-XXXX` | `haffler/weld_double/{id}/telemetry` | `haffler/weld_double/{id}/cmd` | `haffler/weld_double/{id}/ack` |
| ۴ | اره برش دیجیتال تک‌سر | `cut_single` | `HAFFLER-CUT1-XXXX` | `haffler/cut_single/{id}/telemetry` | `haffler/cut_single/{id}/cmd` | `haffler/cut_single/{id}/ack` |
| ۵ | اره برش دیجیتال دوسر | `cut_double` | `HAFFLER-CUT2-XXXX` | `haffler/cut_double/{id}/telemetry` | `haffler/cut_double/{id}/cmd` | `haffler/cut_double/{id}/ack` |
| ۶ | تمیزکن گوشه CNC چهار محور | `corner_clean` | `HAFFLER-CNC-XXXX` | `haffler/corner_clean/{id}/telemetry` | `haffler/corner_clean/{id}/cmd` | `haffler/corner_clean/{id}/ack` |
| ۷ | پرس پانچ هیدرولیک آلومینیوم | `punch` | `HAFFLER-PUNCH-XXXX` | `haffler/punch/{id}/telemetry` | `haffler/punch/{id}/cmd` | `haffler/punch/{id}/ack` |
| ۸ | فرز کپی و سه‌سوراخ‌زن | `copy_router` | `HAFFLER-ROUTER-XXXX`| `haffler/copy_router/{id}/telemetry`| `haffler/copy_router/{id}/cmd`| `haffler/copy_router/{id}/ack`|
| ۹ | تزریق گاز آرگون شیشه دوجداره | `gas_filling` | `HAFFLER-GAS-XXXX` | `haffler/gas_filling/{id}/telemetry` | `haffler/gas_filling/{id}/cmd` | `haffler/gas_filling/{id}/ack` |
| ۱۰ | ماشین‌آلات نانوایی و خمیرگیر | `bakery` | `HAFFLER-BAKE-XXXX` | `haffler/bakery/{id}/telemetry` | `haffler/bakery/{id}/cmd` | `haffler/bakery/{id}/ack` |
| ۱۱ | میکسر صنعتی نانوایی | `bakery_mixer` | `HAFFLER-MIX-XXXX` | `haffler/bakery_mixer/{id}/telemetry`| `haffler/bakery_mixer/{id}/cmd`| `haffler/bakery_mixer/{id}/ack`|

---

## ۳. استاندارد کانال‌های چهارگانه مخابرات ابری

### ۳.۱ کانال اعلام حضور و سلامت (`/online`)
- **تاپیک:** `haffler/{machine_slug}/{device_id}/online`
- **پیام برخط شدن (Connect با Retain=true و QoS=1):**
```json
{
  "online": true,
  "dev_id": "HAFFLER-ICE-8846C4",
  "model": "ICE CREAM CM-700",
  "cust": "کارگاه هافلر",
  "city": "تهران",
  "sn": "HAFFLER-ICE-8846C4",
  "ver": "3.6.0",
  "ip": "192.168.1.45",
  "rssi": -58
}
```
- **پیام آخرین وصیت هنگام قطع ناگهانی (LWT با Retain=true و QoS=1):**
```json
{
  "online": false,
  "dev_id": "HAFFLER-ICE-8846C4",
  "reason": "lwt_connection_lost"
}
```

### ۳.۲ کانال تله‌متری بلادرنگ با نرخ تطبیقی (`/telemetry`)
- **تاپیک:** `haffler/{machine_slug}/{device_id}/telemetry`
- **قانون نرخ تطبیقی (Adaptive Rate Regulation):**
  - **حالت کارکرد فعال (Active Production / Dispense / Wash):** ارسال هر **۱ ثانیه** جهت نظارت بی‌درنگ بر دما و بار موتور.
  - **حالت آماده‌باش (Idle / Standby):** ارسال هر **۵ ثانیه** جهت کاهش مصرف دیتای اینترنت و پایداری رم.
  - **رویدادهای آنی (Event-Triggered):** در لحظه تغییر ورودی/خروجی، بروز خطا، یا اجرای فرمان، تله‌متری بلافاصله (زیر ۵۰ms) ارسال می‌گردد.

### ۳.۳ کانال صدور فرمان (`/cmd`)
- **تاپیک:** `haffler/{machine_slug}/{device_id}/cmd`
- **ساختار استاندارد:**
```json
{
  "req_id": "req_1726589000",
  "action": "make_toggle",
  "params": {},
  "pin": "",
  "timestamp": 1726589000
}
```

### ۳.۴ کانال تاییدیه سخت‌افزار (`/ack` و `/cmd_resp`)
- **تاپیک:** `haffler/{machine_slug}/{device_id}/ack` (همچنین جهت سازگاری با سیستم‌های قدیمی به `/cmd_resp` نیز ارسال می‌شود).
- **ساختار رسمی استاندارد:**
```json
{
  "req_id": "req_1726589000",
  "dev_id": "HAFFLER-ICE-8846C4",
  "status": "SUCCESS",
  "ok": true,
  "executed_action": "make_toggle",
  "code": 0,
  "message": "چرخه تولید بستنی آغاز شد."
}
```

---

## ۴. امنیت سایبری و ایمنی صنعتی فرامین از راه دور

1. **اینترلاک‌های ایمنی سخت‌افزاری (Hardware Interlocks):**
   - هیچ فرمان کنترلی اینترنتی نمی‌تواند اینترلاک‌های ایمنی (مانند شستی E-Stop، خطای اورلود موتور، یا سنسور انجماد سیلندر) را دور بزند.
   - کلیه دستورات ابتدا در تسک کنترل هسته ۱ اعتبارسنجی شده و در صورت ایمن بودن اجرا می‌شوند.
2. **پیشگیری از فرمان‌های ناخواسته هنگام قطعی اینترنت:**
   - فرامین دارای تایم‌اوت اعتبارسنجی (`timestamp`) هستند تا فرامین معوق با تأخیر اجرا نشوند.
3. **محافظت از قفل اقساط و رمزهای عبور:**
   - فرامین تغییر روزشمار یا ریست فقط مشروط به ارسال پین معتبر (`pin`) در بدنه جیسون پردازش می‌شوند.

---

## ۵. راهنمای اتصال سریع کارفرما از طریق وب (Zero-Setup Operator Experience)

1. کاربر آدرس پرتال وب هافلر را در مرورگر موبایل یا لپ‌تاپ باز می‌کند.
2. سوئیچ وضعیت بالا را روی **«اینترنت ابری (Cloud Mode)»** قرار می‌دهد.
3. نوع دستگاه (مثلاً بستنی‌ساز یا جوش تک‌سر) را انتخاب کرده و شناسه دستگاه (`Device ID`) را وارد می‌کند.
4. نشانگر نقطه سبز اتصال روشن شده و تمامی پارامترها، دماها، تایمرها و وضعیت رله‌ها به صورت زنده نمایش داده می‌شوند.
5. کلیدهای کنترلی (تولید، نگهداری شبانه، شستشو و ریست خطا) آماده دریافت فرمان می‌شوند.
