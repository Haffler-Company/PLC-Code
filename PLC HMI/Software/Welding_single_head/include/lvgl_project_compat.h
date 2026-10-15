#ifndef LVGL_PROJECT_COMPAT_H
#define LVGL_PROJECT_COMPAT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* v8 memory API used by EEZ-generated code */
#ifndef lv_mem_alloc
#define lv_mem_alloc  lv_malloc
#endif
#ifndef lv_mem_free
#define lv_mem_free   lv_free
#endif
#ifndef lv_mem_realloc
#define lv_mem_realloc lv_realloc
#endif

/* v8 image color-format constants -> v9 */
#ifndef LV_IMG_CF_TRUE_COLOR
#define LV_IMG_CF_TRUE_COLOR              LV_COLOR_FORMAT_RGB565
#endif
#ifndef LV_IMG_CF_TRUE_COLOR_ALPHA
#define LV_IMG_CF_TRUE_COLOR_ALPHA        LV_COLOR_FORMAT_ARGB8565
#endif
#ifndef LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED
#define LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED LV_COLOR_FORMAT_RGB565
#endif

/* v8 widget/event helpers -> v9 (application code only) */
#ifndef lv_textarea_del_char
#define lv_textarea_del_char lv_textarea_delete_char
#endif

#define lv_event_send(O, C, P) lv_obj_send_event((lv_obj_t *)(O), (lv_event_code_t)(C), (P))

#ifndef lv_obj_del_delayed
#define lv_obj_del_delayed lv_obj_delete_delayed
#endif

lv_obj_t * lvgl_project_event_get_target(lv_event_t * e);
lv_obj_t * lvgl_project_event_get_current_target(lv_event_t * e);
#define lv_event_get_target(E) lvgl_project_event_get_target(E)
#define lv_event_get_current_target(E) lvgl_project_event_get_current_target(E)

void lvgl_project_line_set_points(lv_obj_t * obj, const lv_point_t points[], uint32_t point_num);
#define lv_line_set_points(O, PTS, N) lvgl_project_line_set_points((O), (PTS), (N))

/* v8 style property names -> v9 */
#define lv_obj_set_style_arc_img_src lv_obj_set_style_arc_image_src

/* v8 class/type names */
#define lv_img_class           lv_image_class
#define lv_btn_class           lv_button_class
#define lv_btnmatrix_class     lv_buttonmatrix_class

#ifdef __cplusplus
static inline void lvgl_compat_obj_clear_flag(lv_obj_t * obj, int flags)
{
    lv_obj_remove_flag(obj, (lv_obj_flag_t)flags);
}
static inline void lvgl_compat_obj_add_flag(lv_obj_t * obj, int flags)
{
    lv_obj_add_flag(obj, (lv_obj_flag_t)flags);
}
#undef lv_obj_clear_flag
#undef lv_obj_add_flag
#define lv_obj_clear_flag lvgl_compat_obj_clear_flag
#define lv_obj_add_flag   lvgl_compat_obj_add_flag
#endif

/* v8 spinner: create(parent, time_ms, arc_deg) */
lv_obj_t * lvgl_project_spinner_create(lv_obj_t * parent, uint32_t time_ms, uint32_t arc_len);
#define lv_spinner_create(P, T, A) lvgl_project_spinner_create((P), (uint32_t)(T), (uint32_t)(A))

/* lv_meter removed in v9 — compatibility shim */
#include "lv_meter_compat.h"

#ifdef __cplusplus
}
#endif

#endif
