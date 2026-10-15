#ifndef WARRANTY_MGR_H
#define WARRANTY_MGR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void warranty_mgr_init(void);
bool warranty_is_active(void);
const char *warranty_get_date(void);
void warranty_activate(const char *date_str);

bool stop_mode_is_nc(void);
void stop_mode_set_nc(bool is_nc);
bool stop_input_is_active(void);

#ifdef __cplusplus
}
#endif

#endif // WARRANTY_MGR_H
