#include "plane.h"

vec2d plane_to_screen(const plane *plane, double x, double y) {
    return (vec2d){
        .x = ((x + plane->offset.x) * plane->scale.x * plane->zoom),
        .y = -((y + plane->offset.y) * plane->scale.y * plane->zoom),
    };
}

vec2d plane_from_screen_no_offset(const plane *plane, double x, double y) {
    return (vec2d){
        .x = x / plane->scale.x / plane->zoom,
        .y = -y / plane->scale.y / plane->zoom,
    };
}

vec2d plane_from_screen(const plane *plane, double x, double y) {
    return (vec2d){
        .x = (x / plane->scale.x / plane->zoom) - plane->offset.x,
        .y = -(y / plane->scale.y / plane->zoom) - plane->offset.y,
    };
}

void plane_zoom_around(plane *plane, int64_t mouse_x, int64_t mouse_y, double wheel) {
    vec2d mouse_scr = plane_from_screen_no_offset(plane, mouse_x, mouse_y);
    plane->zoom += wheel;
    vec2d new_mouse_scr = plane_from_screen_no_offset(plane, mouse_x, mouse_y);
    plane->offset.x += new_mouse_scr.x - mouse_scr.x;
    plane->offset.y += new_mouse_scr.y - mouse_scr.y;
}
