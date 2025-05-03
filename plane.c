#include "plane.h"

ms_vec2d plane_to_screen(const ms_plane *p, double x, double y) {
    return (ms_vec2d){
        .x = ((x + p->offset.x) * p->scale.x * p->zoom),
        .y = -((y + p->offset.y) * p->scale.y * p->zoom),
    };
}

ms_vec2d plane_from_screen_no_offset(const ms_plane *p, double x, double y) {
    return (ms_vec2d){
        .x = x / p->scale.x / p->zoom,
        .y = -y / p->scale.y / p->zoom,
    };
}

ms_vec2d plane_from_screen(const ms_plane *p, double x, double y) {
    return (ms_vec2d){
        .x = (x / p->scale.x / p->zoom) - p->offset.x,
        .y = -(y / p->scale.y / p->zoom) - p->offset.y,
    };
}

void plane_zoom_around(ms_plane *p, int64_t mouse_x, int64_t mouse_y, double wheel) {
    ms_vec2d mouse_scr = plane_from_screen_no_offset(p, mouse_x, mouse_y);
    p->zoom += wheel;
    ms_vec2d new_mouse_scr = plane_from_screen_no_offset(p, mouse_x, mouse_y);
    p->offset.x += new_mouse_scr.x - mouse_scr.x;
    p->offset.y += new_mouse_scr.y - mouse_scr.y;
}
