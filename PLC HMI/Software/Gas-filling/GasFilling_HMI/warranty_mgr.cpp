#include "warranty_mgr.h"
#include <Preferences.h>
#include <Arduino.h>

#define PIN_OPTO_STOP 3

static Preferences s_prefs;
static bool s_warr_active = false;
static char s_warr_date[32] = {0};
static volatile bool s_stop_nc = false;
static bool s_inited = false;

void warranty_mgr_init(void) {
    if (s_inited) return;
    s_prefs.begin("haffler_hw", false);
    s_warr_active = s_prefs.getBool("warr_act", false);
    String d = s_prefs.getString("warr_date", "");
    if (d.length() > 0) {
        strncpy(s_warr_date, d.c_str(), sizeof(s_warr_date) - 1);
    }
    s_stop_nc = s_prefs.getBool("stop_nc", false);
    s_inited = true;
}

bool warranty_is_active(void) {
    if (!s_inited) warranty_mgr_init();
    return s_warr_active;
}

const char *warranty_get_date(void) {
    if (!s_inited) warranty_mgr_init();
    return s_warr_date;
}

void warranty_activate(const char *date_str) {
    if (!s_inited) warranty_mgr_init();
    s_warr_active = true;
    if (date_str && strlen(date_str) > 0) {
        strncpy(s_warr_date, date_str, sizeof(s_warr_date) - 1);
    } else {
        strcpy(s_warr_date, "1405/06/17");
    }
    s_prefs.putBool("warr_act", true);
    s_prefs.putString("warr_date", String(s_warr_date));
}

bool stop_mode_is_nc(void) {
    if (!s_inited) warranty_mgr_init();
    return s_stop_nc;
}

void stop_mode_set_nc(bool is_nc) {
    if (!s_inited) warranty_mgr_init();
    s_stop_nc = is_nc;
    s_prefs.putBool("stop_nc", is_nc);
}

bool stop_input_is_active(void) {
    /* Pure ISR-safe atomic read: ZERO heap allocations, ZERO mutex, ZERO Preferences */
    int val = digitalRead(PIN_OPTO_STOP);
    if (s_stop_nc) {
        return (val == HIGH);
    } else {
        return (val == LOW);
    }
}

