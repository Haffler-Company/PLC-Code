#ifndef UI_BREATH_H
#define UI_BREATH_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Build Breath + restyle General rows inside the General tab. */
void ui_breath_create(lv_obj_t *general_tab);

/** Soft opacity pulse + filling-sync while Setting/General is visible. */
void ui_breath_tick(void);

/** Stop anims / clear focus when leaving Setting. */
void ui_breath_on_leave(void);

#ifdef __cplusplus
}
#endif

#endif
