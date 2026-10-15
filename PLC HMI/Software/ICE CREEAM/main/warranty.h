#ifndef ASTIAJ_WARRANTY_H
#define ASTIAJ_WARRANTY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WARRANTY_GRACE_PERIOD_SEC  (15 * 60)  /* 15 minutes test grace period */

typedef struct {
    uint8_t  active;           /* 1 if warranty has been activated, 0 otherwise */
    char     buyer_name[64];   /* Full Name of Buyer */
    char     city[32];         /* Installation City */
    char     phone[16];        /* Mobile Phone Number */
    char     date_str[32];     /* Registration Date string (Jalali or Gregorian) */
    char     customer_code[16];/* Unique 11-digit Luhn customer code */
    uint8_t  sms_sent;         /* 1 if activation SMS was successfully delivered */
    uint8_t  sms_pending;      /* 1 if offline activated and waiting for Wi-Fi */
} warranty_info_t;

/* Initialize warranty subsystem from NVS */
esp_err_t warranty_init(void);

/* Check if warranty is officially activated */
bool warranty_is_active(void);

/* Check if the 15-minute grace period has expired and machine is locked out */
bool warranty_is_expired_and_locked(void);

/* Dismiss the expired dialog (outputs remain locked until reboot or warranty activation) */
void warranty_dismiss_expired_lock(void);

/* Get remaining grace seconds (0..900) */
uint32_t warranty_get_grace_seconds_left(void);

/* Get current warranty info pointer */
const warranty_info_t *warranty_get_info(void);

/* Generate 11-digit customer code based on MAC address and Luhn checksum */
void warranty_get_customer_code(char *out_code, size_t max_sz);

/* Validate any 11-digit code with Luhn check digit */
bool warranty_verify_customer_code(const char *code);

/* Normalize and sanitize phone number (converts Persian/Arabic digits, strips +98/0098) */
void warranty_sanitize_phone(const char *in, char *out, size_t out_sz);

/* Activate warranty with given buyer name, city, and phone.
 * If wifi is connected, fires SMS immediately.
 * If offline, can be called with is_offline_pin=true to queue SMS for later dispatch.
 */
esp_err_t warranty_activate(const char *name, const char *city, const char *phone, bool is_offline_pin);

/* Called every 1s from system timer to track the 15-minute countdown */
void warranty_tick_1s(void);

/* Called when Wi-Fi connects to send pending SMS if needed */
void warranty_on_wifi_connected(void);

/* Build the complete SMS payload including machine info, day lock schedule and challenge instructions */
void warranty_build_sms_text(char *out_buf, size_t buf_sz);

/* Build customer congratulatory and warranty expiration SMS */
void warranty_build_customer_sms_text(char *out_buf, size_t buf_sz);

/* Trigger SMS task directly in background */
void warranty_trigger_sms_dispatch(void);

/* Build installment unlock SMS text */
void warranty_build_installment_sms_text(char *out_buf, size_t buf_sz, uint8_t stage, const char *pin, int16_t days);

/* Send installment unlock notification SMS to manager in background */
void warranty_send_installment_unlock_sms(uint8_t stage, const char *pin, int16_t days);

/* Get display date formatted LTR (Jalali for FA, Gregorian for EN/TR) */
void warranty_get_display_date(char *out_str, size_t out_sz, bool is_jalali);

#ifdef __cplusplus
}
#endif

#endif // ASTIAJ_WARRANTY_H
