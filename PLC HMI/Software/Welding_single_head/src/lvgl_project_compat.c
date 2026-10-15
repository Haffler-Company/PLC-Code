#include "lvgl.h"
#include <stddef.h>

#if LV_USE_SPINNER
lv_obj_t * lvgl_project_spinner_create(lv_obj_t * parent, uint32_t time_ms, uint32_t arc_len)
{
    lv_obj_t * obj = lv_spinner_create(parent);
    if (obj) lv_spinner_set_anim_params(obj, time_ms, arc_len);
    return obj;
}
#endif

lv_obj_t * lvgl_project_event_get_target(lv_event_t * e)
{
    return (lv_obj_t *)lv_event_get_target(e);
}

lv_obj_t * lvgl_project_event_get_current_target(lv_event_t * e)
{
    return (lv_obj_t *)lv_event_get_current_target(e);
}

typedef struct {
    lv_point_precise_t * pts;
} line_pts_store_t;

static void line_pts_delete_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    lv_obj_t * obj = lv_event_get_target(e);
    line_pts_store_t * store = lv_obj_get_user_data(obj);
    if (!store) return;
    if (store->pts) lv_free(store->pts);
    lv_free(store);
    lv_obj_set_user_data(obj, NULL);
}

void lvgl_project_line_set_points(lv_obj_t * obj, const lv_point_t points[], uint32_t point_num)
{
    if (!obj || !points || point_num == 0) return;

    line_pts_store_t * store = lv_obj_get_user_data(obj);
    if (!store) {
        store = lv_malloc(sizeof(line_pts_store_t));
        if (!store) return;
        store->pts = NULL;
        lv_obj_set_user_data(obj, store);
        lv_obj_add_event_cb(obj, line_pts_delete_cb, LV_EVENT_DELETE, NULL);
    } else if (store->pts) {
        lv_free(store->pts);
        store->pts = NULL;
    }

    store->pts = lv_malloc(point_num * sizeof(lv_point_precise_t));
    if (!store->pts) return;

    for (uint32_t i = 0; i < point_num; i++) {
        store->pts[i].x = (lv_value_precise_t)points[i].x;
        store->pts[i].y = (lv_value_precise_t)points[i].y;
    }

    /* LVGL 9 keeps a pointer to the array — must outlive the line object */
    lv_line_set_points_mutable(obj, store->pts, point_num);
}
