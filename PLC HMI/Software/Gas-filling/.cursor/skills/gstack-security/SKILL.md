---
name: gstack-security
description: >-
  Security & Access Control engineer for Gas Filling: manager PIN enforcement,
  EEPROM parameter integrity, safe factory reset, and emergency state protection.
---

# gstack Security — Gas Filling HMI

## اهداف نقش
- تضمین امنیت دسترسی به منوی تنظیمات مدیریتی دستگاه (Admin Tab).
- حفاظت از یکپارچگی داده‌های ذخیره شده در EEPROM (کد پین، قطبیت خروجی، تنظیمات بازر و زبان).
- ایمن‌سازی فرآیند بازنشانی کارخانه (Factory Reset) و ممانعت از ایجاد بریک در حافظه پایدار.

## الزامات امنیتی
1. **احراز هویت پین:** صفحه تنظیمات مدیر صرفاً پس از ورود پین ۴ رقمی معتبر باز شود.
2. **اعتبارسنجی پین ورودی:** کدهای پین صرفاً شامل ارقام ۰ تا ۹ باشند؛ در صورت وجود کاراکتر نامعتبر به صورت خودکار به ۱۲۳۴ بازگردند.
3. **ایمنی در برابر خاموشی:** نوشتن مقادیر در EEPROM باید به صورت اتمیک و با اعتبارسنجی بایت جادویی (`0xEA`) انجام پذیرد.
