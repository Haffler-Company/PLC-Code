# مستندات فنی و اتصال سامانه پیامک هافلر (SMS.IR)
## SMS Gateway Technical Reference & API Specification

> **هدف:** اتصال سیستم‌های اعلان صنعتی، هشدارهای بحرانی خطای دستگاه، وضعیت اقساط و قفل روزشمار، و گزارش‌گیری تله‌متری به سامانه پیامکی سراسری.

---

### ۱. مشخصات احراز هویت و خط ارسال (Credentials & Line Info)

| پارامتر | مقدار | شرح |
|---|---|---|
| **ارائه‌دهنده** | `SMS.IR` (Rest API v2) | پنل پیامک پیشرفته سازمانی |
| **کلید دسترسی (API Key)** | `5SHlWl0KXVb0cT3anLv9gHjC6ehbbKJ1VpokaXA4VlafRQTk` | کلید احراز هویت در هدر رکوئست |
| **هدر احراز هویت** | `X-API-KEY` | هدر الزامی در تمام فراخوانی‌ها |
| **شماره خط اختصاصی** | `30002108037986` | خط پیش‌فرض ثبت‌شده در پنل کاربری هافلر |
| **دامنه اصلی سرویس** | `https://api.sms.ir/v1` | بیس آدرس وب‌سرویس REST |

---

### ۲. نقاط پایانی وب‌سرویس (API Endpoints)

#### الف) استعلام خطوط فعال (Get Available Lines)
* **متد:** `GET`
* **آدرس:** `https://api.sms.ir/v1/line`
* **هدرها:** `X-API-KEY`, `Accept: application/json`

#### ب) استعلام موجودی شارژ پنل (Check Credit)
* **متد:** `GET`
* **آدرس:** `https://api.sms.ir/v1/credit`
* **هدرها:** `X-API-KEY`, `Accept: application/json`

#### ج) ارسال پیامک متنی انبوه / تکی (Send Bulk/Single SMS)
* **متد:** `POST`
* **آدرس:** `https://api.sms.ir/v1/send/bulk`
* **هدرها:**
  * `X-API-KEY: 5SHlWl0KXVb0cT3anLv9gHjC6ehbbKJ1VpokaXA4VlafRQTk`
  * `Content-Type: application/json`
  * `Accept: application/json`
* **بدنه درخواست (JSON Payload):**
  ```json
  {
    "lineNumber": 30002108037986,
    "messageText": "متن پیامک مورد نظر",
    "mobiles": ["09130164461"],
    "sendDateTime": null
  }
  ```

---

### ۳. ابزار پایتون ارسال پیامک (`tools/send_sms.py`)

برای ارسال پیامک یا تست از خط فرمان:
```powershell
# ارسال پیامک تستی
python tools/send_sms.py --mobile 09130164461 --message "سلام. خوبی؟"

# استعلام موجودی شارژ
python tools/send_sms.py --credit
```

---

### ۴. تاریخچه اعتبارسنجی و تست سلامت (Verification History)

* **تاریخ تست:** ۲۵ شهریور ۱۴۰۵ (16-09-2026)
* **شماره مقصد تست:** `+989130164461` (`09130164461`)
* **متن پیام تست:** «سلام. خوبی؟»
* **نتیجه ارسال:** موفق (HTTP 200 OK)
* **شناسه بسته (Pack ID):** `609b8b10-ab0b-4927-8077-0844d8484c33`
* **شناسه پیام (Message ID):** `720672962`
* **هزینه کسرشده:** ۱ پیامک
