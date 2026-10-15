#ifndef UI_LOCALIZATION_H
#define UI_LOCALIZATION_H

#include <Arduino.h>

namespace ui_localization {

int32_t get_ui_language_index();
const char *get_wrong_password_text();

// Warranty localization strings
const char *get_warr_title_text();
const char *get_warr_name_err_text();
const char *get_warr_phone_err_text();
const char *get_warr_submit_text();
const char *get_warr_offline_text();
const char *get_warr_cancel_text();
const char *get_warr_prompt_text();
const char *get_warr_expired_text();
const char *get_warr_expired_desc();
const char *get_warr_dismiss_text();
const char *get_warr_code_prefix();
const char *get_warr_name_ph();
const char *get_warr_city_ph();
const char *get_warr_no_net_title();
const char *get_warr_no_net_desc();

} // namespace ui_localization

#endif // UI_LOCALIZATION_H
